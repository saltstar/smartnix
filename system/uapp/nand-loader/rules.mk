
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := core

MODULE_SRCS := \
    $(LOCAL_DIR)/main.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/fbl \
    system/ulib/fzl \
    system/ulib/zx \
    system/ulib/zxcpp \

MODULE_LIBS := \
    system/ulib/c \
    system/ulib/fdio \
    system/ulib/fs-management \
    system/ulib/zircon \

MODULE_FIDL_LIBS := \
    system/fidl/zircon-nand \

include make/module.mk
