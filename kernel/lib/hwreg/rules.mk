
SRC_DIR := system/ulib/hwreg
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

KERNEL_INCLUDES += $(SRC_DIR)/include

MODULE_SRCS := $(SRC_DIR)/printers.cpp
MODULE_DEPS := kernel/lib/fbl

include make/module.mk
