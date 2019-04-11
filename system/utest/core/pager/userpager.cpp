// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <atomic>
#include <fbl/algorithm.h>
#include <fbl/array.h>
#include <lib/zx/vmar.h>
#include <string.h>
#include <zircon/process.h>
#include <zircon/syscalls.h>
#include <zircon/syscalls/port.h>

#include "userpager.h"

namespace pager_tests {

bool Vmo::CheckVmar(uint64_t offset, uint64_t len, const void* expected) {
    ZX_ASSERT((offset + len) <= (size_ / ZX_PAGE_SIZE));

    len *= ZX_PAGE_SIZE;
    offset *= ZX_PAGE_SIZE;

    for (uint64_t i = offset / sizeof(uint64_t); i < (offset + len) / sizeof(uint64_t); i++) {
        uint64_t actual_val = base_[i];
        // Make sure we deterministically read from the vmar before reading the
        // expected value, in case things get remapped.
        std::atomic_thread_fence(std::memory_order_seq_cst);
        uint64_t expected_val = expected
                                    ? static_cast<const uint64_t*>(expected)[i]
                                    : base_val_ + i;
        if (actual_val != expected_val) {
            return false;
        }
    }
    return true;
}

bool Vmo::OpRange(uint32_t op, uint64_t offset, uint64_t len) {
    return vmo_.op_range(op, offset * ZX_PAGE_SIZE, len * ZX_PAGE_SIZE, nullptr, 0) == ZX_OK;
}

void Vmo::GenerateBufferContents(void* dest_buffer, uint64_t len, uint64_t paged_vmo_offset) {
    len *= ZX_PAGE_SIZE;
    paged_vmo_offset *= ZX_PAGE_SIZE;
    auto buf = static_cast<uint64_t*>(dest_buffer);
    for (uint64_t idx = 0; idx < len / sizeof(uint64_t); idx++) {
        buf[idx] = base_val_ + (paged_vmo_offset / sizeof(uint64_t)) + idx;
    }
}

fbl::unique_ptr<Vmo> Vmo::Clone() {
    zx::vmo clone;
    if (vmo_.clone(ZX_VMO_CLONE_COPY_ON_WRITE, 0, size_, &clone) != ZX_OK) {
        return nullptr;
    }

    zx_vaddr_t addr;
    if (zx::vmar::root_self()->map(
            0, clone, 0, size_, ZX_VM_PERM_READ | ZX_VM_PERM_WRITE, &addr) != ZX_OK) {
        return nullptr;
    }

    return fbl::unique_ptr<Vmo>(new Vmo(
        std::move(clone), size_, reinterpret_cast<uint64_t*>(addr), addr, base_val_));
}

UserPager::~UserPager() {
    while (!vmos_.is_empty()) {
        auto vmo = vmos_.pop_front();
        zx::vmar::root_self()->unmap(vmo->base_addr_, vmo->size_);
    }
    zx_handle_close(port_);
    zx_handle_close(pager_);
}

bool UserPager::Init() {
    return zx_pager_create(0, &pager_) == ZX_OK && zx_port_create(0, &port_) == ZX_OK;
}

bool UserPager::CreateVmo(uint64_t size, Vmo** vmo_out) {
    zx::vmo vmo;
    size *= ZX_PAGE_SIZE;
    if (zx_pager_create_vmo(pager_, 0, port_, next_base_, size,
                            vmo.reset_and_get_address()) != ZX_OK) {
        return false;
    }

    zx_vaddr_t addr;
    if (zx::vmar::root_self()->map(
            0, vmo, 0, size, ZX_VM_PERM_READ | ZX_VM_PERM_WRITE, &addr) != ZX_OK) {
        return false;
    }

    auto paged_vmo = fbl::unique_ptr<Vmo>(new Vmo(
        std::move(vmo), size, reinterpret_cast<uint64_t*>(addr), addr, next_base_));

    next_base_ += (size / sizeof(uint64_t));

    *vmo_out = paged_vmo.get();
    vmos_.push_back(std::move(paged_vmo));

    return true;
}

bool UserPager::UnmapVmo(Vmo* vmo) {
    return zx::vmar::root_self()->unmap(vmo->base_addr_, vmo->size_) == ZX_OK;
}

bool UserPager::ReplaceVmo(Vmo* vmo, zx::vmo* old_vmo) {
    zx::vmo new_vmo;
    zx_status_t s;
    if ((s = zx_pager_create_vmo(pager_, 0, port_, next_base_, vmo->size_,
                                 new_vmo.reset_and_get_address())) != ZX_OK) {
        return false;
    }

    zx_info_vmar_t info;
    uint64_t a1, a2;
    if (zx::vmar::root_self()->get_info(ZX_INFO_VMAR, &info, sizeof(info), &a1, &a2) != ZX_OK) {
        return false;
    }

    zx_vaddr_t addr;
    if ((s = zx::vmar::root_self()->map(
             vmo->base_addr_ - info.base, new_vmo, 0, vmo->size_,
             ZX_VM_PERM_READ | ZX_VM_PERM_WRITE | ZX_VM_SPECIFIC_OVERWRITE, &addr)) != ZX_OK) {
        return false;
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);

    vmo->base_val_ = next_base_;
    next_base_ += (vmo->size_ / sizeof(uint64_t));

    *old_vmo = std::move(vmo->vmo_);
    vmo->vmo_ = std::move(new_vmo);

    return true;
}

bool UserPager::DetachVmo(Vmo* vmo) {
    return zx_pager_detach_vmo(pager_, vmo->vmo_.get()) == ZX_OK;
}

void UserPager::ReleaseVmo(Vmo* vmo) {
    zx::vmar::root_self()->unmap(vmo->base_addr_, vmo->size_);
    vmos_.erase(*vmo);
}

bool UserPager::WaitForPageRead(
    Vmo* vmo, uint64_t offset, uint64_t length, zx_time_t deadline) {
    zx_packet_page_request_t req = {};
    req.command = ZX_PAGER_VMO_READ;
    req.offset = offset * ZX_PAGE_SIZE;
    req.length = length * ZX_PAGE_SIZE;
    return WaitForRequest(vmo->base_val_, req, deadline);
}

bool UserPager::WaitForPageComplete(uint64_t key, zx_time_t deadline) {
    zx_packet_page_request_t req = {};
    req.command = ZX_PAGER_VMO_COMPLETE;
    return WaitForRequest(key, req, deadline);
}

bool UserPager::WaitForRequest(
    uint64_t key, const zx_packet_page_request& req, zx_time_t deadline) {
    zx_port_packet_t expected = {
        .key = key,
        .type = ZX_PKT_TYPE_PAGE_REQUEST,
        .status = ZX_OK,
        .page_request = req,
    };

    return WaitForRequest([expected](const zx_port_packet& actual) -> bool {
        ZX_ASSERT(expected.type == ZX_PKT_TYPE_PAGE_REQUEST);
        if (expected.key != actual.key || ZX_PKT_TYPE_PAGE_REQUEST != actual.type) {
            return false;
        }
        return memcmp(&expected.page_request, &actual.page_request,
                      sizeof(zx_packet_page_request_t)) == 0;
    }, deadline);
}

bool UserPager::GetPageReadRequest(
    Vmo* vmo, zx_time_t deadline, uint64_t* offset, uint64_t* length) {
    return WaitForRequest([vmo, offset, length](const zx_port_packet& packet) -> bool {
        if (packet.key == vmo->base_val_ && packet.type == ZX_PKT_TYPE_PAGE_REQUEST
                && packet.page_request.command == ZX_PAGER_VMO_READ) {
            *offset = packet.page_request.offset / ZX_PAGE_SIZE;
            *length = packet.page_request.length / ZX_PAGE_SIZE;
            return true;
        }
        return false;
    }, deadline);
}

bool UserPager::WaitForRequest(fbl::Function<bool(const zx_port_packet_t& packet)> cmp_fn,
                               zx_time_t deadline) {
    for (auto& iter : requests_) {
        if (cmp_fn(iter.req)) {
            requests_.erase(iter);
            return true;
        }
    }

    zx_time_t now = zx_clock_get_monotonic();
    if (deadline < now) {
        deadline = now;
    }
    while (now <= deadline) {
        zx_port_packet_t actual_packet;
        // TODO: this can block forever if the thread that's
        // supposed to generate the request unexpectedly dies.
        zx_status_t status = zx_port_wait(port_, deadline, &actual_packet);
        if (status == ZX_OK) {
            if (cmp_fn(actual_packet)) {
                return true;
            }

            auto req = fbl::make_unique<request>();
            req->req = actual_packet;
            requests_.push_front(std::move(req));
        } else {
            // Don't advance now on success, to make sure we read any pending requests
            now = zx_clock_get_monotonic();
        }
    }
    return false;
}

bool UserPager::SupplyPages(Vmo* paged_vmo, uint64_t dest_offset,
                            uint64_t length, uint64_t src_offset) {
    zx::vmo vmo;
    if (zx::vmo::create((length + src_offset) * ZX_PAGE_SIZE, 0, &vmo) != ZX_OK) {
        return false;
    }

    uint64_t cur = 0;
    while (cur < length) {
        uint8_t data[ZX_PAGE_SIZE];
        paged_vmo->GenerateBufferContents(data, 1, dest_offset + cur);

        if (vmo.write(data, (src_offset + cur) * ZX_PAGE_SIZE, ZX_PAGE_SIZE) != ZX_OK) {
            return false;
        }

        cur++;
    }

    return SupplyPages(paged_vmo, dest_offset, length, std::move(vmo), src_offset);
}

bool UserPager::SupplyPages(Vmo* paged_vmo, uint64_t dest_offset,
                            uint64_t length, zx::vmo src, uint64_t src_offset) {
    zx_status_t status;
    if ((status = zx_pager_supply_pages(pager_, paged_vmo->vmo_.get(),
                                        dest_offset * ZX_PAGE_SIZE, length * ZX_PAGE_SIZE,
                                        src.get(), src_offset * ZX_PAGE_SIZE)) != ZX_OK) {
        return false;
    }
    return true;
}

} // namespace pager_tests
