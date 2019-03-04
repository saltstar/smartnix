
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := core

MODULE_SRCS += \
    $(LOCAL_DIR)/lsusb.c

MODULE_STATIC_LIBS := system/ulib/pretty

MODULE_LIBS := system/ulib/fdio system/ulib/zircon system/ulib/c

MODULE_FIDL_LIBS := system/fidl/zircon-usb-device

include make/module.mk
