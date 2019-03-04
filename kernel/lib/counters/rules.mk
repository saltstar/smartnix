
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/counters.cpp

MODULE_DEPS += \
	kernel/lib/console

include make/module.mk
