
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS := \
    $(LOCAL_DIR)/ums-function.c \

MODULE_STATIC_LIBS := system/ulib/ddk system/ulib/sync system/dev/lib/usb

MODULE_LIBS := system/ulib/driver system/ulib/zircon system/ulib/c

include make/module.mk
