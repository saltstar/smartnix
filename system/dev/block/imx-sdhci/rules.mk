
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS := \
    $(LOCAL_DIR)/imx-sdhci.c

MODULE_STATIC_LIBS := system/ulib/ddk system/ulib/sync system/ulib/pretty

MODULE_LIBS := system/ulib/driver system/ulib/zircon system/ulib/c

MODULE_HEADER_DEPS := system/dev/lib/imx8m

include make/module.mk
