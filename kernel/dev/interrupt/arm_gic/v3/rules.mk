
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

KERNEL_INCLUDES += kernel/dev/interrupt/arm_gic/v2/include

MODULE_SRCS += \
	$(LOCAL_DIR)/arm_gicv3.cpp \
	$(LOCAL_DIR)/arm_gicv3_pcie.cpp

MODULE_DEPS += \
	kernel/dev/interrupt \
	kernel/dev/interrupt/arm_gic/common \
	kernel/dev/pdev \
	kernel/dev/pdev/interrupt \

include make/module.mk
