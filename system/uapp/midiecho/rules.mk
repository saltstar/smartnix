
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := misc

MODULE_SRCS += \
    $(LOCAL_DIR)/midiecho.c

MODULE_LIBS := system/ulib/zircon system/ulib/fdio system/ulib/c

include make/module.mk
