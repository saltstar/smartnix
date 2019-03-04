
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/el2.S \
	$(LOCAL_DIR)/el2_cpu_state.cpp \
	$(LOCAL_DIR)/guest.cpp \
	$(LOCAL_DIR)/vcpu.cpp \
	$(LOCAL_DIR)/vmexit.cpp \
	$(LOCAL_DIR)/gic/gicv2.cpp \
	$(LOCAL_DIR)/gic/gicv3.cpp \
	$(LOCAL_DIR)/gic/el2.S \

include make/module.mk
