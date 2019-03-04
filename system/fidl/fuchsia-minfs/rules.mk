
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := fidl

MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := fuchsia.minfs

MODULE_SRCS += $(LOCAL_DIR)/minfs.fidl

include make/module.mk
