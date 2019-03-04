
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += \
    $(LOCAL_DIR)/io-buffer.c \
    $(LOCAL_DIR)/mmio-buffer.c \
    $(LOCAL_DIR)/phys-iter.c \

MODULE_STATIC_LIBS := system/ulib/pretty system/ulib/sync

MODULE_FIDL_LIBS := \
    system/fidl/zircon-nand \

MODULE_EXPORT := a

include make/module.mk
