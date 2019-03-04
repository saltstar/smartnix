
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := fidl

MODULE_PACKAGE := fidl

MODULE_FIDL_LIBRARY := fuchsia.sysmem

MODULE_SRCS += \
	$(LOCAL_DIR)/allocator.fidl \
	$(LOCAL_DIR)/collections.fidl \
	$(LOCAL_DIR)/formats.fidl \
	$(LOCAL_DIR)/image_formats.fidl \
	$(LOCAL_DIR)/usages.fidl

include make/module.mk
