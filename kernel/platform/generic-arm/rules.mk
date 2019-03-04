
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/platform.cpp \

ARCH := arm64

KERNEL_DEFINES += \
	PLATFORM_SUPPORTS_PANIC_SHELL=1

MODULE_DEPS += \
	kernel/lib/cbuf \
	kernel/lib/lockdep \
	kernel/lib/fbl \
	kernel/lib/memory_limit \
	kernel/lib/zbi \
	kernel/dev/pcie \
	kernel/dev/pdev \
	kernel/dev/pdev/power \
	kernel/dev/timer/arm_generic \
	kernel/dev/interrupt/arm_gic/v2 \
	kernel/dev/interrupt/arm_gic/v3 \
	kernel/dev/interrupt/arm_gic/common \
	kernel/dev/power/hisi \
	kernel/dev/psci \
	kernel/dev/uart/amlogic_s905 \
	kernel/dev/uart/mt8167 \
	kernel/dev/uart/nxp-imx \
	kernel/dev/uart/pl011 \
	kernel/dev/hdcp/amlogic_s912 \

include make/module.mk
