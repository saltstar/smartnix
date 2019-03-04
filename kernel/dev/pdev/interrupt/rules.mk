
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/interrupt.cpp

MODULE_DEPS += \
    kernel/dev/pdev \

include make/module.mk
