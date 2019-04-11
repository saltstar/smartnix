# Copyright 2016 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

# devmgr - core userspace services process
#
MODULE := $(LOCAL_DIR)

MODULE_NAME := devmgr
MODULE_TYPE := userapp
MODULE_GROUP := core

MODULE_SRCS += \
    $(LOCAL_DIR)/devmgr/binding.cpp \
    $(LOCAL_DIR)/devmgr/coordinator.cpp \
    $(LOCAL_DIR)/devmgr/devfs.cpp \
    $(LOCAL_DIR)/devmgr/devhost-loader-service.cpp \
    $(LOCAL_DIR)/devmgr/device.cpp \
    $(LOCAL_DIR)/devmgr/driver.cpp \
    $(LOCAL_DIR)/devmgr/fidl.cpp \
    $(LOCAL_DIR)/devmgr/main.cpp \
    $(LOCAL_DIR)/shared/env.cpp \
    $(LOCAL_DIR)/shared/fdio.cpp \

# userboot supports loading via the dynamic linker, so libc (system/ulib/c)
# can be linked dynamically.  But it doesn't support any means to look
# up other shared libraries, so everything else must be linked statically.

MODULE_FIDL_LIBS := \
    system/fidl/fuchsia-device-manager \
    system/fidl/fuchsia-io \
    system/fidl/fuchsia-mem \

# ddk is needed only for ddk/device.h
MODULE_HEADER_DEPS := \
    system/ulib/bootsvc-protocol \
    system/ulib/ddk \
    system/ulib/devmgr-launcher \
    system/ulib/zircon-internal \

MODULE_STATIC_LIBS := \
    system/ulib/async \
    system/ulib/async-loop \
    system/ulib/async-loop.cpp \
    system/ulib/async.cpp \
    system/ulib/bootdata \
    system/ulib/driver-info \
    system/ulib/fbl \
    system/ulib/fidl \
    system/ulib/fit \
    system/ulib/fs \
    system/ulib/fzl \
    system/ulib/libzbi \
    system/ulib/loader-service \
    system/ulib/memfs \
    system/ulib/sync \
    system/ulib/zx \
    system/ulib/zxcpp \
    third_party/ulib/lz4 \

MODULE_LIBS := \
    system/ulib/async.default \
    system/ulib/c \
    system/ulib/fdio \
    system/ulib/launchpad \
    system/ulib/zircon \

include make/module.mk

MODULE := $(LOCAL_DIR).test

MODULE_NAME := devmgr-test
MODULE_TYPE := usertest
MODULE_USERTEST_GROUP := ddk

MODULE_SRCS += \
    $(LOCAL_DIR)/devmgr/binding.cpp \
    $(LOCAL_DIR)/devmgr/coordinator-test.cpp \
    $(LOCAL_DIR)/devmgr/coordinator.cpp \
    $(LOCAL_DIR)/devmgr/devhost-loader-service.cpp \
    $(LOCAL_DIR)/devmgr/devfs.cpp \
    $(LOCAL_DIR)/devmgr/device.cpp \
    $(LOCAL_DIR)/devmgr/driver.cpp \
    $(LOCAL_DIR)/devmgr/fidl.cpp \
    $(LOCAL_DIR)/devmgr/test-main.cpp \
    $(LOCAL_DIR)/shared/env.cpp \

MODULE_FIDL_LIBS := \
    system/fidl/fuchsia-device-manager \
    system/fidl/fuchsia-io \
    system/fidl/fuchsia-mem \

MODULE_HEADER_DEPS := \
    system/ulib/ddk \
    system/ulib/zircon-internal \

MODULE_STATIC_LIBS := \
    system/ulib/async \
    system/ulib/async-loop \
    system/ulib/async-loop.cpp \
    system/ulib/async.cpp \
    system/ulib/driver-info \
    system/ulib/fidl \
    system/ulib/fit \
    system/ulib/fbl \
    system/ulib/fs \
    system/ulib/fzl \
    system/ulib/libzbi \
    system/ulib/loader-service \
    system/ulib/memfs \
    system/ulib/zx \
    system/ulib/zxcpp \

MODULE_LIBS := \
    system/ulib/async.default \
    system/ulib/c \
    system/ulib/fdio \
    system/ulib/launchpad \
    system/ulib/unittest \
    system/ulib/zircon \

include make/module.mk

# fshost - container for filesystems

MODULE := $(LOCAL_DIR).fshost

MODULE_NAME := fshost
MODULE_TYPE := userapp
MODULE_GROUP := core

MODULE_SRCS := \
    $(LOCAL_DIR)/fshost/block-watcher.cpp \
    $(LOCAL_DIR)/fshost/main.cpp \
    $(LOCAL_DIR)/fshost/vfs-rpc.cpp \
    $(LOCAL_DIR)/shared/env.cpp \
    $(LOCAL_DIR)/shared/fdio.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/memfs.cpp \
    system/ulib/memfs \
    system/ulib/fs \
    system/ulib/loader-service \
    system/ulib/async.cpp \
    system/ulib/async \
    system/ulib/async-loop.cpp \
    system/ulib/async-loop \
    system/ulib/bootdata \
    system/ulib/bootfs \
    system/ulib/fbl \
    system/ulib/fit \
    system/ulib/gpt \
    system/ulib/sync \
    system/ulib/trace \
    system/ulib/zx \
    system/ulib/zxcpp \
    third_party/ulib/cksum \
    third_party/ulib/lz4 \

MODULE_LIBS := \
    system/ulib/async.default \
    system/ulib/launchpad \
    system/ulib/fdio \
    system/ulib/fs-management \
    system/ulib/trace-engine \
    system/ulib/zircon \
    system/ulib/c

MODULE_FIDL_LIBS := \
    system/fidl/fuchsia-hardware-ramdisk \
    system/fidl/fuchsia-io \
    system/fidl/fuchsia-mem \

include make/module.mk


# devhost - container for drivers
#
# This is just a main() that calls device_host_main() which
# is provided by libdriver, where all the other devhost-*.c
# files get built.
#
MODULE := $(LOCAL_DIR).host

# The ASanified devhost is installed as devhost.asan so that
# devmgr can use the ASanified host for ASanified driver modules.
# TODO(mcgrathr): One day, both devhost and devhost.asan can both go
# into the same system image, independent of whether devmgr is ASanified.
ifeq ($(call TOBOOL,$(USE_ASAN)),true)
DEVHOST_SUFFIX := .asan
else
DEVHOST_SUFFIX :=
endif

MODULE_NAME := devhost$(DEVHOST_SUFFIX)

MODULE_TYPE := userapp
MODULE_GROUP := core

MODULE_SRCS := \
	$(LOCAL_DIR)/devhost/main.cpp

MODULE_LIBS := \
    system/ulib/driver \
    system/ulib/fdio \
    system/ulib/c \

include make/module.mk


# dmctl - bridge between dm command and devmgr

MODULE := $(LOCAL_DIR).dmctl

MODULE_TYPE := driver

MODULE_NAME := dmctl

MODULE_SRCS := \
	$(LOCAL_DIR)/dmctl/dmctl.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/ddk \
    system/ulib/ddktl \
    system/ulib/fbl \
    system/ulib/fidl \
    system/ulib/zx \
    system/ulib/zxcpp \

MODULE_LIBS := system/ulib/driver system/ulib/zircon system/ulib/c

MODULE_FIDL_LIBS := \
    system/fidl/fuchsia-device-manager \

include make/module.mk
