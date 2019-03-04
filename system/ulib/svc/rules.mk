
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += \
    $(LOCAL_DIR)/outgoing.cpp

MODULE_STATIC_LIBS := \
    system/ulib/fs \
    system/ulib/async \
    system/ulib/fbl \
    system/ulib/zx

MODULE_LIBS := \
    system/ulib/fdio \
    system/ulib/c \
    system/ulib/zircon

MODULE_FIDL_LIBS := \
    system/fidl/fuchsia-io \

include make/module.mk
