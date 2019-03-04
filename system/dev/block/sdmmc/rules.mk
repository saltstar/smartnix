
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := driver

MODULE_SRCS += \
    $(LOCAL_DIR)/mmc.c \
    $(LOCAL_DIR)/ops.c \
    $(LOCAL_DIR)/sd.c \
    $(LOCAL_DIR)/sdmmc.c \
    $(LOCAL_DIR)/sdio.c \
    $(LOCAL_DIR)/sdio-interrupts.c \

MODULE_STATIC_LIBS := \
    system/ulib/ddk \
    system/ulib/sync \
    system/ulib/pretty \

MODULE_LIBS := system/ulib/driver \
    system/ulib/c \
    system/ulib/zircon \
    system/ulib/fdio \

ifeq ($(call TOBOOL,$(ENABLE_DRIVER_TRACING)),true)
MODULE_STATIC_LIBS += system/ulib/trace.driver
endif
MODULE_HEADER_DEPS += system/ulib/trace system/ulib/trace-engine

include make/module.mk
