
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userlib

MODULE_SRCS += \
	$(LOCAL_DIR)/pio.cpp

MODULE_LIBS := \
    system/ulib/fbl \
    system/ulib/hwreg \
    system/ulib/ddk \
    system/ulib/ddktl \
    system/ulib/zx \

include make/module.mk
