
#include <dev/iommu/intel.h>
#include <ktl/move.h>

#include "iommu_impl.h"

zx_status_t IntelIommu::Create(ktl::unique_ptr<const uint8_t[]> desc, size_t desc_len,
                               fbl::RefPtr<Iommu>* out) {
    return intel_iommu::IommuImpl::Create(ktl::move(desc), desc_len, out);
}
