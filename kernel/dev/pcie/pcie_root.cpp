
#include <dev/pcie_root.h>

#define LOCAL_TRACE 0

PcieRoot::PcieRoot(PcieBusDriver& bus_drv, uint mbus_id)
    : PcieUpstreamNode(bus_drv, PcieUpstreamNode::Type::ROOT, mbus_id),
      bus_drv_(bus_drv) {
}
