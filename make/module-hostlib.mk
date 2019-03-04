
# check for disallowed options
ifneq ($(MODULE_DEPS)$(MODULE_LIBS)$(MODULE_STATIC_LIBS)$(MODULE_FIDL_LIBS)$(MODULE_BANJO_LIBS),)
$(error $(MODULE) $(MODULE_TYPE) modules must not use MODULE_{DEPS,LIBS,STATIC_LIBS,FIDL_LIBS,BANJO_LIBS})
endif

MODULE_HOSTLIB := $(call TOBUILDDIR,tools/lib/lib$(MODULE_NAME).a)

$(MODULE_HOSTLIB): $(MODULE_OBJS)
	@$(MKDIR)
	$(call BUILDECHO,linking hostlib $@)
	$(NOECHO)rm -f -- "$@"
	$(NOECHO)$(HOST_AR) cr $@ $^

ALLHOST_LIBS += $(MODULE_HOSTLIB)

GENERATED += $(MODULE_HOSTLIB)

