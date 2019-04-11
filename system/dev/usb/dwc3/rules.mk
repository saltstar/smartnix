# Copyright 2016 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS += \
    $(LOCAL_DIR)/dwc3.cpp \
    $(LOCAL_DIR)/dwc3-commands.cpp \
    $(LOCAL_DIR)/dwc3-endpoints.cpp \
    $(LOCAL_DIR)/dwc3-ep0.cpp \
    $(LOCAL_DIR)/dwc3-events.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/ddk  	 \
    system/ulib/ddktl \
    system/ulib/fbl \
    system/ulib/hwreg \
    system/ulib/sync   \
    system/ulib/pretty \
    system/dev/lib/usb \
    system/ulib/zx \
    system/ulib/zxcpp \

MODULE_LIBS := \
    system/ulib/driver \
    system/ulib/zircon \
    system/ulib/c \

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-platform-device \
    system/banjo/ddk-protocol-usb \
    system/banjo/ddk-protocol-usb-dci \
    system/banjo/ddk-protocol-usb-modeswitch \
    system/banjo/ddk-protocol-usb-request \

include make/module.mk
