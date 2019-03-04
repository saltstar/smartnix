
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	kernel/lib/debug \
	kernel/lib/console

MODULE_SRCS += \
	$(LOCAL_DIR)/debugcommands.cpp

include make/module.mk
