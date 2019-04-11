# Copyright 2018 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SO_INSTALL_NAME := driver/test/sysdev.so

MODULE_SRCS := \
  $(LOCAL_DIR)/bind.c \
  $(LOCAL_DIR)/sysdev.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/ddk \
    system/ulib/ddktl \
    system/ulib/fbl \
    system/ulib/zx \
    system/ulib/zxcpp \

MODULE_LIBS := \
    system/ulib/fdio \
    system/ulib/driver \
    system/ulib/zircon \
    system/ulib/c \

include make/module.mk
