// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "audio-stream-in.h"

#include <limits.h>
#include <optional>
#include <utility>

#include <ddk/binding.h>
#include <ddk/debug.h>
#include <ddk/driver.h>
#include <ddk/platform-defs.h>
#include <soc/mt8167/mt8167-clk-regs.h>

namespace audio {
namespace mt8167 {

// Expects 2 mics.
constexpr size_t kNumberOfChannels = 2;
// Calculate ring buffer size for 1 second of 16-bit, 48kHz.
constexpr size_t kRingBufferSize = fbl::round_up<size_t, size_t>(48000 * 2 * kNumberOfChannels,
                                                                 PAGE_SIZE);

Mt8167AudioStreamIn::Mt8167AudioStreamIn(zx_device_t* parent)
    : SimpleAudioStream(parent, true /* is input */), pdev_(parent) {
}

zx_status_t Mt8167AudioStreamIn::Init() {
    zx_status_t status;

    status = InitPdev();
    if (status != ZX_OK) {
        return status;
    }

    status = AddFormats();
    if (status != ZX_OK) {
        return status;
    }
    // Set our gain capabilities.
    cur_gain_state_.cur_gain = 0;
    cur_gain_state_.cur_mute = false;
    cur_gain_state_.cur_agc = false;
    cur_gain_state_.min_gain = 0;
    cur_gain_state_.max_gain = 0;
    cur_gain_state_.gain_step = 0;
    cur_gain_state_.can_mute = false;
    cur_gain_state_.can_agc = false;

    snprintf(device_name_, sizeof(device_name_), "mt8167-audio-in");
    snprintf(mfr_name_, sizeof(mfr_name_), "unknown");
    snprintf(prod_name_, sizeof(prod_name_), "mt8167");

    unique_id_ = AUDIO_STREAM_UNIQUE_ID_BUILTIN_MICROPHONE;

    return ZX_OK;
}

zx_status_t Mt8167AudioStreamIn::InitPdev() {
    if (!pdev_.is_valid()) {
        return ZX_ERR_NO_RESOURCES;
    }

    clk_ = pdev_.GetClk(0);
    if (!clk_.is_valid()) {
        zxlogf(ERROR, "%s failed to allocate clk\n", __FUNCTION__);
        return ZX_ERR_NO_RESOURCES;
    }

    codec_reset_ = pdev_.GetGpio(0);
    if (!codec_reset_.is_valid()) {
        zxlogf(ERROR, "%s failed to allocate gpio\n", __FUNCTION__);
        return ZX_ERR_NO_RESOURCES;
    }

    codec_ = Tlv320adc::Create(pdev_, 0); // ADC for TDM in.
    if (!codec_) {
        zxlogf(ERROR, "%s could not get Tlv320adc\n", __func__);
        return ZX_ERR_NO_RESOURCES;
    }

    zx_status_t status = pdev_.GetBti(0, &bti_);
    if (status != ZX_OK) {
        zxlogf(ERROR, "%s could not obtain bti %d\n", __func__, status);
        return status;
    }

    std::optional<ddk::MmioBuffer> mmio_audio, mmio_clk, mmio_pll;
    status = pdev_.MapMmio(0, &mmio_audio);
    if (status != ZX_OK) {
        return status;
    }
    status = pdev_.MapMmio(1, &mmio_clk);
    if (status != ZX_OK) {
        return status;
    }
    status = pdev_.MapMmio(2, &mmio_pll);
    if (status != ZX_OK) {
        return status;
    }

    mt_audio_ = MtAudioInDevice::Create(*std::move(mmio_audio), MtAudioInDevice::I2S3);
    if (mt_audio_ == nullptr) {
        zxlogf(ERROR, "%s failed to create device\n", __FUNCTION__);
        return ZX_ERR_NO_MEMORY;
    }

    // Reset Codec. "After all power supplies are at their specified values, the RESET pin must
    // be driven low for at least 10 ns".
    codec_reset_.Write(0); // Reset.
    zx_nanosleep(zx_deadline_after(ZX_NSEC(10)));
    codec_reset_.Write(1); // Set to "not reset".

    codec_->Init();

    // Initialize the ring buffer
    InitBuffer(kRingBufferSize);

    mt_audio_->SetBuffer(pinned_ring_buffer_.region(0).phys_addr,
                         pinned_ring_buffer_.region(0).size);

    // Configure XO and PLLs for interface aud1.
    clk_.Enable(0); // 0 is the index, enables board_mt8167::kClkAud1.

    // Power up by clearing the power down bit.
    CLK_SEL_9::Get().ReadFrom(&*mmio_clk).set_apll12_div2_pdn(0).WriteTo(&*mmio_clk);   // I2S3.
    CLK_SEL_9::Get().ReadFrom(&*mmio_clk).set_apll12_div5_pdn(0).WriteTo(&*mmio_clk);   // MCK.
    CLK_SEL_9::Get().ReadFrom(&*mmio_clk).set_apll12_div5b_pdn(0).WriteTo(&*mmio_clk);  // BCK.
    CLK_SEL_11::Get().ReadFrom(&*mmio_clk).set_apll12_ck_div5b(15).WriteTo(&*mmio_clk); // BCK.

    // Enable aud1 PLL.
    APLL1_CON0::Get().ReadFrom(&*mmio_pll).set_APLL1_EN(1).WriteTo(&*mmio_pll);

    return ZX_OK;
}

zx_status_t Mt8167AudioStreamIn::ChangeFormat(const audio_proto::StreamSetFmtReq& req) {
    fifo_depth_ = mt_audio_->fifo_depth();
    external_delay_nsec_ = 0;

    // At this time only one format is supported, and hardware is initialized
    //  during driver binding, so nothing to do at this time.
    return ZX_OK;
}

zx_status_t Mt8167AudioStreamIn::GetBuffer(const audio_proto::RingBufGetBufferReq& req,
                                           uint32_t* out_num_rb_frames,
                                           zx::vmo* out_buffer) {
    uint32_t rb_frames =
        static_cast<uint32_t>(pinned_ring_buffer_.region(0).size) / frame_size_;

    if (req.min_ring_buffer_frames > rb_frames) {
        return ZX_ERR_OUT_OF_RANGE;
    }
    zx_status_t status;
    constexpr uint32_t rights = ZX_RIGHT_READ | ZX_RIGHT_WRITE | ZX_RIGHT_MAP | ZX_RIGHT_TRANSFER;
    status = ring_buffer_vmo_.duplicate(rights, out_buffer);
    if (status != ZX_OK) {
        return status;
    }

    *out_num_rb_frames = rb_frames;

    mt_audio_->SetBuffer(pinned_ring_buffer_.region(0).phys_addr,
                         rb_frames * frame_size_);
    return ZX_OK;
}

zx_status_t Mt8167AudioStreamIn::Start(uint64_t* out_start_time) {
    *out_start_time = mt_audio_->Start();

    uint32_t notifs = LoadNotificationsPerRing();
    if (notifs) {
        us_per_notification_ = static_cast<uint32_t>(
            1000 * pinned_ring_buffer_.region(0).size / (frame_size_ * 48 * notifs));
        notify_timer_->Arm(zx_deadline_after(ZX_USEC(us_per_notification_)));
    } else {
        us_per_notification_ = 0;
    }
    return ZX_OK;
}

// Timer handler for sending out position notifications.
zx_status_t Mt8167AudioStreamIn::ProcessRingNotification() {
    ZX_ASSERT(us_per_notification_ != 0);

    notify_timer_->Arm(zx_deadline_after(ZX_USEC(us_per_notification_)));

    audio_proto::RingBufPositionNotify resp = {};
    resp.hdr.cmd = AUDIO_RB_POSITION_NOTIFY;

    resp.ring_buffer_pos = mt_audio_->GetRingPosition();
    return NotifyPosition(resp);
}

zx_status_t Mt8167AudioStreamIn::InitPost() {

    notify_timer_ = dispatcher::Timer::Create();
    if (notify_timer_ == nullptr) {
        return ZX_ERR_NO_MEMORY;
    }

    dispatcher::Timer::ProcessHandler thandler(
        [mt_audio = this](dispatcher::Timer * timer)->zx_status_t {
            OBTAIN_EXECUTION_DOMAIN_TOKEN(t, mt_audio->domain_);
            return mt_audio->ProcessRingNotification();
        });

    return notify_timer_->Activate(domain_, std::move(thandler));
}

zx_status_t Mt8167AudioStreamIn::Stop() {
    notify_timer_->Cancel();
    us_per_notification_ = 0;
    mt_audio_->Stop();
    return ZX_OK;
}

zx_status_t Mt8167AudioStreamIn::AddFormats() {
    fbl::AllocChecker ac;
    supported_formats_.reserve(1, &ac);
    if (!ac.check()) {
        zxlogf(ERROR, "Out of memory, can not create supported formats list\n");
        return ZX_ERR_NO_MEMORY;
    }

    audio_stream_format_range_t range;
    range.min_channels = kNumberOfChannels;
    range.max_channels = kNumberOfChannels;
    range.sample_formats = AUDIO_SAMPLE_FORMAT_16BIT;
    range.min_frames_per_second = 48000;
    range.max_frames_per_second = 48000;
    range.flags = ASF_RANGE_FLAG_FPS_48000_FAMILY;

    supported_formats_.push_back(range);

    return ZX_OK;
}

zx_status_t Mt8167AudioStreamIn::InitBuffer(size_t size) {
    zx_status_t status = zx_vmo_create_contiguous(bti_.get(), size, 0,
                                                  ring_buffer_vmo_.reset_and_get_address());
    if (status != ZX_OK) {
        zxlogf(ERROR, "%s failed to allocate ring buffer vmo - %d\n", __func__, status);
        return status;
    }

    status = pinned_ring_buffer_.Pin(ring_buffer_vmo_, bti_, ZX_VM_PERM_READ | ZX_VM_PERM_WRITE);
    if (status != ZX_OK) {
        zxlogf(ERROR, "%s failed to pin ring buffer vmo - %d\n", __func__, status);
        return status;
    }
    if (pinned_ring_buffer_.region_count() != 1) {
        zxlogf(ERROR, "%s buffer is not contiguous", __func__);
        return ZX_ERR_NO_MEMORY;
    }

    return ZX_OK;
}

} // mt8167
} // audio

__BEGIN_CDECLS

zx_status_t mt_audio_in_bind(void* ctx, zx_device_t* device) {

    auto stream =
        audio::SimpleAudioStream::Create<audio::mt8167::Mt8167AudioStreamIn>(device);
    if (stream == nullptr) {
        return ZX_ERR_NO_MEMORY;
    }

    return ZX_OK;
}

static zx_driver_ops_t mt_audio_in_driver_ops = {
    .version = DRIVER_OPS_VERSION,
    .init = nullptr,
    .bind = mt_audio_in_bind,
    .create = nullptr,
    .release = nullptr,
};

// clang-format off
ZIRCON_DRIVER_BEGIN(mt8167_audio_in, mt_audio_in_driver_ops, "zircon", "0.1", 3)
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_VID, PDEV_VID_MEDIATEK),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_PID, PDEV_PID_MEDIATEK_8167S_REF),
    BI_MATCH_IF(EQ, BIND_PLATFORM_DEV_DID, PDEV_DID_MEDIATEK_AUDIO_IN),
ZIRCON_DRIVER_END(mt8167_audio_in)
// clang-format on

__END_CDECLS
