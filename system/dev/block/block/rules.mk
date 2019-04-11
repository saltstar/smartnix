# Copyright 2017 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS := \
    $(LOCAL_DIR)/block.cpp \
    $(LOCAL_DIR)/server.cpp \
    $(LOCAL_DIR)/server-manager.cpp \
    $(LOCAL_DIR)/txn-group.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/ddk \
    system/ulib/ddktl \
    system/ulib/fbl \
    system/ulib/fzl \
    system/ulib/sync \
    system/ulib/zx \
    system/ulib/zxcpp \

MODULE_LIBS := system/ulib/c system/ulib/driver system/ulib/zircon

MODULE_BANJO_LIBS := \
    system/banjo/ddk-protocol-block \
    system/banjo/ddk-protocol-block-partition \
    system/banjo/ddk-protocol-block-volume \

include make/module.mk
