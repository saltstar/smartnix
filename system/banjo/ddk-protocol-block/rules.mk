# Copyright 2018 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := banjo

MODULE_PACKAGE := banjo

MODULE_BANJO_LIBRARY := ddk.protocol.block

MODULE_BANJO_NAME := block

MODULE_BANJO_DEPS := \
    system/banjo/zircon-device-block \

MODULE_SRCS += $(LOCAL_DIR)/block.banjo

include make/module.mk
