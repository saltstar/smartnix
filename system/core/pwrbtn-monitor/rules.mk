
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := core

MODULE_SRCS += \
    $(LOCAL_DIR)/pwrbtn-monitor.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/fbl \
    system/ulib/fzl \
    system/ulib/hid-parser \
    system/ulib/zxcpp \
    system/ulib/zx

MODULE_LIBS := \
    system/ulib/fdio \
    system/ulib/c \
    system/ulib/zircon

MODULE_FIDL_LIBS := \
    system/fidl/zircon-input \

include make/module.mk
