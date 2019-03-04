
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_SRCS += \
    $(LOCAL_DIR)/ldmsg.c \

MODULE_LIBS := \
    system/ulib/zircon \

MODULE_PACKAGE := static

include make/module.mk
