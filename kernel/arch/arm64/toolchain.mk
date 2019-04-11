
# arm64 GCC toolchain
ifndef ARCH_arm64_TOOLCHAIN_INCLUDED
ARCH_arm64_TOOLCHAIN_INCLUDED := 1

ifndef ARCH_arm64_TOOLCHAIN_PREFIX
ARCH_arm64_TOOLCHAIN_PREFIX := aarch64-elf-
endif
FOUNDTOOL=$(shell which $(ARCH_arm64_TOOLCHAIN_PREFIX)gcc)

endif # ifndef ARCH_arm64_TOOLCHAIN_INCLUDED

# Clang
ifeq ($(call TOBOOL,$(USE_CLANG)),true)
FOUNDTOOL=$(shell which $(CLANG_TOOLCHAIN_PREFIX)clang)
endif # USE_CLANG==true

ifeq ($(FOUNDTOOL),)
$(error cannot find toolchain, please set ARCH_arm64_TOOLCHAIN_PREFIX, \
        CLANG_TOOLCHAIN_PREFIX, or add either to your path)
endif
