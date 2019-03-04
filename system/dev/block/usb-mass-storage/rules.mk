
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS := \
    $(LOCAL_DIR)/block.c \
    $(LOCAL_DIR)/usb-mass-storage.c \

MODULE_STATIC_LIBS := system/ulib/ddk system/dev/lib/usb system/ulib/sync

MODULE_LIBS := system/ulib/driver system/ulib/zircon system/ulib/c

include make/module.mk
