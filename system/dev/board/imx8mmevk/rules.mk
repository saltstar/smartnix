# Copyright 2018 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS += \
    $(LOCAL_DIR)/binding.c \
    $(LOCAL_DIR)/imx8mmevk.cpp \
    $(LOCAL_DIR)/imx8mmevk-sysmem.cpp \
    $(LOCAL_DIR)/imx8mmevk-gpio.cpp \

MODULE_STATIC_LIBS := \
    system/dev/lib/imx8m \
    system/ulib/ddk \
    system/ulib/ddktl \
    system/ulib/fbl \
    system/ulib/zx \
    system/ulib/zxcpp \

MODULE_LIBS := \
    system/ulib/driver \
    system/ulib/c \
    system/ulib/zircon \

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-gpio \
    system/banjo/ddk-protocol-gpioimpl \
    system/banjo/ddk-protocol-iommu \
    system/banjo/ddk-protocol-platform-bus \
    system/banjo/ddk-protocol-platform-device \

include make/module.mk
