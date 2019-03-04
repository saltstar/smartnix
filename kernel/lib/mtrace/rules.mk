
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/mtrace.cpp \
	$(LOCAL_DIR)/mtrace-ipm.cpp \
	$(LOCAL_DIR)/mtrace-ipt.cpp

include make/module.mk
