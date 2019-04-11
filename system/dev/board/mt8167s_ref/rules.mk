# Copyright 2018 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS := \
    $(LOCAL_DIR)/mt8167.cpp \
    $(LOCAL_DIR)/mt8167-emmc.cpp \
    $(LOCAL_DIR)/mt8167-sdio.cpp \
    $(LOCAL_DIR)/mt8167-soc.cpp \
    $(LOCAL_DIR)/mt8167-sysmem.cpp \
    $(LOCAL_DIR)/mt8167-gpio.cpp \
    $(LOCAL_DIR)/mt8167-gpu.cpp \
    $(LOCAL_DIR)/mt8167-display.cpp \
    $(LOCAL_DIR)/mt8167-i2c.cpp \
    $(LOCAL_DIR)/mt8167-buttons.cpp \
    $(LOCAL_DIR)/mt8167-clk.cpp \
    $(LOCAL_DIR)/mt8167-usb.cpp \
    $(LOCAL_DIR)/mt8167-thermal.cpp \
    $(LOCAL_DIR)/mt8167-touch.cpp \
    $(LOCAL_DIR)/mt8167-sensors.cpp \
    $(LOCAL_DIR)/mt8167-backlight.cpp \
    $(LOCAL_DIR)/mt8167-audio.cpp \

MODULE_STATIC_LIBS := \
    system/dev/lib/focaltech \
    system/dev/lib/mt8167 \
    system/ulib/ddk \
    system/ulib/ddktl \
    system/ulib/fbl \
    system/ulib/hwreg \
    system/ulib/zx \
    system/ulib/zxcpp \

MODULE_LIBS := \
    system/ulib/driver \
    system/ulib/zircon \
    system/ulib/c \

MODULE_FIDL_LIBS := system/fidl/fuchsia-hardware-usb-peripheral

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-gpio \
    system/banjo/ddk-protocol-gpioimpl \
    system/banjo/ddk-protocol-platform-bus \
    system/banjo/ddk-protocol-platform-device \
    system/banjo/ddk-protocol-scpi \

include make/module.mk
