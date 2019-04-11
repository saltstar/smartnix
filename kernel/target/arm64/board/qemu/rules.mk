
PLATFORM_BOARD_NAME := qemu

include kernel/target/arm64/boot-shim/rules.mk

# qemu needs a trampoline shim.
QEMU_BOOT_SHIM := $(BUILDDIR)/qemu-boot-shim.bin
EXTRA_BUILDDEPS += $(QEMU_BOOT_SHIM)
