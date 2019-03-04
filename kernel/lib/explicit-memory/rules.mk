
SRC_DIR := system/ulib/explicit-memory
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

KERNEL_INCLUDES += $(SRC_DIR)/include

MODULE_SRCS += \
    $(SRC_DIR)/bytes.c \

include make/module.mk
