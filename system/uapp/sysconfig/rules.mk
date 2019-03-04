
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_GROUP := misc

MODULE_TYPE := userapp

MODULE_SRCS += \
    $(LOCAL_DIR)/sysconfig.c

MODULE_LIBS := system/ulib/zircon system/ulib/fdio system/ulib/c

MODULE_STATIC_LIBS := system/ulib/kvstore third_party/ulib/cksum

MODULE_HEADER_DEPS := system/ulib/ddk

include make/module.mk
