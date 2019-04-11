// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <ddk/device.h>
#include <fuchsia/hardware/rtc/c/fidl.h>
#include <zircon/compiler.h>

__BEGIN_CDECLS

// Basic validation that |rtc| has reasonable values. Does not check leap year.
bool rtc_is_invalid(const fuchsia_hardware_rtc_Time* rtc);

// Computes seconds (Unix epoch) to |rtc|. Does not validate. Does not handle times
// earlier than 2000/1/1T00:00:00.
uint64_t seconds_since_epoch(const fuchsia_hardware_rtc_Time* rtc);
void seconds_to_rtc(uint64_t seconds, fuchsia_hardware_rtc_Time* rtc);

// Validates and cleans what an RTC device |dev| returns. If the device returns
// nonsensical values, it sets |rtc| to  2018/1/1T00:00:00.
void sanitize_rtc(void* ctx, fuchsia_hardware_rtc_Time* rtc,
                  zx_status_t (*rtc_get)(void*, fuchsia_hardware_rtc_Time*),
                  zx_status_t (*rtc_set)(void*, const fuchsia_hardware_rtc_Time*));

// Utility binary-coded-decimal routines.
uint8_t to_bcd(uint8_t binary);
uint8_t from_bcd(uint8_t bcd);

__END_CDECLS
