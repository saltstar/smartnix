
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/arm_gicv2.cpp \
	$(LOCAL_DIR)/arm_gicv2m.cpp \
	$(LOCAL_DIR)/arm_gicv2m_msi.cpp \
	$(LOCAL_DIR)/arm_gicv2m_pcie.cpp \

MODULE_DEPS += \
	kernel/dev/interrupt \
	kernel/dev/interrupt/arm_gic/common \
	kernel/dev/pdev \
	kernel/dev/pdev/interrupt \
	kernel/lib/pow2_range_allocator

include make/module.mk
