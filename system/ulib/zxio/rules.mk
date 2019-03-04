
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += \
    $(LOCAL_DIR)/null.cpp \
    $(LOCAL_DIR)/pipe.cpp \
    $(LOCAL_DIR)/remote.cpp \
    $(LOCAL_DIR)/zxio.cpp \

MODULE_FIDL_LIBS := system/fidl/fuchsia-io

MODULE_STATIC_LIBS := system/ulib/zx

MODULE_LIBS := system/ulib/zircon

MODULE_PACKAGE := static

include make/module.mk
