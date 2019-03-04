
LOCAL_DIR := $(GET_LOCAL_DIR)

KERNEL_INCLUDES += $(LOCAL_DIR)/include

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/page_tables.cpp

MODULE_DEPS += \
	kernel/lib/fbl \
	kernel/lib/hwreg \

include make/module.mk
