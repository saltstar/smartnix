
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_SRCS += \
    $(LOCAL_DIR)/blktest.cpp \

MODULE_SO_NAME := blktest

MODULE_STATIC_LIBS := \
    system/ulib/block-client \
    system/ulib/sync \
    system/ulib/pretty \
    system/ulib/zxcpp \
    system/ulib/fbl \

MODULE_LIBS := \
    system/ulib/c \
    system/ulib/zircon \
    system/ulib/fdio \
    system/ulib/unittest \

include make/module.mk
