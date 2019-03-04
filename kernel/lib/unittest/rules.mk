
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/unittest.cpp \
	$(LOCAL_DIR)/user_memory.cpp \

include make/module.mk
