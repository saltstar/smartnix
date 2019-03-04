
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

PLATFORM := pc

MODULE_SRCS := $(LOCAL_DIR)/empty.cpp

MODULE_DEPS := \
    kernel/dev/intel_rng

include make/module.mk

include $(LOCAL_DIR)/multiboot/rules.mk
