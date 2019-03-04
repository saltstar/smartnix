
SRC_DIR := system/ulib/pretty
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

KERNEL_INCLUDES += $(SRC_DIR)/include

MODULE_SRCS := \
    $(SRC_DIR)/hexdump.c \
    $(SRC_DIR)/sizes.c \

include make/module.mk

