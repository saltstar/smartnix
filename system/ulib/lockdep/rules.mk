
LOCAL_DIR := $(GET_LOCAL_DIR)

#
# Userspace library.
#

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += \
    $(LOCAL_DIR)/lock_dep.cpp \

MODULE_LIBS := \
    system/ulib/fbl \
    system/ulib/zx

MODULE_PACKAGE := src
MODULE_STATIC_LIBS := \
    system/ulib/fbl \
    system/ulib/zx

include make/module.mk

