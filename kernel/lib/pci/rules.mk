
LOCAL_DIR := $(GET_LOCAL_DIR)

KERNEL_INCLUDES += $(LOCAL_DIR)/include

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/pio.cpp

include make/module.mk
