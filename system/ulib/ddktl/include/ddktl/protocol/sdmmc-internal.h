// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// WARNING: THIS FILE IS MACHINE GENERATED. DO NOT EDIT.
//          MODIFY system/fidl/protocols/sdmmc.banjo INSTEAD.

#pragma once

#include <ddk/protocol/sdmmc.h>
#include <fbl/type_support.h>

namespace ddk {
namespace internal {

DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_sdmmc_protocol_host_info, SdmmcHostInfo,
                                     zx_status_t (C::*)(sdmmc_host_info_t* out_info));
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_sdmmc_protocol_set_signal_voltage, SdmmcSetSignalVoltage,
                                     zx_status_t (C::*)(sdmmc_voltage_t voltage));
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_sdmmc_protocol_set_bus_width, SdmmcSetBusWidth,
                                     zx_status_t (C::*)(sdmmc_bus_width_t bus_width));
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_sdmmc_protocol_set_bus_freq, SdmmcSetBusFreq,
                                     zx_status_t (C::*)(uint32_t bus_freq));
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_sdmmc_protocol_set_timing, SdmmcSetTiming,
                                     zx_status_t (C::*)(sdmmc_timing_t timing));
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_sdmmc_protocol_hw_reset, SdmmcHwReset, void (C::*)());
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_sdmmc_protocol_perform_tuning, SdmmcPerformTuning,
                                     zx_status_t (C::*)(uint32_t cmd_idx));
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_sdmmc_protocol_request, SdmmcRequest,
                                     zx_status_t (C::*)(sdmmc_req_t* req));

template <typename D>
constexpr void CheckSdmmcProtocolSubclass() {
    static_assert(internal::has_sdmmc_protocol_host_info<D>::value,
                  "SdmmcProtocol subclasses must implement "
                  "zx_status_t SdmmcHostInfo(sdmmc_host_info_t* out_info");
    static_assert(internal::has_sdmmc_protocol_set_signal_voltage<D>::value,
                  "SdmmcProtocol subclasses must implement "
                  "zx_status_t SdmmcSetSignalVoltage(sdmmc_voltage_t voltage");
    static_assert(internal::has_sdmmc_protocol_set_bus_width<D>::value,
                  "SdmmcProtocol subclasses must implement "
                  "zx_status_t SdmmcSetBusWidth(sdmmc_bus_width_t bus_width");
    static_assert(internal::has_sdmmc_protocol_set_bus_freq<D>::value,
                  "SdmmcProtocol subclasses must implement "
                  "zx_status_t SdmmcSetBusFreq(uint32_t bus_freq");
    static_assert(internal::has_sdmmc_protocol_set_timing<D>::value,
                  "SdmmcProtocol subclasses must implement "
                  "zx_status_t SdmmcSetTiming(sdmmc_timing_t timing");
    static_assert(internal::has_sdmmc_protocol_hw_reset<D>::value,
                  "SdmmcProtocol subclasses must implement "
                  "void SdmmcHwReset(");
    static_assert(internal::has_sdmmc_protocol_perform_tuning<D>::value,
                  "SdmmcProtocol subclasses must implement "
                  "zx_status_t SdmmcPerformTuning(uint32_t cmd_idx");
    static_assert(internal::has_sdmmc_protocol_request<D>::value,
                  "SdmmcProtocol subclasses must implement "
                  "zx_status_t SdmmcRequest(sdmmc_req_t* req");
}

} // namespace internal
} // namespace ddk
