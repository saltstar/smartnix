
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := core

MODULE_SRCS += \
    $(LOCAL_DIR)/main.c

MODULE_STATIC_LIBS := \
    system/ulib/gpt \
    system/ulib/pretty \
    third_party/ulib/cksum \

MODULE_LIBS := system/ulib/zircon system/ulib/fdio system/ulib/c

MODULE_FIDL_LIBS := system/fidl/zircon-nand.skipblock

include make/module.mk
