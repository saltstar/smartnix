
LOCAL_DIR := $(GET_LOCAL_DIR)

# Common Code

LOCAL_SRCS := \
    $(LOCAL_DIR)/system-topology.cpp \
    $(LOCAL_DIR)/system-topology_test.cpp \

# system-topology

MODULE := $(LOCAL_DIR)

MODULE_GROUP := core

MODULE_SRCS := $(LOCAL_SRCS)

MODULE_DEPS := \
    kernel/lib/fbl \
    kernel/lib/unittest \

MODULE_NAME := system-topology

include make/module.mk
