
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := core

MODULE_SRCS += $(LOCAL_DIR)/ethtool.c

MODULE_STATIC_LIBS := system/ulib/pretty system/ulib/inet6

MODULE_LIBS := system/ulib/fdio system/ulib/zircon system/ulib/c

MODULE_FIDL_LIBS := system/fidl/zircon-ethernet

include make/module.mk
