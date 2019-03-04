
LOCAL_DIR := $(GET_LOCAL_DIR)

ASM_STRING_OPS := memcpy memset

MODULE_SRCS += \
	$(LOCAL_DIR)/memcpy.S \
	$(LOCAL_DIR)/memset.S \
	$(LOCAL_DIR)/selector.cpp \
	$(LOCAL_DIR)/tests.cpp \

MODULE_DEPS += \
	kernel/lib/code_patching \
	kernel/lib/unittest \

# filter out the C implementation
C_STRING_OPS := $(filter-out $(ASM_STRING_OPS),$(C_STRING_OPS))
