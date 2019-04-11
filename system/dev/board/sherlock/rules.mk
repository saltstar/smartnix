# Copyright 2018 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS := \
    $(LOCAL_DIR)/bind.c \
    $(LOCAL_DIR)/sherlock.cpp \
    $(LOCAL_DIR)/sherlock-camera.cpp \
    $(LOCAL_DIR)/sherlock-canvas.cpp \
    $(LOCAL_DIR)/sherlock-clk.cpp \
    $(LOCAL_DIR)/sherlock-emmc.cpp \
    $(LOCAL_DIR)/sherlock-sdio.cpp \
    $(LOCAL_DIR)/sherlock-sysmem.cpp \
    $(LOCAL_DIR)/sherlock-gpio.cpp \
    $(LOCAL_DIR)/sherlock-i2c.cpp \
    $(LOCAL_DIR)/sherlock-mali.cpp \
    $(LOCAL_DIR)/sherlock-bluetooth.cpp \
    $(LOCAL_DIR)/sherlock-usb.cpp \
    $(LOCAL_DIR)/sherlock-video.cpp \
    $(LOCAL_DIR)/sherlock-backlight.cpp \
    $(LOCAL_DIR)/sherlock-buttons.cpp \
    $(LOCAL_DIR)/sherlock-display.cpp \
    $(LOCAL_DIR)/sherlock-audio.cpp \

MODULE_STATIC_LIBS := \
    system/dev/lib/amlogic \
    system/dev/lib/broadcom \
    system/ulib/ddk \
    system/ulib/ddktl \
    system/ulib/fbl \
    system/ulib/zx \
    system/ulib/zxcpp \

MODULE_LIBS := \
    system/ulib/driver \
    system/ulib/zircon \
    system/ulib/c \

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-gpio \
    system/banjo/ddk-protocol-gpioimpl \
    system/banjo/ddk-protocol-iommu \
    system/banjo/ddk-protocol-platform-bus \
    system/banjo/ddk-protocol-platform-device \
    system/banjo/ddk-protocol-serial \

include make/module.mk
