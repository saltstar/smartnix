
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += $(LOCAL_DIR)/printers.cpp
MODULE_LIBS += system/ulib/fbl

MODULE_PACKAGE := src

include make/module.mk
