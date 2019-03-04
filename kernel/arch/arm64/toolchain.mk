
ifndef ARCH_arm64_TOOLCHAIN_INCLUDED
ARCH_arm64_TOOLCHAIN_INCLUDED := 1

ifndef ARCH_arm64_TOOLCHAIN_PREFIX
ARCH_arm64_TOOLCHAIN_PREFIX := aarch64-elf-
FOUNDTOOL=$(shell which $(ARCH_arm64_TOOLCHAIN_PREFIX)gcc)
ifeq ($(FOUNDTOOL),)
ARCH_arm64_TOOLCHAIN_PREFIX := aarch64-linux-android-
FOUNDTOOL=$(shell which $(ARCH_arm64_TOOLCHAIN_PREFIX)gcc)
ifeq ($(FOUNDTOOL),)
$(error cannot find toolchain, please set ARCH_arm64_TOOLCHAIN_PREFIX or add it to your path)
endif
endif
endif

endif

# Clang
ifeq ($(call TOBOOL,$(USE_CLANG)),true)
FOUNDTOOL=$(shell which $(CLANG_TOOLCHAIN_PREFIX)clang)
ifeq ($(FOUNDTOOL),)
$(error cannot find toolchain, please set CLANG_TOOLCHAIN_PREFIX or add it to your path)
endif
endif
