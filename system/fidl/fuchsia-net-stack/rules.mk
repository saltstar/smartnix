
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := fidl

MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := fuchsia.net.stack

MODULE_FIDL_DEPS := system/fidl/fuchsia-net

MODULE_SRCS += $(LOCAL_DIR)/stack.fidl

include make/module.mk
