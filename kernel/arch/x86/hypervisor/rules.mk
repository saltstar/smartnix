
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/guest.cpp \
	$(LOCAL_DIR)/vcpu.cpp \
	$(LOCAL_DIR)/vmexit.cpp \
	$(LOCAL_DIR)/vmx.S \
	$(LOCAL_DIR)/vmx_cpu_state.cpp \
	$(LOCAL_DIR)/pvclock.cpp \

include make/module.mk
