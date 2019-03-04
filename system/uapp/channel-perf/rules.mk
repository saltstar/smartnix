
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := misc

MODULE_SRCS += \
    $(LOCAL_DIR)/main.cpp \

MODULE_LIBS := system/ulib/zircon system/ulib/fdio system/ulib/c
MODULE_STATIC_LIBS := system/ulib/zxcpp system/ulib/fbl

include make/module.mk
