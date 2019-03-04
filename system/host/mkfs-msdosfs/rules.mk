
LOCAL_DIR := $(GET_LOCAL_DIR)

MKMSDOS_DIR := third_party/uapp/mkfs-msdosfs

MODULE_DEFINES := _XOPEN_SOURCE _GNU_SOURCE

MODULE := $(LOCAL_DIR)

MODULE_TYPE := hostapp

MODULE_SRCS += \
	$(MKMSDOS_DIR)/mkfs_msdos.c \
	$(MKMSDOS_DIR)/mkfs_msdos.h \
	$(MKMSDOS_DIR)/newfs_msdos.c

MODULE_PACKAGE := bin

include make/module.mk
