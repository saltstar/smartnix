
SRC_DIR := system/ulib/zxcpp
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
    $(SRC_DIR)/new.cpp \
    $(SRC_DIR)/pure_virtual.cpp \

KERNEL_INCLUDES += $(SRC_DIR)/include

include make/module.mk
