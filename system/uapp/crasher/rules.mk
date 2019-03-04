
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := misc

MODULE_SRCS += \
    $(LOCAL_DIR)/crasher.c \
    $(LOCAL_DIR)/cpp_specific.cpp \

MODULE_NAME := crasher

MODULE_LIBS := system/ulib/fdio system/ulib/c system/ulib/zircon
MODULE_STATIC_LIBS := system/ulib/zxcpp

MODULE_COMPILEFLAGS := -fstack-protector-all

include make/module.mk
