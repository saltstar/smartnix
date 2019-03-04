
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/arm_gic_common.cpp \
	$(LOCAL_DIR)/arm_gic_hw_interface.cpp \

include make/module.mk
