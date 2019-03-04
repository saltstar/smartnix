
LOCAL_DIR := $(GET_LOCAL_DIR)

# Host library.

MODULE := $(LOCAL_DIR).hostlib

MODULE_TYPE := hostlib

MODULE_SRCS += \
    $(LOCAL_DIR)/client.cpp \

MODULE_COMPILEFLAGS := \
    -Isystem/ulib/fbl/include \

MODULE_HOST_LIBS := \
    system/ulib/fbl.hostlib \

include make/module.mk
