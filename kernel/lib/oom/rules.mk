
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS :=

MODULE_SRCS := \
	$(LOCAL_DIR)/oom.cpp

include make/module.mk
