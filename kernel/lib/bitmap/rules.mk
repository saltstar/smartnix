
SRC_DIR := system/ulib/bitmap
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

KERNEL_INCLUDES += $(SRC_DIR)/include

MODULE_DEPS := kernel/lib/fbl

MODULE_SRCS := \
    $(SRC_DIR)/raw-bitmap.cpp \
    $(SRC_DIR)/rle-bitmap.cpp \

include make/module.mk
