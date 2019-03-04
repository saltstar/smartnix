
# tests only differ from apps by their install location

MODULE_HOSTAPP_BIN := $(BUILDDIR)/host_tests/$(MODULE_NAME)

include make/module-hostapp.mk
