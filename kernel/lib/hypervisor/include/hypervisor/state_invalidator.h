
#pragma once

namespace hypervisor {

// Allows hypervisor state to be invalidated.
struct StateInvalidator {
    virtual ~StateInvalidator() = default;
    virtual void Invalidate() = 0;
};

} // namespace hypervisor
