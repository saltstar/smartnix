
ifeq ($(call TOBOOL, $(ENABLE_USER_PCI)), false)
LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.cpp \
	$(LOCAL_DIR)/pci_config.cpp \
	$(LOCAL_DIR)/pcie_bridge.cpp \
	$(LOCAL_DIR)/pcie_bus_driver.cpp \
	$(LOCAL_DIR)/pcie_caps.cpp \
	$(LOCAL_DIR)/pcie_device.cpp \
	$(LOCAL_DIR)/pcie_irqs.cpp \
	$(LOCAL_DIR)/pcie_quirks.cpp \
	$(LOCAL_DIR)/pcie_root.cpp \
	$(LOCAL_DIR)/pcie_upstream_node.cpp \
	$(LOCAL_DIR)/address_provider/mmio.cpp \
	$(LOCAL_DIR)/address_provider/pio.cpp \
	$(LOCAL_DIR)/address_provider/designware.cpp \
	$(LOCAL_DIR)/address_provider/ecam_region.cpp \


MODULE_DEPS += \
    kernel/lib/zxcpp \
    kernel/lib/fbl \
    kernel/lib/pci \
    kernel/lib/region-alloc

MODULE_CPPFLAGS += -Wno-invalid-offsetof
include make/module.mk
endif
