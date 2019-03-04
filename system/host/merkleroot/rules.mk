
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := hostapp

MODULE_COMPILEFLAGS += \
	-Ithird_party/ulib/uboringssl/include \
	-Isystem/ulib/digest/include \
	-Isystem/ulib/zxcpp/include \
	-Isystem/ulib/fbl/include

MODULE_SRCS += \
	$(LOCAL_DIR)/merkleroot.cpp

MODULE_HOST_LIBS := \
	third_party/ulib/uboringssl.hostlib \
	system/ulib/digest.hostlib \
	system/ulib/fbl.hostlib \

MODULE_PACKAGE := bin

include make/module.mk
