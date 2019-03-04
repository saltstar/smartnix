
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS := \
    $(LOCAL_DIR)/block.c \
    $(LOCAL_DIR)/server.cpp \
    $(LOCAL_DIR)/txn-group.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/ddk \
    system/ulib/sync \
    system/ulib/zx \
    system/ulib/zxcpp \
    system/ulib/fbl \
    system/ulib/fzl \

MODULE_LIBS := system/ulib/c system/ulib/driver system/ulib/zircon

include make/module.mk
