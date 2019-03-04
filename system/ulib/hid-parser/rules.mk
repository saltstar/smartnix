
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_PACKAGE := static

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += \
    $(LOCAL_DIR)/item.cpp \
    $(LOCAL_DIR)/parser.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/fbl \
    system/ulib/zxcpp

MODULE_LIBS := \
    system/ulib/c \
    system/ulib/zircon

include make/module.mk
