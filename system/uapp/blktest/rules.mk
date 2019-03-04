
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := misc

MODULE_SRCS += \
    $(LOCAL_DIR)/main.c

MODULE_LIBS := \
    system/ulib/c \
    system/ulib/unittest \
    system/ulib/blktest \

include make/module.mk
