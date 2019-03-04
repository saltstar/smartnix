
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	kernel/kernel \
	kernel/platform \
	kernel/target

MODULE_SRCS := \
	$(LOCAL_DIR)/init.cpp \
	$(LOCAL_DIR)/main.cpp \

include make/module.mk
