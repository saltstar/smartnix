
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := hostapp

MODULE_SRCS += $(LOCAL_DIR)/bootserver.c
MODULE_SRCS += $(LOCAL_DIR)/netboot.c
MODULE_SRCS += $(LOCAL_DIR)/tftp.c

MODULE_HOST_LIBS := system/ulib/tftp

MODULE_PACKAGE := bin

include make/module.mk
