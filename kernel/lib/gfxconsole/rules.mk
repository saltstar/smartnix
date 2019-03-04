
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	kernel/lib/gfx

MODULE_SRCS += \
	$(LOCAL_DIR)/gfxconsole.cpp

include make/module.mk
