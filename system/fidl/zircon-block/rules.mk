
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := fidl

MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := zircon.block

MODULE_SRCS += \
    $(LOCAL_DIR)/ftl.fidl \

include make/module.mk
