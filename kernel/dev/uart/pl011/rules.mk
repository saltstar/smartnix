
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/uart.cpp

MODULE_DEPS += \
	kernel/dev/pdev \
	kernel/dev/pdev/uart \

include make/module.mk
