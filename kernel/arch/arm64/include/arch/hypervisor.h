
#pragma once

#include <arch/arm64/hypervisor/el2_state.h>
#include <fbl/ref_ptr.h>
#include <ktl/unique_ptr.h>
#include <hypervisor/guest_physical_address_space.h>
#include <hypervisor/id_allocator.h>
#include <hypervisor/interrupt_tracker.h>
#include <hypervisor/page.h>
#include <hypervisor/trap_map.h>
#include <kernel/event.h>
#include <kernel/spinlock.h>
#include <zircon/types.h>

static constexpr uint8_t kNumGroups = 2;
// See CoreLink GIC-400, Section 2.3.2 PPIs.
static constexpr uint32_t kMaintenanceVector = 25;
static constexpr uint32_t kTimerVector = 27;
static constexpr uint16_t kNumInterrupts = 256;
static_assert(kMaintenanceVector < kNumInterrupts, "Maintenance vector is out of range");
static_assert(kTimerVector < kNumInterrupts, "Timer vector is out of range");

typedef struct zx_port_packet zx_port_packet_t;
class PortDispatcher;

class Guest {
public:
    static zx_status_t Create(ktl::unique_ptr<Guest>* out);
    ~Guest();
    DISALLOW_COPY_ASSIGN_AND_MOVE(Guest);

    zx_status_t SetTrap(uint32_t kind, zx_vaddr_t addr, size_t len,
                        fbl::RefPtr<PortDispatcher> port, uint64_t key);

    hypervisor::GuestPhysicalAddressSpace* AddressSpace() const { return gpas_.get(); }
    hypervisor::TrapMap* Traps() { return &traps_; }
    uint8_t Vmid() const { return vmid_; }

    zx_status_t AllocVpid(uint8_t* vpid);
    zx_status_t FreeVpid(uint8_t vpid);

private:
    ktl::unique_ptr<hypervisor::GuestPhysicalAddressSpace> gpas_;
    hypervisor::TrapMap traps_;
    const uint8_t vmid_;

    fbl::Mutex vcpu_mutex_;
    // TODO(alexlegg): Find a good place for this constant to live (max vcpus).
    hypervisor::IdAllocator<uint8_t, 8> TA_GUARDED(vcpu_mutex_) vpid_allocator_;

    explicit Guest(uint8_t vmid);
};

// Stores the state of the GICH across VM exits.
struct GichState {
    // Tracks pending interrupts.
    hypervisor::InterruptTracker<kNumInterrupts> interrupt_tracker;
    // Tracks active interrupts.
    bitmap::RawBitmapGeneric<bitmap::FixedStorage<kNumInterrupts>> active_interrupts;
};

// Loads GICH within a given scope.
class AutoGich {
public:
    AutoGich(IchState* ich_state, bool pending);
    ~AutoGich();

private:
    IchState* ich_state_;
};

class Vcpu {
public:
    static zx_status_t Create(Guest* guest, zx_vaddr_t entry, ktl::unique_ptr<Vcpu>* out);
    ~Vcpu();
    DISALLOW_COPY_ASSIGN_AND_MOVE(Vcpu);

    zx_status_t Resume(zx_port_packet_t* packet);
    cpu_mask_t Interrupt(uint32_t vector, hypervisor::InterruptType type);
    void VirtualInterrupt(uint32_t vector);
    zx_status_t ReadState(uint32_t kind, void* buf, size_t len) const;
    zx_status_t WriteState(uint32_t kind, const void* buf, size_t len);

private:
    Guest* guest_;
    const uint8_t vpid_;
    const thread_t* thread_;
    fbl::atomic_bool running_;
    // We allocate El2State in its own page as it is passed between EL1 and EL2,
    // which have different address space mappings. This ensures that El2State
    // will not cross a page boundary and be incorrectly accessed in EL2.
    hypervisor::PagePtr<El2State> el2_state_;
    GichState gich_state_;
    uint64_t hcr_;

    Vcpu(Guest* guest, uint8_t vpid, const thread_t* thread);
};
