# Copyright 2017 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS += \
    $(LOCAL_DIR)/mmc.c \
    $(LOCAL_DIR)/ops.c \
    $(LOCAL_DIR)/sd.c \
    $(LOCAL_DIR)/sdmmc.c \
    $(LOCAL_DIR)/sdio.c \
    $(LOCAL_DIR)/sdio-interrupts.c \

MODULE_STATIC_LIBS := \
    system/ulib/ddk \
    system/ulib/sync \
    system/ulib/pretty \

MODULE_LIBS := system/ulib/driver \
    system/ulib/c \
    system/ulib/zircon \
    system/ulib/fdio \

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-block \
    system/banjo/ddk-protocol-platform-device \
    system/banjo/ddk-protocol-sdio \
    system/banjo/ddk-protocol-sdmmc \

ifeq ($(call TOBOOL,$(ENABLE_DRIVER_TRACING)),true)
MODULE_STATIC_LIBS += system/ulib/trace.driver
endif
MODULE_HEADER_DEPS += system/ulib/trace system/ulib/trace-engine

include make/module.mk
