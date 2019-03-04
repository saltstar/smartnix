
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/init.cpp

include make/module.mk

# copy our all-boards.list for the fuchsia build
# this file should contain only boards that we want to support outside of zircon
BOARD_LIST_SRC := $(LOCAL_DIR)/all-boards.list
BOARD_LIST_DEST := $(BUILDDIR)/export/all-boards.list

$(BOARD_LIST_DEST): $(BOARD_LIST_SRC)
	$(call BUILDECHO,copying $@)
	@$(MKDIR)
	$(NOECHO)cp $< $@

packages: $(BOARD_LIST_DEST)
