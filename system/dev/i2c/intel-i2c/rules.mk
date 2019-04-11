# Copyright 2018 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS += \
    $(LOCAL_DIR)/intel-i2c-controller.c \
    $(LOCAL_DIR)/intel-i2c-slave.c \

MODULE_LIBS := \
    system/ulib/driver \
    system/ulib/zircon \
    system/ulib/c \

MODULE_STATIC_LIBS := \
    system/ulib/ddk \
    system/ulib/fidl \
    system/ulib/sync \

MODULE_FIDL_LIBS := system/fidl/fuchsia-hardware-i2c

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-i2c \
    system/banjo/ddk-protocol-i2cimpl \
    system/banjo/ddk-protocol-pci \

include make/module.mk
