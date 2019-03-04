
SRC_DIR := system/ulib/fbl
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

KERNEL_INCLUDES += $(LOCAL_DIR)/include \
                   $(SRC_DIR)/include

MODULE_SRCS := \
    $(LOCAL_DIR)/arena.cpp \
    $(LOCAL_DIR)/arena_tests.cpp \
    $(LOCAL_DIR)/inline_array_tests.cpp \
    $(LOCAL_DIR)/name_tests.cpp \
    $(SRC_DIR)/alloc_checker.cpp \

include make/module.mk
