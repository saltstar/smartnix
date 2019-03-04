
# By default drivers live in driver/.

ifeq ($(MODULE_SO_INSTALL_NAME),)
MODULE_SO_INSTALL_NAME := driver/$(MODULE_NAME).so
endif

include make/module-userlib.mk
