
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS := $(LOCAL_DIR)/ramdisk.c

MODULE_STATIC_LIBS := \
    system/ulib/ddk \
    system/ulib/sync \
    system/ulib/zxcpp \
    system/ulib/fbl

MODULE_LIBS := system/ulib/driver system/ulib/zircon system/ulib/c

include make/module.mk
