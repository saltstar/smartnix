
LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/psci.cpp

include make/module.mk
