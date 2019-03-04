
LOCAL_DIR := $(GET_LOCAL_DIR)

KERNEL_INCLUDES += $(LOCAL_DIR)/include

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/heap_wrapper.cpp

# use the cmpctmalloc heap implementation
MODULE_DEPS := kernel/lib/heap/cmpctmalloc

include make/module.mk
