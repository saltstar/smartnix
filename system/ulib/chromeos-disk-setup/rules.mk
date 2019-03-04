
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib
MODULE_GROUP := core

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS := $(LOCAL_DIR)/chromeos-disk-setup.c

MODULE_LIBS := system/ulib/c

MODULE_STATIC_LIBS := system/ulib/gpt \
    system/ulib/zircon

include make/module.mk
