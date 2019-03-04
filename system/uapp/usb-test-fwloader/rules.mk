
LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)
MODULE_TYPE := userapp
MODULE_GROUP := misc

MODULE_SRCS += \
    $(LOCAL_DIR)/usb-test-fwloader.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/zx \
    system/ulib/zxcpp \
    system/ulib/fbl

MODULE_LIBS := \
    system/ulib/fdio \
    system/ulib/zircon \
    system/ulib/c

MODULE_FIDL_LIBS := \
    system/fidl/zircon-usb-test-fwloader \
    system/fidl/zircon-usb-tester

include make/module.mk
