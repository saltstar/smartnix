
#include "e1000_api.h"

/*
 * NOTE: the following routines using the e1000
 *  naming style are provided to the shared
 *  code but are OS specific
 */

void e1000_write_pci_cfg(struct e1000_hw* hw, u32 reg, u16* value) {
    pci_config_write16(hw2pci(hw), reg, *value);
}

void e1000_read_pci_cfg(struct e1000_hw* hw, u32 reg, u16* value) {
    pci_config_read16(hw2pci(hw), reg, value);
}

void e1000_pci_set_mwi(struct e1000_hw* hw) {
    pci_config_write16(hw2pci(hw), PCI_CONFIG_COMMAND, (hw->bus.pci_cmd_word | CMD_MEM_WRT_INVALIDATE));
}

void e1000_pci_clear_mwi(struct e1000_hw* hw) {
    pci_config_write16(hw2pci(hw), PCI_CONFIG_COMMAND, (hw->bus.pci_cmd_word & ~CMD_MEM_WRT_INVALIDATE));
}

/*
 * Read the PCI Express capabilities
 */
int32_t e1000_read_pcie_cap_reg(struct e1000_hw* hw, u32 reg, u16* value) {
    pci_protocol_t* pci = hw2pci(hw);
    uint8_t offset = pci_get_first_capability(pci, PCI_CAP_ID_PCI_EXPRESS);

    pci_config_read16(pci, offset + reg, value);
    return E1000_SUCCESS;
}

/*
 * Write the PCI Express capabilities
 */
int32_t e1000_write_pcie_cap_reg(struct e1000_hw* hw, u32 reg, u16* value) {
    pci_protocol_t* pci = hw2pci(hw);
    uint8_t offset = pci_get_first_capability(pci, PCI_CAP_ID_PCI_EXPRESS);

    pci_config_write16(pci, offset + reg, *value);
    return E1000_SUCCESS;
}
