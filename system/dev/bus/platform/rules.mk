# Copyright 2016 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_NAME := platform-bus

MODULE_SRCS := \
    $(LOCAL_DIR)/device-resources.cpp \
    $(LOCAL_DIR)/platform-bus.cpp \
    $(LOCAL_DIR)/platform-device.cpp \
    $(LOCAL_DIR)/platform-protocol-device.cpp \
    $(LOCAL_DIR)/platform-i2c.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/ddk \
    system/ulib/ddktl \
    system/ulib/fbl \
    system/ulib/sync \
    system/ulib/zx \
    system/ulib/zxcpp \

MODULE_LIBS := \
    system/ulib/driver \
    system/ulib/zircon \
    system/ulib/c \

MODULE_FIDL_LIBS := \
    system/fidl/fuchsia-sysinfo \

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-clk \
    system/banjo/ddk-protocol-gpio \
    system/banjo/ddk-protocol-gpioimpl \
    system/banjo/ddk-protocol-i2c \
    system/banjo/ddk-protocol-i2cimpl \
    system/banjo/ddk-protocol-iommu \
    system/banjo/ddk-protocol-platform-bus \
    system/banjo/ddk-protocol-platform-device \
    system/banjo/ddk-protocol-platform-proxy \

include make/module.mk

MODULE := $(LOCAL_DIR).proxy

MODULE_TYPE := driver

MODULE_NAME := platform-bus.proxy

MODULE_SRCS := \
    $(LOCAL_DIR)/platform-proxy.cpp \
    $(LOCAL_DIR)/platform-proxy-bind.c \
    $(LOCAL_DIR)/platform-proxy-client.cpp \
    $(LOCAL_DIR)/platform-proxy-device.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/ddk \
    system/ulib/ddktl \
    system/ulib/fbl \
    system/ulib/sync \
    system/ulib/zx \
    system/ulib/zxcpp \

MODULE_LIBS := \
    system/ulib/driver \
    system/ulib/zircon \
    system/ulib/c \

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-clk \
    system/banjo/ddk-protocol-gpio \
    system/banjo/ddk-protocol-gpioimpl \
    system/banjo/ddk-protocol-i2c \
    system/banjo/ddk-protocol-i2cimpl \
    system/banjo/ddk-protocol-platform-bus \
    system/banjo/ddk-protocol-platform-device \
    system/banjo/ddk-protocol-platform-proxy \

include make/module.mk
