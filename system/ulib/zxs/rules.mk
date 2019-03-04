
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += \
    $(LOCAL_DIR)/zxs.cpp \

MODULE_FIDL_LIBS := system/fidl/fuchsia-net

MODULE_LIBS := \
    system/ulib/zx \
    system/ulib/zircon \

MODULE_PACKAGE := static

include make/module.mk
