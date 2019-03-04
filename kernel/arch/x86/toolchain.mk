
# x86-64 toolchain
ifndef ARCH_x86_64_TOOLCHAIN_INCLUDED
ARCH_x86_64_TOOLCHAIN_INCLUDED := 1

ifndef ARCH_x86_64_TOOLCHAIN_PREFIX
ARCH_x86_64_TOOLCHAIN_PREFIX := x86_64-elf-
endif
FOUNDTOOL=$(shell which $(ARCH_x86_64_TOOLCHAIN_PREFIX)gcc)

ifeq ($(FOUNDTOOL),)
$(error cannot find toolchain, please set ARCH_x86_64_TOOLCHAIN_PREFIX or add it to your path)
endif

endif # ifndef ARCH_x86_64_TOOLCHAIN_INCLUDED

# Clang
ifeq ($(call TOBOOL,$(USE_CLANG)),true)
FOUNDTOOL=$(shell which $(CLANG_TOOLCHAIN_PREFIX)clang)
ifeq ($(FOUNDTOOL),)
$(error cannot find toolchain, please set CLANG_TOOLCHAIN_PREFIX or add it to your path)
endif
endif # USE_CLANG==true
