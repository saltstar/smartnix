
LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
    $(LOCAL_DIR)/pow2_tests.cpp

MODULE_DEPS = kernel/lib/unittest

include make/module.mk

