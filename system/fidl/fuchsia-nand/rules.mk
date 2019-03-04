
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := fidl

MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := fuchsia.nand

MODULE_SRCS += \
    $(LOCAL_DIR)/broker.fidl \

MODULE_FIDL_DEPS := system/fidl/zircon-nand

include make/module.mk
