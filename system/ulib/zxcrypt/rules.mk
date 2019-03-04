
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_SO_NAME := $(LOCAL_DIR)
MODULE_TYPE := userlib

MODULE_SRCS += \
    $(LOCAL_DIR)/volume.cpp \

MODULE_LIBS := \
    system/ulib/c \
    system/ulib/crypto \
    system/ulib/driver \
    system/ulib/fdio \
    system/ulib/fs-management \
    system/ulib/zircon \

MODULE_STATIC_LIBS := \
    third_party/ulib/uboringssl \
    system/ulib/ddk \
    system/ulib/elf-search \
    system/ulib/fbl \
    system/ulib/fs-management \
    system/ulib/pretty \
    system/ulib/sync \
    system/ulib/zx \
    system/ulib/zxcpp

MODULE_COMPILEFLAGS := -fsanitize=integer-divide-by-zero,signed-integer-overflow -fsanitize-undefined-trap-on-error

include make/module.mk
