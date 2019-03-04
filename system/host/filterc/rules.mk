
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := hostapp

MODULE_SRCS += \
    $(LOCAL_DIR)/filterc.cpp \

MODULE_PACKAGE := bin

include make/module.mk
