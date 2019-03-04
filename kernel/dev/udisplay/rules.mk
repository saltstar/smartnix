
LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
    kernel/lib/crashlog \
    kernel/lib/gfx \
    kernel/lib/gfxconsole

MODULE_SRCS += \
	$(LOCAL_DIR)/udisplay.cpp

include make/module.mk
