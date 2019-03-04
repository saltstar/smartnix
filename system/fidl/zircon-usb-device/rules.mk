
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_TYPE := fidl
MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := zircon.usb.device
MODULE_SRCS += $(LOCAL_DIR)/usb-device.fidl

include make/module.mk
