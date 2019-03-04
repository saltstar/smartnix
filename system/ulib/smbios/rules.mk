
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_COMPILEFLAGS += -fvisibility=hidden

MODULE_SRCS += \
    $(LOCAL_DIR)/smbios.cpp \

MODULE_LIBS := \
	system/ulib/c \
	system/ulib/fbl \
	system/ulib/zircon \

include make/module.mk
