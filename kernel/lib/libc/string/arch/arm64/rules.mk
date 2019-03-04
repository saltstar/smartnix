
LOCAL_DIR := $(GET_LOCAL_DIR)

ASM_STRING_OPS := memcpy memset

MODULE_SRCS += \
    third_party/lib/cortex-strings/src/aarch64/memcpy.S \
    third_party/lib/cortex-strings/no-neon/src/aarch64/memset.S \

# filter out the C implementation
C_STRING_OPS := $(filter-out $(ASM_STRING_OPS),$(C_STRING_OPS))
