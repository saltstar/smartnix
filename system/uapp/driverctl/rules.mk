
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_GROUP := misc

MODULE_TYPE := userapp

MODULE_SRCS += \
    $(LOCAL_DIR)/driverctl.c

MODULE_LIBS := system/ulib/zircon system/ulib/fdio system/ulib/c

MODULE_HEADER_DEPS := system/ulib/ddk

include make/module.mk
