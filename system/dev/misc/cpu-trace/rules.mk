# Copyright 2017 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO(dje): This can either be removed (we only support arm,x86)
# or even add arm to the filter-out list when arm supported is added.
ifeq ($(filter-out $(ARCH),x86),)

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS := \
    $(LOCAL_DIR)/cpu-trace.cpp

ifeq ($(ARCH),x86)
MODULE_SRCS += \
    $(LOCAL_DIR)/intel-pm.cpp \
    $(LOCAL_DIR)/intel-pt.cpp
endif

MODULE_STATIC_LIBS := \
    system/ulib/ddk \
    system/ulib/ddktl \
    system/ulib/fbl \
    system/ulib/zircon-internal \
    system/ulib/zx \
    system/ulib/zxcpp

MODULE_LIBS := \
    system/ulib/driver \
    system/ulib/zircon \
    system/ulib/c

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-platform-device \

include make/module.mk

endif
