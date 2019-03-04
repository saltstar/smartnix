
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_TYPE := fidl
MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := zircon.rtc
MODULE_SRCS += $(LOCAL_DIR)/rtc.fidl

include make/module.mk
