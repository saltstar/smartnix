
LOCAL_DIR := $(GET_LOCAL_DIR)

#
# Userspace library.
#

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE := $(LOCAL_DIR)
MODULE_TYPE := userlib
MODULE_PACKAGE := src

MODULE_SRCS := \
    $(LOCAL_DIR)/empty.c \

MODULE_STATIC_LIBS := \
    system/ulib/async \
    system/ulib/async.cpp \
    system/ulib/fbl \
    system/ulib/zx \
    system/ulib/zxcpp \

MODULE_LIBS := \
    system/ulib/fidl \
    system/ulib/zircon \

include make/module.mk
