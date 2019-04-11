# Copyright 2018 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := usertest

MODULE_SRCS += \
    $(LOCAL_DIR)/main.cpp

MODULE_NAME := pbus-test

MODULE_STATIC_LIBS := \
	system/ulib/ddk \
	system/ulib/fbl \
	system/ulib/libzbi \
	system/ulib/zx \
	system/ulib/zxcpp \

MODULE_LIBS := \
	system/ulib/c \
	system/ulib/devmgr-integration-test \
	system/ulib/devmgr-launcher \
	system/ulib/fdio \
	system/ulib/unittest \
	system/ulib/zircon \

include make/module.mk
