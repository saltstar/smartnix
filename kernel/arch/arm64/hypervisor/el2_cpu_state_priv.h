
#pragma once

#include <fbl/array.h>
#include <ktl/unique_ptr.h>
#include <hypervisor/id_allocator.h>
#include <hypervisor/page.h>
#include <kernel/mp.h>

class El2TranslationTable {
public:
    zx_status_t Init();
    zx_paddr_t Base() const;

private:
    hypervisor::Page l0_page_;
    hypervisor::Page l1_page_;
};

// Represents a stack for use with EL2/
class El2Stack {
public:
    zx_status_t Alloc();
    zx_paddr_t Top() const;

private:
    hypervisor::Page page_;
};

// Maintains the EL2 state for each CPU.
class El2CpuState : public hypervisor::IdAllocator<uint8_t, 64> {
public:
    static zx_status_t Create(ktl::unique_ptr<El2CpuState>* out);
    ~El2CpuState();

private:
    cpu_mask_t cpu_mask_ = 0;
    El2TranslationTable table_;
    fbl::Array<El2Stack> stacks_;

    El2CpuState() = default;

    static zx_status_t OnTask(void* context, uint cpu_num);
};

// Allocate and free virtual machine IDs.
zx_status_t alloc_vmid(uint8_t* vmid);
zx_status_t free_vmid(uint8_t vmid);

// Allocate and free virtual processor IDs.
zx_status_t alloc_vpid(uint8_t* vpid);
zx_status_t free_vpid(uint8_t vpid);
