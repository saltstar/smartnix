
# host blobfs lib

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR).hostlib

MODULE_TYPE := hostlib

MODULE_SRCS := $(LOCAL_DIR)/common.cpp

MODULE_COMPILEFLAGS := \
    -Werror-implicit-function-declaration \
    -Wstrict-prototypes -Wwrite-strings \
    -Isystem/ulib/zircon/include \
    -Isystem/ulib/fbl/include \

MODULE_HOST_LIBS := \
    system/ulib/fbl.hostlib \

include make/module.mk
