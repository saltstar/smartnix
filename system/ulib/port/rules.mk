
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += $(LOCAL_DIR)/port.c

MODULE_LIBS := system/ulib/fdio system/ulib/c

include make/module.mk

