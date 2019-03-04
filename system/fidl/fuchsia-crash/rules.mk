
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := fidl

MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := fuchsia.crash

MODULE_FIDL_DEPS := system/fidl/fuchsia-mem

MODULE_SRCS += $(LOCAL_DIR)/crash.fidl

include make/module.mk
