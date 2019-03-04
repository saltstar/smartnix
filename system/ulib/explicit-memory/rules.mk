
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += \
    $(LOCAL_DIR)/bytes.c \

MODULE_HEADER_DEPS += system/ulib/c system/ulib/fbl

MODULE_EXPORT := a

include make/module.mk
