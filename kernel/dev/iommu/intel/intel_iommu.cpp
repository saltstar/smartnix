
#include <dev/iommu/intel.h>
#include "iommu_impl.h"

zx_status_t IntelIommu::Create(fbl::unique_ptr<const uint8_t[]> desc, size_t desc_len,
                               fbl::RefPtr<Iommu>* out) {
    return intel_iommu::IommuImpl::Create(fbl::move(desc), desc_len, out);
}
