
# check for disallowed options
ifneq ($(MODULE_DEPS)$(MODULE_LIBS)$(MODULE_STATIC_LIBS)$(MODULE_FIDL_LIBS)$(MODULE_BANJO_LIBS),)
$(error $(MODULE) $(MODULE_TYPE) modules must not use MODULE_{DEPS,LIBS,STATIC_LIBS,FIDL_LIBS,BANJO_LIBS})
endif

MODULE_EFILIB := $(call TOBUILDDIR,EFI_libs/lib$(MODULE_NAME).a)

$(MODULE_EFILIB): $(MODULE_OBJS)
	@$(MKDIR)
	$(call BUILDECHO,linking efilib $@)
	$(NOECHO)rm -f -- "$@"
	$(NOECHO)$(EFI_AR) cr $@ $^

ALLEFI_LIBS += $(MODULE_EFILIB)

GENERATED += $(MODULE_EFILIB)

