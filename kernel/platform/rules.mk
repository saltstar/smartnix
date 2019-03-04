
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# shared platform code
MODULE_SRCS += \
	$(LOCAL_DIR)/debug.cpp \
	$(LOCAL_DIR)/init.cpp \
	$(LOCAL_DIR)/power.cpp

MODULE_DEPS += \
	kernel/lib/zxcpp

include make/module.mk


