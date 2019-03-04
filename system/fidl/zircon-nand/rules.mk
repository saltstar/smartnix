
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := fidl

MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := zircon.nand

MODULE_SRCS += \
    $(LOCAL_DIR)/nand.fidl \
    $(LOCAL_DIR)/ram-nand.fidl \

include make/module.mk

MODULE := $(LOCAL_DIR).skipblock

MODULE_TYPE := fidl

MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := zircon.skipblock

MODULE_SRCS += \
    $(LOCAL_DIR)/skip-block.fidl \

include make/module.mk
