
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := misc

MODULE_SRCS += \
    $(LOCAL_DIR)/usbctl.c

MODULE_LIBS := \
    system/ulib/zircon \
    system/ulib/fdio \
    system/ulib/c

MODULE_STATIC_LIBS := \
    system/ulib/fidl \

MODULE_FIDL_LIBS := system/fidl/zircon-usb-peripheral

include make/module.mk
