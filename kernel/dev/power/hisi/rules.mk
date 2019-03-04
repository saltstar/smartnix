
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/power.cpp

MODULE_DEPS += \
    kernel/dev/pdev \
    kernel/dev/pdev/power

include make/module.mk
