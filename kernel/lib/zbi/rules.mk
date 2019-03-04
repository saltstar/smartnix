
SRC_DIR := system/ulib/libzbi
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

KERNEL_INCLUDES += $(SRC_DIR)/include

MODULE_SRCS := \
    $(SRC_DIR)/zbi.c

include make/module.mk
