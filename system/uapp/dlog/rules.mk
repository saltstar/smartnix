
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := core

MODULE_SRCS += \
	$(LOCAL_DIR)/dlog.c

MODULE_LIBS := \
    system/ulib/fdio system/ulib/zircon system/ulib/c

include make/module.mk
