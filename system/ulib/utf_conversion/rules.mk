
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_STATIC_LIBS := system/ulib/fbl

MODULE_SRCS += \
    $(LOCAL_DIR)/utf_conversion.cpp

MODULE_PACKAGE := static

include make/module.mk
