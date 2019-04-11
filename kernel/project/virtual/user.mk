
# modules needed to implement user space

KERNEL_DEFINES += WITH_DEBUG_LINEBUFFER=1

MODULES += \
    kernel/lib/userboot \
    kernel/lib/debuglog \
    kernel/lib/ktrace \
    kernel/lib/mtrace \
    kernel/object \
    kernel/syscalls \

# include all banjo, core, dev, fidl, uapp, ulib and utest from system/...
MODULES += $(patsubst %/rules.mk,%,$(wildcard system/banjo/*/rules.mk))
MODULES += $(patsubst %/rules.mk,%,$(wildcard system/core/*/rules.mk))
MODULES += $(patsubst %/rules.mk,%,$(wildcard system/dev/*/*/rules.mk))
MODULES += $(patsubst %/rules.mk,%,$(wildcard system/fidl/*/rules.mk))
MODULES += $(patsubst %/rules.mk,%,$(wildcard system/uapp/*/rules.mk))
MODULES += $(patsubst %/rules.mk,%,$(wildcard system/ulib/*/rules.mk))
ifeq ($(call TOBOOL,$(DISABLE_UTEST)),false)
MODULES += $(patsubst %/rules.mk,%,$(wildcard system/utest/*/rules.mk))
endif

# include all uapp, udev, ulib and utest from third_party/...
MODULES += $(patsubst %/rules.mk,%,$(wildcard third_party/uapp/*/rules.mk))
MODULES += $(patsubst %/rules.mk,%,$(wildcard third_party/dev/*/*/rules.mk))
MODULES += $(patsubst %/rules.mk,%,$(wildcard third_party/ulib/*/rules.mk))
ifeq ($(call TOBOOL,$(DISABLE_UTEST)),false)
MODULES += $(patsubst %/rules.mk,%,$(wildcard third_party/utest/*/rules.mk))
endif
