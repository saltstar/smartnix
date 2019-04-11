// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "mt8167s-display.h"
#include "common.h"
#include "registers-ovl.h"
#include <fbl/auto_call.h>
#include <lib/zx/pmt.h>
#include <zircon/pixelformat.h>
#include <ddk/binding.h>
#include <ddk/platform-defs.h>

namespace mt8167s_display {

namespace {
// List of supported pixel formats
zx_pixel_format_t kSupportedPixelFormats[3] = {
    ZX_PIXEL_FORMAT_ARGB_8888, ZX_PIXEL_FORMAT_RGB_x888, ZX_PIXEL_FORMAT_RGB_565
};
constexpr uint64_t kDisplayId = PANEL_DISPLAY_ID;

struct ImageInfo {
    zx::pmt pmt;
    zx_paddr_t paddr;
    // TODO(payamm): Use fbl lists instead
    list_node_t node;
};

} // namespace

void Mt8167sDisplay::PopulateAddedDisplayArgs(added_display_args_t* args) {
    args->display_id = kDisplayId;
    args->edid_present = false;
    args->panel.params.height = height_;
    args->panel.params.width = width_;
    args->panel.params.refresh_rate_e2 = 3000; // Just guess that it's 30fps
    args->pixel_format_list = kSupportedPixelFormats;
    args->pixel_format_count = countof(kSupportedPixelFormats);
    args->cursor_info_count = 0;
}

// part of ZX_PROTOCOL_DISPLAY_CONTROLLER_IMPL ops
uint32_t Mt8167sDisplay::DisplayControllerImplComputeLinearStride(uint32_t width,
                                                                  zx_pixel_format_t format) {
    return ROUNDUP(width, 32 / ZX_PIXEL_FORMAT_BYTES(format));
}

// part of ZX_PROTOCOL_DISPLAY_CONTROLLER_IMPL ops
void Mt8167sDisplay::DisplayControllerImplSetDisplayControllerInterface(
    const display_controller_interface_t* intf) {
    fbl::AutoLock lock(&display_lock_);
    dc_intf_ = ddk::DisplayControllerInterfaceClient(intf);
    added_display_args_t args;
    PopulateAddedDisplayArgs(&args);
    dc_intf_.OnDisplaysChanged(&args, 1, NULL, 0, NULL, 0, NULL);
}

// part of ZX_PROTOCOL_DISPLAY_CONTROLLER_IMPL ops
zx_status_t Mt8167sDisplay::DisplayControllerImplImportVmoImage(image_t* image, zx::vmo vmo,
                                                                size_t offset) {
    ImageInfo* import_info = new(ImageInfo);
    if (import_info == nullptr) {
        return ZX_ERR_NO_MEMORY;
    }
    auto cleanup = fbl::MakeAutoCall([&]() {
        if (import_info->pmt) {
            import_info->pmt.unpin();
        }
        delete(import_info);
     });

    fbl::AutoLock lock(&image_lock_);
    if (image->type != IMAGE_TYPE_SIMPLE || image->pixel_format != kSupportedPixelFormats[0]) {
        return ZX_ERR_INVALID_ARGS;
    }

    uint32_t stride = DisplayControllerImplComputeLinearStride(image->width, image->pixel_format);
    unsigned pixel_size = ZX_PIXEL_FORMAT_BYTES(image->pixel_format);
    size_t size = ROUNDUP((stride * image->height * pixel_size) +
                          (offset & (PAGE_SIZE - 1)), PAGE_SIZE);
    zx_paddr_t paddr;
    zx_status_t status = bti_.pin(ZX_BTI_PERM_READ | ZX_BTI_PERM_WRITE | ZX_BTI_CONTIGUOUS,
                                  vmo, offset & ~(PAGE_SIZE - 1), size, &paddr, 1,
                                  &import_info->pmt);
    if (status != ZX_OK) {
        DISP_ERROR("Could not pin bit\n");
        return status;
    }
    // Make sure paddr is allocated in the lower 4GB. (ZX-1073)
    ZX_ASSERT((paddr + size) <= UINT32_MAX);
    import_info->paddr = paddr;
    list_add_head(&imported_images_, &import_info->node);
    image->handle = paddr;
    cleanup.cancel();
    return status;
}

// part of ZX_PROTOCOL_DISPLAY_CONTROLLER_IMPL ops
void Mt8167sDisplay::DisplayControllerImplReleaseImage(image_t* image) {
    fbl::AutoLock lock(&image_lock_);
    zx_paddr_t image_paddr = reinterpret_cast<zx_paddr_t>(image->handle);
    ImageInfo* info;
    list_for_every_entry(&imported_images_, info, ImageInfo, node) {
        if (info->paddr == image_paddr) {
            list_delete(&info->node);
            break;
        }
    }
    if (info) {
        info->pmt.unpin();
        delete(info);
    }
}

// part of ZX_PROTOCOL_DISPLAY_CONTROLLER_IMPL ops
uint32_t Mt8167sDisplay::DisplayControllerImplCheckConfiguration(
    const display_config_t** display_configs, size_t display_count,
    uint32_t** layer_cfg_results, size_t* layer_cfg_result_count) {
    if (display_count != 1) {
        ZX_DEBUG_ASSERT(display_count == 0);
        return CONFIG_DISPLAY_OK;
    }
    ZX_DEBUG_ASSERT(display_configs[0]->display_id == PANEL_DISPLAY_ID);

    fbl::AutoLock lock(&display_lock_);

    bool success = true;
    if (display_configs[0]->layer_count > kMaxLayer) {
        success = false;
    } else {
        for (size_t j = 0; j < display_configs[0]->layer_count; j++) {
            switch (display_configs[0]->layer_list[j]->type) {
            case LAYER_TYPE_PRIMARY: {
                const primary_layer_t& layer = display_configs[0]->layer_list[j]->cfg.primary;
                // TODO(payamm) Add support for 90 and 270 degree rotation (ZX-3252)
                if (layer.transform_mode != FRAME_TRANSFORM_IDENTITY &&
                    layer.transform_mode != FRAME_TRANSFORM_REFLECT_X &&
                    layer.transform_mode != FRAME_TRANSFORM_REFLECT_Y &&
                    layer.transform_mode != FRAME_TRANSFORM_ROT_180) {
                    layer_cfg_results[0][j] |= CLIENT_TRANSFORM;
                }
                // TODO(payamm) Add support for scaling (ZX-3228) :
                if (layer.src_frame.width != layer.dest_frame.width ||
                    layer.src_frame.height != layer.dest_frame.height) {
                    layer_cfg_results[0][j] |= CLIENT_FRAME_SCALE;
                }
                // Ony support ALPHA_HW_MULTIPLY
                if (layer.alpha_mode == ALPHA_PREMULTIPLIED) {
                    layer_cfg_results[0][j] |= CLIENT_ALPHA;;
                }
                break;
            }
            case LAYER_TYPE_COLOR: {
                if (j != 0) {
                    layer_cfg_results[0][j] |= CLIENT_USE_PRIMARY;
                }
                break;
            }
            default:
                layer_cfg_results[0][j] |= CLIENT_USE_PRIMARY;
                break;
            }
        }
    }

    if (!success) {
        layer_cfg_results[0][0] = CLIENT_MERGE_BASE;
        for (unsigned i = 1; i < display_configs[0]->layer_count; i++) {
            layer_cfg_results[0][i] = CLIENT_MERGE_SRC;
        }
    }
    return CONFIG_DISPLAY_OK;
}

// part of ZX_PROTOCOL_DISPLAY_CONTROLLER_IMPL ops
void Mt8167sDisplay::DisplayControllerImplApplyConfiguration(
    const display_config_t** display_configs, size_t display_count) {
    ZX_DEBUG_ASSERT(display_configs);
    fbl::AutoLock lock(&display_lock_);
    auto* config = display_configs[0];
    if (display_count == 1 && config->layer_count) {
        // First stop the overlay engine, followed by the DISP RDMA Engine
        ovl_->Stop();
        disp_rdma_->Stop();
        for (size_t j = 0; j < config->layer_count; j++) {
            const primary_layer_t& layer = config->layer_list[j]->cfg.primary;
            zx_paddr_t addr = reinterpret_cast<zx_paddr_t>(layer.image.handle);
            // Build the overlay configuration. For now we only provide format and address.
            OvlConfig cfg;
            cfg.paddr = addr;
            cfg.format = layer.image.pixel_format;
            cfg.alpha_mode = layer.alpha_mode;
            cfg.alpha_val = layer.alpha_layer_val;
            cfg.src_frame = layer.src_frame;
            cfg.dest_frame = layer.dest_frame;
            cfg.pitch = DisplayControllerImplComputeLinearStride(layer.image.width, cfg.format) *
                        ZX_PIXEL_FORMAT_BYTES(cfg.format);
            cfg.transform = layer.transform_mode;
            ovl_->Config(static_cast<uint8_t>(j), cfg);
        }
        // All configurations are done. Re-start the engine
        disp_rdma_->Start();
        ovl_->Start();
    } else {
        ovl_->Restart();
        disp_rdma_->Restart();
    }
}

// part of ZX_PROTOCOL_DISPLAY_CONTROLLER_IMPL ops
zx_status_t Mt8167sDisplay::DisplayControllerImplAllocateVmo(uint64_t size, zx::vmo* vmo_out) {
    return zx::vmo::create_contiguous(bti_, size, 0, vmo_out);
}

int Mt8167sDisplay::VSyncThread() {
    zx_status_t status;
    while (1) {
        // clear interrupt source
        ovl_->ClearIrq();
        zx::time timestamp;
        status = vsync_irq_.wait(&timestamp);
        if (status != ZX_OK) {
            DISP_ERROR("VSync Interrupt wait failed\n");
            break;
        }
        fbl::AutoLock lock(&display_lock_);
        if (!ovl_->IsValidIrq()) {
            DISP_SPEW("Spurious Interrupt\n");
            continue;
        }
        uint64_t handles[kMaxLayer];
        size_t handle_count = 0;
        // For all 4 layers supported,obtain the handle for that layer and clear it since
        // we are done applying the new configuration to that layer.
        for (uint8_t i = 0; i < kMaxLayer; i++) {
            if (ovl_->IsLayerActive(i)) {
                handles[handle_count++] = ovl_->GetLayerHandle(i);
                ovl_->ClearLayer(i);
            }
        }
        if (dc_intf_.is_valid()) {
            dc_intf_.OnDisplayVsync(kDisplayId, timestamp.get(), handles, handle_count);
        }
    }
    return ZX_OK;
}

zx_status_t Mt8167sDisplay::DisplaySubsystemInit() {

    // Create and initialize ovl object
    fbl::AllocChecker ac;
    ovl_ = fbl::make_unique_checked<mt8167s_display::Ovl>(&ac, height_, width_);
    if (!ac.check()) {
        return ZX_ERR_NO_MEMORY;
    }
    // Initialize ovl object
    auto status = ovl_->Init(parent_);
    if (status != ZX_OK) {
        DISP_ERROR("Could not initialize OVL object\n");
        return status;
    }

    // Create and initialize Display RDMA object
    disp_rdma_ = fbl::make_unique_checked<mt8167s_display::DispRdma>(&ac, height_, width_);
    if (!ac.check()) {
        return ZX_ERR_NO_MEMORY;
    }
    // Initialize disp rdma object
    status = disp_rdma_->Init(parent_);
    if (status != ZX_OK) {
        DISP_ERROR("Could not initialize DISP RDMA object\n");
        return status;
    }

    // Reset and start the various subsytems. Order matters
    ovl_->Reset();
    disp_rdma_->Reset();
    // TODO(payamm): configuring the display RDMA engine does take into account height and width
    // of the display destination frame. However, it is not clear right now how to program
    // these if various layers have different destination dimensions. For now, we will configure
    // the display rdma to the display's height and width. However, this may need fine-tuning later
    // on.
    disp_rdma_->Config();
    ovl_->Start();
    disp_rdma_->Start();
    return ZX_OK;
}

void Mt8167sDisplay::Shutdown() {
    vsync_irq_.destroy();
    thrd_join(vsync_thread_, nullptr);
}

void Mt8167sDisplay::DdkUnbind() {
    Shutdown();
    DdkRemove();
}
void Mt8167sDisplay::DdkRelease() {
    delete this;
}

zx_status_t Mt8167sDisplay::Bind() {

    zx_status_t status = device_get_protocol(parent_, ZX_PROTOCOL_PDEV, &pdev_);
    if (status != ZX_OK) {
        DISP_ERROR("Could not get parent protocol\n");
        return status;
    }

    status = pdev_get_bti(&pdev_, 0, bti_.reset_and_get_address());
    if (status != ZX_OK) {
        DISP_ERROR("Could not get BTI handle\n");
        return status;
    }

    // Map VSync Interrupt
    status = pdev_map_interrupt(&pdev_, 0, vsync_irq_.reset_and_get_address());
    if (status != ZX_OK) {
        DISP_ERROR("Could not map vsync Interruptn");
        return status;
    }

    // Initialize Display Subsystem
    status = DisplaySubsystemInit();
    if (status != ZX_OK) {
        DISP_ERROR("Could not initialize Display Subsystem\n");
        return status;
    }

    auto start_thread = [](void* arg) { return static_cast<Mt8167sDisplay*>(arg)->VSyncThread(); };
    status = thrd_create_with_name(&vsync_thread_, start_thread, this, "vsync_thread");
    if (status != ZX_OK) {
        DISP_ERROR("Could not create vsync_thread\n");
        return status;
    }

    list_initialize(&imported_images_);

    status = DdkAdd("mt8167s-display");
    if (status != ZX_OK) {
        DISP_ERROR("coud not add device\n");
        Shutdown();
        return status;
    }

    return ZX_OK;
}

// main bind function called from dev manager
zx_status_t display_bind(void* ctx, zx_device_t* parent) {
    fbl::AllocChecker ac;
    auto dev = fbl::make_unique_checked<mt8167s_display::Mt8167sDisplay>(&ac, parent,
                                                                         DISPLAY_WIDTH,
                                                                         DISPLAY_HEIGHT);
    if (!ac.check()) {
        DISP_ERROR("no bind\n");
        return ZX_ERR_NO_MEMORY;
    }
    zx_status_t status = dev->Bind();
    if (status == ZX_OK) {
        __UNUSED auto ptr = dev.release();
    }
    return status;
}

static zx_driver_ops_t display_ops = [](){
    zx_driver_ops_t ops;
    ops.version = DRIVER_OPS_VERSION;
    ops.bind = display_bind;
    return ops;
}();

} // namespace mt8167s_display

// clang-format off
ZIRCON_DRIVER_BEGIN(mt8167s_display, mt8167s_display::display_ops, "zircon", "0.1", 3)
    BI_ABORT_IF(NE, BIND_PROTOCOL, ZX_PROTOCOL_PDEV),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_VID, PDEV_VID_MEDIATEK),
    BI_MATCH_IF(EQ, BIND_PLATFORM_DEV_DID, PDEV_DID_MEDIATEK_DISPLAY),
ZIRCON_DRIVER_END(mt8167s_display)
