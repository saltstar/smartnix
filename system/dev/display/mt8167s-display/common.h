// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <hwreg/mmio.h>

#define DISP_ERROR(fmt, ...) zxlogf(ERROR, "[%s %d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#define DISP_INFO(fmt, ...) zxlogf(INFO, "[%s %d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#define DISP_SPEW(fmt, ...) zxlogf(SPEW, "[%s %d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#define DISP_TRACE zxlogf(INFO, "[%s %d]\n", __func__, __LINE__)

// Should match display_mmios table in board driver
enum {
    MMIO_DISP_OVL,
};

constexpr uint8_t PANEL_DISPLAY_ID = 1;

// mt8167s_ref Display dimension
constexpr uint32_t DISPLAY_WIDTH = 720;
constexpr uint32_t DISPLAY_HEIGHT = 1280;
