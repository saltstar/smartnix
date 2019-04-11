// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <ddk/binding.h>
#include <ddk/device.h>
#include <fbl/intrusive_double_list.h>
#include <fbl/string.h>
#include <fbl/unique_ptr.h>
#include <fbl/vector.h>
#include <lib/async/cpp/wait.h>
#include <lib/zx/channel.h>
#include <lib/zx/event.h>
#include <lib/zx/job.h>
#include <lib/zx/process.h>
#include <lib/zx/socket.h>
#include <lib/zx/vmo.h>

#include <utility>

#include "device.h"
#include "driver.h"
#include "metadata.h"

namespace devmgr {

class DevhostLoaderService;

class Devhost {
public:
    enum Flags : uint32_t {
        kDying = 1 << 0,
        kSuspend = 1 << 1,
    };

    struct AllDevhostsNode {
        static fbl::DoublyLinkedListNodeState<Devhost*>& node_state(Devhost& obj) {
            return obj.anode_;
        }
    };
    struct SuspendNode {
        static fbl::DoublyLinkedListNodeState<Devhost*>& node_state(Devhost& obj) {
            return obj.snode_;
        }
    };
    struct Node {
        static fbl::DoublyLinkedListNodeState<Devhost*>& node_state(Devhost& obj) {
            return obj.node_;
        }
    };

    zx_handle_t hrpc() const { return hrpc_; }
    void set_hrpc(zx_handle_t hrpc) { hrpc_ = hrpc; }
    zx::unowned_process proc() const { return zx::unowned_process(proc_); }
    void set_proc(zx_handle_t proc) { proc_.reset(proc); }
    zx_koid_t koid() const { return koid_; }
    void set_koid(zx_koid_t koid) { koid_ = koid; }
    // Note: this is a non-const reference to make |= etc. ergonomic.
    uint32_t& flags() { return flags_; }
    Devhost* parent() const { return parent_; }
    void set_parent(Devhost* parent) { parent_ = parent; }
    fbl::DoublyLinkedList<Device*, Device::DevhostNode>& devices() { return devices_; }
    fbl::DoublyLinkedList<Devhost*, Node>& children() { return children_; }

    // The AddRef and Release functions follow the contract for fbl::RefPtr.
    void AddRef() const { ++refcount_; }

    // Returns true when the last reference has been released.
    bool Release() const {
        const int32_t rc = refcount_;
        --refcount_;
        return rc == 1;
    }

private:
    async::Wait wait_;
    zx_handle_t hrpc_ = ZX_HANDLE_INVALID;
    zx::process proc_;
    zx_koid_t koid_ = 0;
    mutable int32_t refcount_ = 0;
    uint32_t flags_ = 0;
    Devhost* parent_ = nullptr;

    // list of all devices on this devhost
    fbl::DoublyLinkedList<Device*, Device::DevhostNode> devices_;

    // listnode for this devhost in the all devhosts list
    fbl::DoublyLinkedListNodeState<Devhost*> anode_;

    // listnode for this devhost in the order-to-suspend list
    fbl::DoublyLinkedListNodeState<Devhost*> snode_;

    // listnode for this devhost in its parent devhost's list-of-children
    fbl::DoublyLinkedListNodeState<Devhost*> node_;

    // list of all child devhosts of this devhost
    fbl::DoublyLinkedList<Devhost*, Node> children_;
};

class SuspendContext {
public:
    enum class Flags : uint32_t {
        kRunning = 0u,
        kSuspend = 1u,
    };

    SuspendContext() = default;

    SuspendContext(Coordinator* coordinator, Flags flags, uint32_t sflags, zx::socket socket,
                   zx::vmo kernel = zx::vmo(), zx::vmo bootdata = zx::vmo())
        : coordinator_(coordinator), flags_(flags), sflags_(sflags), socket_(std::move(socket)),
          kernel_(std::move(kernel)), bootdata_(std::move(bootdata)) {}

    ~SuspendContext() { devhosts_.clear(); }

    SuspendContext(SuspendContext&&) = default;
    SuspendContext& operator=(SuspendContext&&) = default;

    Coordinator* coordinator() { return coordinator_; }

    zx_status_t status() const { return status_; }
    void set_status(zx_status_t status) { status_ = status; }
    Flags flags() const { return flags_; }
    void set_flags(Flags flags) { flags_ = flags; }
    uint32_t sflags() const { return sflags_; }

    Devhost* dh() const { return dh_; }
    void set_dh(Devhost* dh) { dh_ = dh; }

    using DevhostList = fbl::DoublyLinkedList<Devhost*, Devhost::SuspendNode>;
    DevhostList& devhosts() { return devhosts_; }
    const DevhostList& devhosts() const { return devhosts_; }

    const zx::vmo& kernel() const { return kernel_; }
    const zx::vmo& bootdata() const { return bootdata_; }

    // Close the socket whose ownership was handed to this SuspendContext.
    void CloseSocket() { socket_.reset(); }

    // The AddRef and Release functions follow the contract for fbl::RefPtr.
    void AddRef() const { ++count_; }

    // Returns true when the last message reference has been released.
    bool Release() const {
        const int32_t rc = count_;
        --count_;
        return rc == 1;
    }

private:
    Coordinator* coordinator_ = nullptr;

    zx_status_t status_ = ZX_OK;
    Flags flags_ = Flags::kRunning;

    // suspend flags
    uint32_t sflags_ = 0u;
    // outstanding msgs
    mutable uint32_t count_ = 0u;
    // next devhost to process
    Devhost* dh_ = nullptr;
    fbl::DoublyLinkedList<Devhost*, Devhost::SuspendNode> devhosts_;

    // socket to notify on for 'dm reboot' and 'dm poweroff'
    zx::socket socket_;

    // mexec arguments
    zx::vmo kernel_;
    zx::vmo bootdata_;
};

// clang-format off

// This device is never destroyed
#define DEV_CTX_IMMORTAL      0x01

// This device requires that children are created in a
// new devhost attached to a proxy device
#define DEV_CTX_MUST_ISOLATE  0x02

// This device may be bound multiple times
#define DEV_CTX_MULTI_BIND    0x04

// This device is bound and not eligible for binding
// again until unbound.  Not allowed on MULTI_BIND ctx.
#define DEV_CTX_BOUND         0x08

// Device has been remove()'d
#define DEV_CTX_DEAD          0x10

// Device has been removed but its rpc channel is not
// torn down yet.  The rpc transport will call remove
// when it notices at which point the device will leave
// the zombie state and drop the reference associated
// with the rpc channel, allowing complete destruction.
#define DEV_CTX_ZOMBIE        0x20

// Device is a proxy -- its "parent" is the device it's
// a proxy to.
#define DEV_CTX_PROXY         0x40

// Device is not visible in devfs or bindable.
// Devices may be created in this state, but may not
// return to this state once made visible.
#define DEV_CTX_INVISIBLE     0x80

// clang-format on

zx_status_t devfs_publish(Device* parent, Device* dev);
void devfs_unpublish(Device* dev);
void devfs_advertise(Device* dev);
void devfs_advertise_modified(Device* dev);
zx_status_t devfs_connect(Device* dev, zx::channel client_remote);

// Values parsed out of argv.  All paths described below are absolute paths.
struct DevmgrArgs {
    // Load drivers from these directories.  If this is empty, the default will
    // be used.
    fbl::Vector<const char*> driver_search_paths;
    // Load the drivers with these paths.  The specified drivers do not need to
    // be in directories in |driver_search_paths|.
    fbl::Vector<const char*> load_drivers;
    // Use this driver as the sys_device driver.  If nullptr, the default will
    // be used.
    const char* sys_device_driver = nullptr;
};

struct CoordinatorConfig {
    // Initial root resource from the kernel.
    zx::resource root_resource;
    // Job for sysinfo.
    zx::job sysinfo_job;
    // Job for all devhosts.
    zx::job devhost_job;
    // Event that controls the fshost.
    zx::event fshost_event;
    // Async dispatcher for the coordinator.
    async_dispatcher_t* dispatcher;
    // Whether we require /system.
    bool require_system;
    // Whether we require ASan drivers.
    bool asan_drivers;
    // Whether to reboot the device when suspend does not finish on time.
    bool suspend_fallback;
    // Whether to print out debugging when suspend does not finish on time.
    bool suspend_debug;
};

class Coordinator {
public:
    Coordinator(const Coordinator&) = delete;
    Coordinator& operator=(const Coordinator&) = delete;
    Coordinator(Coordinator&&) = delete;
    Coordinator& operator=(Coordinator&&) = delete;

    explicit Coordinator(CoordinatorConfig config);
    ~Coordinator();

    zx_status_t InitializeCoreDevices(const char* sys_device_driver);
    zx_status_t SetBootdata(const zx::unowned_vmo& vmo);
    bool InSuspend() const;

    void ScanSystemDrivers();
    void BindDrivers();
    void UseFallbackDrivers();
    void DriverAdded(Driver* drv, const char* version);
    void DriverAddedInit(Driver* drv, const char* version);
    zx_status_t LibnameToVmo(const fbl::String& libname, zx::vmo* out_vmo) const;

    // Used to implement fuchsia::device::manager::Coordinator.
    zx_status_t AddDevice(Device* parent, zx::channel rpc, const uint64_t* props_data,
                          size_t props_count, fbl::StringPiece name, uint32_t protocol_id,
                          fbl::StringPiece driver_path, fbl::StringPiece args, bool invisible,
                          zx::channel client_remote);
    zx_status_t RemoveDevice(Device* dev, bool forced);
    zx_status_t MakeVisible(Device* dev);
    zx_status_t BindDevice(Device* dev, fbl::StringPiece drvlibname);
    zx_status_t GetTopologicalPath(const Device* dev, char* out, size_t max) const;
    zx_status_t LoadFirmware(Device* dev, const char* path, zx::vmo* vmo, size_t* size);

    zx_status_t GetMetadata(Device* dev, uint32_t type, void* buffer, size_t buflen,
                            size_t* actual);
    zx_status_t GetMetadataSize(Device* dev, uint32_t type, size_t* size);
    zx_status_t AddMetadata(Device* dev, uint32_t type, const void* data, uint32_t length);
    zx_status_t PublishMetadata(Device* dev, const char* path, uint32_t type, const void* data,
                                uint32_t length);
    zx_status_t DmCommand(size_t len, const char* cmd);
    zx_status_t DmOpenVirtcon(zx::channel virtcon_receiver) const;
    void DmMexec(zx::vmo kernel, zx::vmo bootdata);

    zx_status_t HandleDeviceRead(Device* dev);
    void HandleNewDevice(Device* dev);
    zx_status_t PrepareProxy(Device* dev);
    void DumpState() const;

    const zx::resource& root_resource() const { return config_.root_resource; }
    const zx::event& fshost_event() const { return config_.fshost_event; }
    async_dispatcher_t* dispatcher() const { return config_.dispatcher; }
    bool require_system() const { return config_.require_system; }
    bool suspend_fallback() const { return config_.suspend_fallback; }
    bool suspend_debug() const { return config_.suspend_debug; }

    void set_running(bool running) { running_ = running; }
    bool system_available() const { return system_available_; }
    void set_system_available(bool system_available) { system_available_ = system_available; }
    bool system_loaded() const { return system_loaded_; }
    void set_loader_service(DevhostLoaderService* loader_service) {
        loader_service_ = loader_service;
    }
    void set_virtcon_channel(zx::channel virtcon_channel) {
        virtcon_channel_ = std::move(virtcon_channel);
    }
    void set_dmctl_socket(zx::socket dmctl_socket) { dmctl_socket_ = std::move(dmctl_socket); }

    fbl::DoublyLinkedList<Driver*, Driver::Node>& drivers() { return drivers_; }
    const fbl::DoublyLinkedList<Driver*, Driver::Node>& drivers() const { return drivers_; }
    fbl::DoublyLinkedList<Device*, Device::AllDevicesNode>& devices() { return devices_; }
    const fbl::DoublyLinkedList<Device*, Device::AllDevicesNode>& devices() const {
        return devices_;
    }

    Device& root_device() { return root_device_; }
    Device& misc_device() { return misc_device_; }
    Device& sys_device() { return sys_device_; }
    Device& test_device() { return test_device_; }

    SuspendContext& suspend_context() { return suspend_context_; }
    const SuspendContext& suspend_context() const { return suspend_context_; }

private:
    CoordinatorConfig config_;
    bool running_ = false;
    bool launched_first_devhost_ = false;
    bool system_available_ = false;
    bool system_loaded_ = false;
    DevhostLoaderService* loader_service_ = nullptr;

    // Channel for creating new virtual consoles.
    zx::channel virtcon_channel_;
    // This socket is used by DmPrintf for output, and DmPrintf can be called in
    // the context of a const member function, therefore it is also const. Given
    // that, we must make dmctl_socket_ mutable.
    mutable zx::socket dmctl_socket_;
    zx::vmo bootdata_vmo_;

    // All Drivers
    fbl::DoublyLinkedList<Driver*, Driver::Node> drivers_;

    // Drivers to try last
    fbl::DoublyLinkedList<Driver*, Driver::Node> fallback_drivers_;

    // List of drivers loaded from /system by system_driver_loader()
    fbl::DoublyLinkedList<Driver*, Driver::Node> system_drivers_;

    // All Devices (excluding static immortal devices)
    fbl::DoublyLinkedList<Device*, Device::AllDevicesNode> devices_;

    // All DevHosts
    fbl::DoublyLinkedList<Devhost*, Devhost::AllDevhostsNode> devhosts_;

    Device root_device_{this};
    Device misc_device_{this};
    Device sys_device_{this};
    Device test_device_{this};

    SuspendContext suspend_context_;

    fbl::DoublyLinkedList<fbl::unique_ptr<Metadata>, Metadata::Node> published_metadata_;

    void DmPrintf(const char* fmt, ...) const;
    void DumpDevice(const Device* dev, size_t indent) const;
    void DumpDeviceProps(const Device* dev) const;
    void DumpGlobalDeviceProps() const;
    void DumpDrivers() const;

    void BuildSuspendList();
    void Suspend(SuspendContext ctx);
    void Suspend(uint32_t flags);

    fbl::unique_ptr<Driver> ValidateDriver(fbl::unique_ptr<Driver> drv);
    const Driver* LibnameToDriver(const fbl::String& libname) const;

    zx_status_t NewDevhost(const char* name, Devhost* parent, Devhost** out);
    void ReleaseDevhost(Devhost* dh);
    void ReleaseDevice(Device* dev);

    zx_status_t BindDriver(Driver* drv);
    zx_status_t AttemptBind(const Driver* drv, Device* dev);
    void BindSystemDrivers();
    void DriverAddedSys(Driver* drv, const char* version);
};

bool dc_is_bindable(const Driver* drv, uint32_t protocol_id, zx_device_prop_t* props,
                    size_t prop_count, bool autobind);

// Methods for composing FIDL RPCs to the devhosts
zx_status_t dh_send_remove_device(const Device* dev);
zx_status_t dh_send_create_device(Device* dev, Devhost* dh, zx::channel rpc, zx::vmo driver,
                                  const char* args, zx::handle rpc_proxy);
zx_status_t dh_send_create_device_stub(Devhost* dh, zx::channel rpc, uint32_t protocol_id);
zx_status_t dh_send_bind_driver(Device* dev, const char* libname, zx::vmo driver);
zx_status_t dh_send_connect_proxy(const Device* dev, zx::channel proxy);
zx_status_t dh_send_suspend(const Device* dev, uint32_t flags);

} // namespace devmgr
