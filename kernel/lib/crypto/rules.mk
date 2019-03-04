
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/global_prng.cpp \
    $(LOCAL_DIR)/global_prng_unittest.cpp \
    $(LOCAL_DIR)/prng.cpp \
    $(LOCAL_DIR)/prng_unittest.cpp

MODULE_DEPS += third_party/lib/uboringssl
MODULE_DEPS += kernel/lib/explicit-memory
MODULE_DEPS += kernel/lib/fbl
MODULE_DEPS += kernel/lib/unittest

include $(LOCAL_DIR)/entropy/rules.mk

include make/module.mk
