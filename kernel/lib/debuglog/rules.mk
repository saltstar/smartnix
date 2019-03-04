
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
    $(LOCAL_DIR)/debuglog.cpp \

MODULE_DEPS := \
    kernel/lib/crashlog \
    kernel/lib/version

include make/module.mk
