
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := fidl

MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := zircon.usb.test.fwloader

MODULE_SRCS += $(LOCAL_DIR)/usb-test-fwloader.fidl

include make/module.mk
