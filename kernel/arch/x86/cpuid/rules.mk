
LOCAL_DIR := $(GET_LOCAL_DIR)

# Common Code

LOCAL_SRCS := \
    $(LOCAL_DIR)/cpuid.cpp \
    $(LOCAL_DIR)/cpuid_test.cpp \

# system-topology

MODULE := $(LOCAL_DIR)

MODULE_GROUP := core

MODULE_SRCS := $(LOCAL_SRCS)

MODULE_DEPS := \
    kernel/lib/fbl \
    kernel/lib/unittest \

MODULE_NAME := cpuid

include make/module.mk
