
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib
MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS = \
    $(LOCAL_DIR)/empty.c \

MODULE_PACKAGE := src

include make/module.mk
