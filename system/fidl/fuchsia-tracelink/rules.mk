
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := fidl

MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := fuchsia.tracelink

MODULE_SRCS += $(LOCAL_DIR)/tracelink.fidl

include make/module.mk
