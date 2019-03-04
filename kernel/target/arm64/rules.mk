
LOCAL_DIR := $(GET_LOCAL_DIR)

PLATFORM := generic-arm

# include rules for our various arm64 boards
include $(LOCAL_DIR)/board/*/rules.mk
