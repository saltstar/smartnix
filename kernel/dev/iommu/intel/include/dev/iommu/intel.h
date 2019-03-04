
#pragma once

#include <dev/iommu.h>
#include <fbl/ref_ptr.h>
#include <zircon/syscalls/iommu.h>

class IntelIommu {
public:
    static zx_status_t Create(fbl::unique_ptr<const uint8_t[]> desc, size_t desc_len,
                              fbl::RefPtr<Iommu>* out);
};
