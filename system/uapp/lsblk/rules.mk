# Copyright 2016 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := core

MODULE_SRCS += \
    $(LOCAL_DIR)/main.c

MODULE_STATIC_LIBS := \
    system/ulib/fbl \
    system/ulib/gpt \
    system/ulib/pretty \
    system/ulib/zx \
    system/ulib/zxcpp \
    third_party/ulib/cksum \

MODULE_LIBS := system/ulib/zircon system/ulib/fdio system/ulib/c

MODULE_FIDL_LIBS := system/fidl/fuchsia-hardware-skipblock

include make/module.mk
