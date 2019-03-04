
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := misc

MODULE_SRCS += $(LOCAL_DIR)/iotime.c

MODULE_STATIC_LIBS := \
    system/ulib/block-client \
    system/ulib/sync

MODULE_LIBS := \
    system/ulib/fs-management \
    system/ulib/fdio \
    system/ulib/zircon \
    system/ulib/c

include make/module.mk
