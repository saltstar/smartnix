
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/arm_generic_timer.cpp

MODULE_DEPS += \
	kernel/lib/fixed_point

include make/module.mk
