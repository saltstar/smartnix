
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_SRCS += \
    $(LOCAL_DIR)/elf-search.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/fbl \
    system/ulib/zx \

MODULE_LIBS := \
    system/ulib/zircon \
    system/ulib/c \

MODULE_PACKAGE := src

include make/module.mk
