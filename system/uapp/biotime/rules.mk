
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := misc

MODULE_SRCS += $(LOCAL_DIR)/biotime.cpp

MODULE_STATIC_LIBS := \
    system/ulib/fbl \
    system/ulib/perftest \
    system/ulib/sync \
    system/ulib/zircon-internal \
    system/ulib/zxcpp \

MODULE_LIBS := \
    system/ulib/fdio \
    system/ulib/zircon \
    system/ulib/c

include make/module.mk