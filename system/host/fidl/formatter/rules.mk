
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_NAME := fidl-format

MODULE_TYPE := hostapp

MODULE_SRCS := \
    $(LOCAL_DIR)/main.cpp \

MODULE_HOST_LIBS := \
    system/host/fidl \

MODULE_PACKAGE := bin

include make/module.mk
