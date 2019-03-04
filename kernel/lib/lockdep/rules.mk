
SRC_DIR := system/ulib/lockdep
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

KERNEL_INCLUDES += $(LOCAL_DIR)/include \
                   $(SRC_DIR)/include

MODULE_SRCS := \
	$(LOCAL_DIR)/lock_dep.cpp

include make/module.mk
