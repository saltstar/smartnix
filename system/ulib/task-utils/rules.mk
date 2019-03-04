
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += \
    $(LOCAL_DIR)/get.c \
    $(LOCAL_DIR)/walker.cpp

MODULE_LIBS := \
    system/ulib/zircon \
    system/ulib/fdio \
    system/ulib/c

MODULE_STATIC_LIBS := system/ulib/zx

MODULE_FIDL_LIBS := system/fidl/fuchsia-sysinfo

include make/module.mk
