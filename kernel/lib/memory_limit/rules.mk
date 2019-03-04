
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/memory_limit.cpp \

MODULE_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_DEPS += \
    kernel/lib/fbl \
    kernel/lib/pretty \

include make/module.mk
