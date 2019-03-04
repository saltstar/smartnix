
LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
    $(LOCAL_DIR)/dummy_iommu.cpp

include make/module.mk
