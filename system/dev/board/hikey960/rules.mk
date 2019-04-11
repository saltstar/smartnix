# Copyright 2017 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS += \
    $(LOCAL_DIR)/hikey960.c \
    $(LOCAL_DIR)/hikey960-devices.c \
    $(LOCAL_DIR)/hikey960-i2c.c \
    $(LOCAL_DIR)/hikey960-usb.c \
    $(LOCAL_DIR)/hikey960-sysmem.c \

MODULE_STATIC_LIBS := \
    system/dev/lib/hi3660 \
    system/dev/gpio/pl061 \
    system/ulib/ddk \
    system/ulib/sync

MODULE_LIBS := system/ulib/driver system/ulib/c system/ulib/zircon

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-gpio \
    system/banjo/ddk-protocol-gpioimpl \
    system/banjo/ddk-protocol-i2c \
    system/banjo/ddk-protocol-iommu \
    system/banjo/ddk-protocol-platform-bus \
    system/banjo/ddk-protocol-platform-device \
    system/banjo/ddk-protocol-usb-modeswitch \

include make/module.mk

MODULE := $(LOCAL_DIR).i2c-test.c

MODULE_NAME := hi3660-i2c-test

MODULE_TYPE := driver

MODULE_SRCS += \
    $(LOCAL_DIR)/hikey960-i2c-test.c \

MODULE_STATIC_LIBS := system/ulib/ddk system/ulib/sync

MODULE_LIBS := system/ulib/driver system/ulib/c system/ulib/zircon

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-i2c \
    system/banjo/ddk-protocol-platform-device \

include make/module.mk
