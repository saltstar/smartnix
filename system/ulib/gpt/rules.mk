
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += \
    $(LOCAL_DIR)/cros.c \
    $(LOCAL_DIR)/gpt.c \

MODULE_STATIC_LIBS := third_party/ulib/cksum

MODULE_LIBS := system/ulib/c

include make/module.mk
