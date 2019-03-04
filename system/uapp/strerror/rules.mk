
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := misc

MODULE_SRCS += \
    $(LOCAL_DIR)/strerror.c

MODULE_STATIC_LIBS := system/ulib/runtime
MODULE_LIBS := system/ulib/fdio system/ulib/zircon system/ulib/c

include make/module.mk
