
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS += \
    $(LOCAL_DIR)/aml-sd-emmc.c

MODULE_STATIC_LIBS := system/ulib/ddk system/ulib/sync

MODULE_LIBS := system/ulib/driver system/ulib/zircon system/ulib/c

MODULE_HEADER_DEPS := $(LOCAL_DIR) system/dev/lib/amlogic

include make/module.mk
