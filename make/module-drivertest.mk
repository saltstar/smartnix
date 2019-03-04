
# By default test drivers live in driver/test/.

ifeq ($(MODULE_SO_INSTALL_NAME),)
MODULE_SO_INSTALL_NAME := driver/test/$(MODULE_NAME).so
endif

include make/module-userlib.mk
