
ifneq ($(MODULE_DEPS)$(MODULE_HOST_LIBS)$(MODULE_HOST_SYSLIBS),)
$(error $(MODULE) $(MODULE_TYPE) banjo modules must use MODULE_BANJO_DEPS)
endif

MODULE_RULESMK := $(MODULE_SRCDIR)/rules.mk

ifeq ($(filter banjo,$(MODULE_EXPORT)),banjo)
MODULE_PACKAGE += $(sort $(MODULE_PACKAGE) banjo)
endif

# TODO(mcgrathr): make this error after bogons are fixed
ifneq ($(MODULE_BANJO_LIBRARY),$(subst -,.,$(notdir $(MODULE))))
$(error system/banjo/foo-bar-baz name must match library name foo.bar.baz)
$(error $(MODULE_BANJO_LIBRARY) vs $(notdir $(MODULE)) should be $(subst .,-,$(notdir $(MODULE_BANJO_LIBRARY))))
endif

ifneq ($(strip $(MODULE_PACKAGE)),)

MODULE_PKG_FILE := $(MODULE_BUILDDIR)/$(MODULE_NAME).pkg
MODULE_EXP_FILE := $(BUILDDIR)/export/$(MODULE_NAME).pkg

MODULE_PKG_SRCS := $(MODULE_SRCS)
MODULE_PKG_DEPS := $(MODULE_BANJO_DEPS)

ifneq ($(strip $(MODULE_PKG_DEPS)),)
MODULE_PKG_DEPS := $(foreach dep,$(MODULE_BANJO_DEPS),$(patsubst system/banjo/%,%,$(dep))=SOURCE/$(dep))
endif

ifneq ($(strip $(MODULE_PKG_SRCS)),)
MODULE_PKG_SRCS := $(foreach src,$(MODULE_PKG_SRCS),$(patsubst $(MODULE_SRCDIR)/%,%,$(src))=SOURCE/$(src))
MODULE_PKG_TAG := "[banjo]"
endif

$(MODULE_PKG_FILE): _NAME := $(MODULE_NAME)
$(MODULE_PKG_FILE): _LIBRARY := $(MODULE_BANJO_LIBRARY)
$(MODULE_PKG_FILE): _SRCS := $(if $(MODULE_PKG_SRCS),$(MODULE_PKG_TAG) $(sort $(MODULE_PKG_SRCS)))
$(MODULE_PKG_FILE): _DEPS := $(if $(MODULE_PKG_DEPS),"[banjo-deps]" $(sort $(MODULE_PKG_DEPS)))
$(MODULE_PKG_FILE): $(MODULE_RULESMK) make/module-banjo.mk
	@$(call BUILDECHO,creating banjo library package $@ ;)\
	$(MKDIR) ;\
	echo "[package]" > $@ ;\
	echo "name=$(_NAME)" >> $@ ;\
	echo "library=$(_LIBRARY)" >> $@ ;\
	echo "arch=banjo" >> $@ ;\
	echo "type=banjo" >> $@ ;\
	for i in $(_SRCS) ; do echo "$$i" >> $@ ; done ;\
	for i in $(_DEPS) ; do echo "$$i" >> $@ ; done


$(MODULE_EXP_FILE): $(MODULE_PKG_FILE)
	@$(MKDIR) ;\
	if [ -f "$@" ]; then \
		if ! cmp "$<" "$@" >/dev/null 2>&1; then \
			$(if $(BUILDECHO),echo installing $@ ;)\
			cp -f $< $@; \
		fi \
	else \
		$(if $(BUILDECHO),echo installing $@ ;)\
		cp -f $< $@; \
	fi
GENERATED += $(MODULE_EXP_FILE) $(MODULE_PKG_FILE)
ALLPKGS += $(MODULE_EXP_FILE)

endif # // ifneq ($(strip $(MODULE_PACKAGE)),)
