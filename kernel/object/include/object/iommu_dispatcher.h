
#pragma once

#include <zircon/rights.h>

#include <dev/iommu.h>
#include <object/dispatcher.h>
#include <fbl/canary.h>

#include <sys/types.h>

class IommuDispatcher final : public SoloDispatcher<IommuDispatcher, ZX_DEFAULT_IOMMU_RIGHTS> {
public:
    static zx_status_t Create(uint32_t type, fbl::unique_ptr<const uint8_t[]> desc,
                              size_t desc_len, fbl::RefPtr<Dispatcher>* dispatcher,
                              zx_rights_t* rights);

    ~IommuDispatcher() final;
    zx_obj_type_t get_type() const final { return ZX_OBJ_TYPE_IOMMU; }

    fbl::RefPtr<Iommu> iommu() const { return iommu_; }

private:
    explicit IommuDispatcher(fbl::RefPtr<Iommu> iommu);

    fbl::Canary<fbl::magic("IOMD")> canary_;
    const fbl::RefPtr<Iommu> iommu_;
};
