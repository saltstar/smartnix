
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := fidl

MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := fuchsia.net

MODULE_SRCS += \
    $(LOCAL_DIR)/net.fidl \
    $(LOCAL_DIR)/connectivity.fidl \
    $(LOCAL_DIR)/socket.fidl

include make/module.mk
