
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := misc

MODULE_SRCS += $(LOCAL_DIR)/lspwr.c

MODULE_LIBS := system/ulib/zircon system/ulib/c system/ulib/fdio

include make/module.mk
