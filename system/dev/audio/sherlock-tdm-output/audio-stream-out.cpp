// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/debug.h>

#include <ddktl/pdev.h>

#include <soc/aml-t931/t931-gpio.h>

#include "audio-stream-out.h"

namespace audio {
namespace sherlock {

// Expects L+R for tweeters + L+R for the 1 Woofer (mixed in HW).
// The user must perform crossover filtering on these channels.
constexpr size_t kNumberOfChannels = 4;
// Calculate ring buffer size for 1 second of 16-bit, 48kHz.
constexpr size_t kRingBufferSize = fbl::round_up<size_t, size_t>(48000 * 2 * kNumberOfChannels,
                                                                 PAGE_SIZE);

SherlockAudioStreamOut::SherlockAudioStreamOut(zx_device_t* parent)
    : SimpleAudioStream(parent, false), pdev_(parent) {
}

zx_status_t SherlockAudioStreamOut::InitPdev() {
    if (!pdev_.is_valid()) {
        return ZX_ERR_NO_RESOURCES;
    }

    audio_fault_ = pdev_.GetGpio(0);
    audio_en_ = pdev_.GetGpio(1);

    if (!audio_fault_.is_valid() || !audio_en_.is_valid()) {
        zxlogf(ERROR, "%s failed to allocate gpio\n", __func__);
        return ZX_ERR_NO_RESOURCES;
    }

    codec_tweeters_ = Tas5760::Create(pdev_, 0);
    if (!codec_tweeters_) {
        zxlogf(ERROR, "%s could not get tas5760\n", __func__);
        return ZX_ERR_NO_RESOURCES;
    }
    codec_woofer_ = Tas5720::Create(pdev_, 1);
    if (!codec_woofer_) {
        zxlogf(ERROR, "%s could not get tas5720\n", __func__);
        return ZX_ERR_NO_RESOURCES;
    }

    zx_status_t status = pdev_.GetBti(0, &bti_);
    if (status != ZX_OK) {
        zxlogf(ERROR, "%s could not obtain bti - %d\n", __func__, status);
        return status;
    }

    std::optional<ddk::MmioBuffer> mmio;
    status = pdev_.MapMmio(0, &mmio);
    if (status != ZX_OK) {
        return status;
    }
    aml_audio_ = AmlTdmDevice::Create(*std::move(mmio), HIFI_PLL, TDM_OUT_C, FRDDR_A, MCLK_A);
    if (aml_audio_ == nullptr) {
        zxlogf(ERROR, "%s failed to create tdm device\n", __func__);
        return ZX_ERR_NO_MEMORY;
    }

    // Drive strength settings
    status = pdev_.MapMmio(1, &mmio);
    if (status != ZX_OK) {
        return status;
    }
    // Strength 1 for sclk (bit 14, GPIOZ(7)) and lrclk (bit 12, GPIOZ(6)),
    // GPIO offsets are in 4 bytes units.
    mmio->SetBits<uint32_t>((1 << 14) | (1 << 12), 4 * T931_PAD_DS_REG4A);
    status = pdev_.MapMmio(2, &mmio);
    if (status != ZX_OK) {
        return status;
    }
    // Strength 1 for mclk (bit 18,  GPIOAO(9)), GPIO offsets are in 4 bytes units.
    mmio->SetBit<uint32_t>(18, 4 * T931_AO_PAD_DS_A);

    audio_en_.Write(1); // SOC_AUDIO_EN.

    codec_tweeters_->Init(); // No slot setting, always uses L+R.
    codec_woofer_->Init(0);  // Use TDM slot 0.

    InitBuffer(kRingBufferSize);

    aml_audio_->SetBuffer(pinned_ring_buffer_.region(0).phys_addr,
                          pinned_ring_buffer_.region(0).size);

    // Setup Stereo Left Justified:
    // -lrclk duty = 64 sclk (SetSclkDiv lrdiv=63 below).
    // -No delay from the time the lrclk signal changes state state to the first bit of data on the
    // data lines  (ConfigTdmOutSlot bitoffset=4 below accomplishes this).
    // -3072MHz/64 = 48KHz.

    // 4 bitoffset, 2 slots, 32 bits/slot, 16 bits/sample, enable mix L+R on lane 1.
    aml_audio_->ConfigTdmOutSlot(4, 1, 31, 15, (1 << 1));

    // Lane 0 L channel set to FRDDR slot 0.
    // Lane 0 R channel set to FRDDR slot 1.
    // Lane 1 L channel set to FRDDR slot 2.  Mixed with R, see ConfigTdmOutSlot above.
    // Lane 1 R channel set to FRDDR slot 3.  Mixed with L, see ConfigTdmOutSlot above.
    aml_audio_->ConfigTdmOutSwaps(0x00003210);

    // Tweeters: Lane 0, unmask TDM slots 0 & 1 (L+R FRDDR slots 0 & 1).
    aml_audio_->ConfigTdmOutLane(0, 0x00000003);

    // Woofer: Lane 1, unmask TDM slot 0 & 1 (Woofer FRDDR slots 2 & 3).
    aml_audio_->ConfigTdmOutLane(1, 0x00000003);

    // mclk = T931_HIFI_PLL_RATE/125 = 1536MHz/125 = 12.288MHz.
    aml_audio_->SetMclkDiv(124);

    // Per schematic, mclk uses pad 0 (MCLK_0 instead of MCLK_1).
    aml_audio_->SetMClkPad(MCLK_PAD_0);

    // sclk = 12.288MHz/4 = 3.072MHz, 32L + 32R sclks = 64 sclks.
    aml_audio_->SetSclkDiv(3, 31, 63);

    aml_audio_->Sync();

    return ZX_OK;
}

zx_status_t SherlockAudioStreamOut::Init() {
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
    float tweeters_gain = codec_tweeters_->GetGain();
    status = codec_woofer_->SetGain(tweeters_gain);
    if (status != ZX_OK) {
        return status;
    }
    cur_gain_state_.cur_gain = codec_woofer_->GetGain();
    cur_gain_state_.cur_mute = false;
    cur_gain_state_.cur_agc = false;

    cur_gain_state_.min_gain = fbl::max(codec_tweeters_->GetMinGain(), codec_woofer_->GetMinGain());
    cur_gain_state_.max_gain = fbl::min(codec_tweeters_->GetMaxGain(), codec_woofer_->GetMaxGain());
    cur_gain_state_.gain_step = fbl::max(codec_tweeters_->GetGainStep(),
                                         codec_woofer_->GetGainStep());
    cur_gain_state_.can_mute = false;
    cur_gain_state_.can_agc = false;

    snprintf(device_name_, sizeof(device_name_), "sherlock-audio-out");
    snprintf(mfr_name_, sizeof(mfr_name_), "unknown");
    snprintf(prod_name_, sizeof(prod_name_), "sherlock");

    unique_id_ = AUDIO_STREAM_UNIQUE_ID_BUILTIN_SPEAKERS;

    return ZX_OK;
}

zx_status_t SherlockAudioStreamOut::InitPost() {

    notify_timer_ = dispatcher::Timer::Create();
    if (notify_timer_ == nullptr) {
        return ZX_ERR_NO_MEMORY;
    }

    dispatcher::Timer::ProcessHandler thandler(
        [tdm = this](dispatcher::Timer * timer)->zx_status_t {
            OBTAIN_EXECUTION_DOMAIN_TOKEN(t, tdm->domain_);
            return tdm->ProcessRingNotification();
        });

    return notify_timer_->Activate(domain_, std::move(thandler));
}

// Timer handler for sending out position notifications.
zx_status_t SherlockAudioStreamOut::ProcessRingNotification() {
    ZX_ASSERT(us_per_notification_ != 0);

    // TODO(andresoportus): johngro noticed there is some drifting on notifications here,
    // could be improved with maintaining an absolute time and even better computing using
    // rationals, but higher level code should not rely on this anyways (see MTWN-57).
    notify_timer_->Arm(zx_deadline_after(ZX_USEC(us_per_notification_)));

    audio_proto::RingBufPositionNotify resp = {};
    resp.hdr.cmd = AUDIO_RB_POSITION_NOTIFY;

    resp.ring_buffer_pos = aml_audio_->GetRingPosition();
    return NotifyPosition(resp);
}

zx_status_t SherlockAudioStreamOut::ChangeFormat(const audio_proto::StreamSetFmtReq& req) {
    fifo_depth_ = aml_audio_->fifo_depth();
    external_delay_nsec_ = 0;

    // At this time only one format is supported, and hardware is initialized
    // during driver binding, so nothing to do at this time.
    return ZX_OK;
}

void SherlockAudioStreamOut::ShutdownHook() {
    aml_audio_->Shutdown();
    audio_en_.Write(0);
}

zx_status_t SherlockAudioStreamOut::SetGain(const audio_proto::SetGainReq& req) {
    zx_status_t status = codec_tweeters_->SetGain(req.gain);
    if (status != ZX_OK) {
        return status;
    }
    float tweeters_gain = codec_tweeters_->GetGain();
    // TODO(andresoportus): More options on volume setting, e.g.:
    // -Allow for ratio between tweeters and woofer gains.
    // -Make use of analog gain options in TAS5720.
    // -Add codecs mute and fade support.
    status = codec_woofer_->SetGain(tweeters_gain);
    if (status != ZX_OK) {
        return status;
    }
    cur_gain_state_.cur_gain = codec_woofer_->GetGain();
    return ZX_OK;
}

zx_status_t SherlockAudioStreamOut::GetBuffer(const audio_proto::RingBufGetBufferReq& req,
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

    aml_audio_->SetBuffer(pinned_ring_buffer_.region(0).phys_addr,
                          rb_frames * frame_size_);

    return ZX_OK;
}

zx_status_t SherlockAudioStreamOut::Start(uint64_t* out_start_time) {

    *out_start_time = aml_audio_->Start();

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

zx_status_t SherlockAudioStreamOut::Stop() {
    notify_timer_->Cancel();
    us_per_notification_ = 0;
    aml_audio_->Stop();
    return ZX_OK;
}

zx_status_t SherlockAudioStreamOut::AddFormats() {
    fbl::AllocChecker ac;
    supported_formats_.reserve(1, &ac);
    if (!ac.check()) {
        zxlogf(ERROR, "Out of memory, can not create supported formats list\n");
        return ZX_ERR_NO_MEMORY;
    }

    // Add the range for basic audio support.
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

zx_status_t SherlockAudioStreamOut::InitBuffer(size_t size) {
    zx_status_t status;
    // TODO(ZX-3149): Per johngro's suggestion preallocate contiguous memory (say in
    // platform bus) since we are likely to fail after running for a while and we need to
    // init again (say the devhost is restarted).
    status = zx_vmo_create_contiguous(bti_.get(), size, 0,
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

} // sherlock
} // audio

extern "C" zx_status_t audio_bind(void* ctx, zx_device_t* device, void** cookie) {

    auto stream =
        audio::SimpleAudioStream::Create<audio::sherlock::SherlockAudioStreamOut>(device);
    if (stream == nullptr) {
        return ZX_ERR_NO_MEMORY;
    }

    return ZX_OK;
}
