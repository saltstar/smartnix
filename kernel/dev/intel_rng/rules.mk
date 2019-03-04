
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/intel-rng.cpp \

MODULE_DEPS += kernel/dev/hw_rng

include make/module.mk
