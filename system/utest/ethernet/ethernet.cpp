// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/protocol/ethernet.h>
#include <fbl/algorithm.h>
#include <fbl/auto_call.h>
#include <fbl/intrusive_single_list.h>
#include <fbl/unique_ptr.h>
#include <fbl/vector.h>
#include <fuchsia/hardware/ethernet/c/fidl.h>
#include <lib/fdio/util.h>
#include <lib/fdio/watcher.h>
#include <lib/fzl/fdio.h>
#include <lib/fzl/fifo.h>
#include <lib/zx/channel.h>
#include <lib/zx/socket.h>
#include <lib/zx/time.h>
#include <lib/zx/vmar.h>
#include <lib/zx/vmo.h>
#include <unittest/unittest.h>
#include <zircon/compiler.h>
#include <zircon/device/device.h>
#include <zircon/device/ethertap.h>
#include <zircon/status.h>
#include <zircon/types.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <utility>

// Delay for data to work through the system. The test will pause this long, so it's best
// to keep it fairly short. If it's too short, the test will occasionally be flaky,
// especially on qemu.
static constexpr zx::duration kPropagateDuration = zx::msec(200);
#define PROPAGATE_TIME (zx::deadline_after(kPropagateDuration))

// We expect something to happen prior to timeout, and the test will fail if it doesn't. So
// wait longer to further reduce the likelihood of test flakiness.
#define FAIL_TIMEOUT (zx::deadline_after(kPropagateDuration * 50))

// Because of test flakiness if a previous test case's ethertap device isn't cleaned up, we put a
// delay at the end of each test to give devmgr time to clean up the ethertap devices.
#define ETHTEST_CLEANUP_DELAY zx::nanosleep(PROPAGATE_TIME)

namespace {

const char kEthernetDir[] = "/dev/class/ethernet";
const char kTapctl[] = "/dev/misc/tapctl";
const uint8_t kTapMac[] = { 0x12, 0x20, 0x30, 0x40, 0x50, 0x60 };

const char* mxstrerror(zx_status_t status) {
    return zx_status_get_string(status);
}

zx_status_t CreateEthertapWithOption(uint32_t mtu, const char* name, zx::socket* sock,
                                     uint32_t options) {
    if (sock == nullptr) {
        return ZX_ERR_INVALID_ARGS;
    }

    fbl::unique_fd ctlfd(open(kTapctl, O_RDONLY));
    if (!ctlfd.is_valid()) {
        fprintf(stderr, "could not open %s: %s\n", kTapctl, strerror(errno));
        return ZX_ERR_IO;
    }

    ethertap_ioctl_config_t config = {};
    strlcpy(config.name, name, ETHERTAP_MAX_NAME_LEN);
    config.options = options;
    // Uncomment this to trace ETHERTAP events
    //config.options |= ETHERTAP_OPT_TRACE;
    config.mtu = mtu;
    memcpy(config.mac, kTapMac, 6);
    ssize_t rc = ioctl_ethertap_config(ctlfd.get(), &config, sock->reset_and_get_address());
    if (rc < 0) {
        zx_status_t status = static_cast<zx_status_t>(rc);
        fprintf(stderr, "could not configure ethertap device: %s\n", mxstrerror(status));
        return status;
    }
    return ZX_OK;
}

zx_status_t CreateEthertap(uint32_t mtu, const char* name, zx::socket* sock) {
    return CreateEthertapWithOption(mtu, name, sock, 0);
}

zx_status_t WatchCb(int dirfd, int event, const char* fn, void* cookie) {
    if (event != WATCH_EVENT_ADD_FILE) return ZX_OK;
    if (!strcmp(fn, ".") || !strcmp(fn, "..")) return ZX_OK;

    zx::channel svc;
    {
        int devfd = openat(dirfd, fn, O_RDONLY);
        if (devfd < 0) {
            return ZX_OK;
        }

        zx_status_t status = fdio_get_service_handle(devfd, svc.reset_and_get_address());
        if (status != ZX_OK) {
            return status;
        }
    }

    // See if this device is our ethertap device
    fuchsia_hardware_ethernet_Info info;
    zx_status_t status = fuchsia_hardware_ethernet_DeviceGetInfo(svc.get(), &info);
    if (status != ZX_OK) {
        fprintf(stderr, "could not get ethernet info for %s/%s: %s\n", kEthernetDir, fn,
                mxstrerror(status));
        // Return ZX_OK to keep watching for devices.
        return ZX_OK;
    }
    if (!(info.features & fuchsia_hardware_ethernet_INFO_FEATURE_SYNTH)) {
        // Not a match, keep looking.
        return ZX_OK;
    }

    // Found it!
    // TODO(tkilbourn): this might not be the test device we created; need a robust way of getting
    // the name of the tap device to check. Note that ioctl_device_get_device_name just returns
    // "ethernet" since that's the child of the tap device that we've opened here.
    auto svcp = reinterpret_cast<zx_handle_t*>(cookie);
    *svcp = svc.release();
    return ZX_ERR_STOP;
}

zx_status_t OpenEthertapDev(zx::channel* svc) {
    if (svc == nullptr) {
        return ZX_ERR_INVALID_ARGS;
    }

    int ethdir = open(kEthernetDir, O_RDONLY);
    if (ethdir < 0) {
        fprintf(stderr, "could not open %s: %s\n", kEthernetDir, strerror(errno));
        return ZX_ERR_IO;
    }

    zx_status_t status;
    status = fdio_watch_directory(ethdir, WatchCb, zx_deadline_after(ZX_SEC(2)),
                                  reinterpret_cast<void*>(svc->reset_and_get_address()));
    if (status == ZX_ERR_STOP) {
        return ZX_OK;
    } else {
        return status;
    }
}

struct FifoEntry : public fbl::SinglyLinkedListable<fbl::unique_ptr<FifoEntry>> {
    fuchsia_hardware_ethernet_FifoEntry e;
};

struct EthernetOpenInfo {
    EthernetOpenInfo(const char* name) : name(name) {}
    // Special setup until we have IGMP: turn off multicast-promisc in init.
    bool multicast = false;
    const char* name;
    bool online = true;
    uint32_t options = 0;
};

class EthernetClient {
public:
    EthernetClient() {}
    ~EthernetClient() {
        Cleanup();
    }

    void Cleanup() {
        if (mapped_ > 0) {
            zx::vmar::root_self()->unmap(mapped_, vmo_size_);
        }
        svc_.reset();
    }

    zx_status_t Register(zx::channel svc, const char* name, uint32_t nbufs, uint16_t bufsize) {
        svc_ = std::move(svc);
        zx_status_t call_status = ZX_OK;
        size_t name_len =
            fbl::min<size_t>(strlen(name), fuchsia_hardware_ethernet_MAX_CLIENT_NAME_LEN);
        zx_status_t status =
            fuchsia_hardware_ethernet_DeviceSetClientName(svc_.get(), name, name_len, &call_status);
        if (status != ZX_OK || call_status != ZX_OK) {
            fprintf(stderr, "could not set client name to %s: %d, %d\n", name, status, call_status);
            return status == ZX_OK ? call_status : status;
        }

        fuchsia_hardware_ethernet_Fifos fifos;
        status = fuchsia_hardware_ethernet_DeviceGetFifos(svc_.get(), &call_status, &fifos);
        if (status != ZX_OK || call_status != ZX_OK) {
            fprintf(stderr, "could not get fifos: %d, %d\n", status, call_status);
            return status == ZX_OK ? call_status : status;
        }

        tx_.reset(fifos.tx);
        rx_.reset(fifos.rx);
        tx_depth_ = fifos.tx_depth;
        rx_depth_ = fifos.rx_depth;

        nbufs_ = nbufs;
        bufsize_ = bufsize;

        vmo_size_ = 2 * nbufs_ * bufsize_;
        status = zx::vmo::create(vmo_size_, ZX_VMO_NON_RESIZABLE, &buf_);
        if (status != ZX_OK) {
            fprintf(stderr, "could not create a vmo of size %" PRIu64 ": %s\n", vmo_size_,
                    mxstrerror(status));
            return status;
        }

        status = zx::vmar::root_self()->map(0, buf_, 0, vmo_size_,
                                            ZX_VM_PERM_READ | ZX_VM_PERM_WRITE,
                                            &mapped_);
        if (status != ZX_OK) {
            fprintf(stderr, "failed to map vmo: %s\n", mxstrerror(status));
            return status;
        }

        zx::vmo buf_copy;
        status = buf_.duplicate(ZX_RIGHT_SAME_RIGHTS, &buf_copy);
        if (status != ZX_OK) {
            fprintf(stderr, "failed to duplicate vmo: %s\n", mxstrerror(status));
            return status;
        }

        zx_handle_t bufh = buf_copy.release();
        status = fuchsia_hardware_ethernet_DeviceSetIOBuffer(svc_.get(), bufh, &call_status);
        if (status != ZX_OK || call_status != ZX_OK) {
            fprintf(stderr, "failed to set eth iobuf: %d, %d\n", status, call_status);
            return status == ZX_OK ? call_status : status;
        }

        uint32_t idx = 0;
        for (; idx < nbufs; idx++) {
            fuchsia_hardware_ethernet_FifoEntry entry = {
                .offset = idx * bufsize_,
                .length = bufsize_,
                .flags = 0,
                .cookie = 0,
            };
            status = rx_.write_one(entry);
            if (status != ZX_OK) {
                fprintf(stderr, "failed call to write(): %s\n", mxstrerror(status));
                return status;
            }
        }

        for (; idx < 2 * nbufs; idx++) {
            auto entry = fbl::unique_ptr<FifoEntry>(new FifoEntry);
            entry->e.offset = idx * bufsize_;
            entry->e.length = bufsize_;
            entry->e.flags = 0;
            entry->e.cookie = reinterpret_cast<uintptr_t>(mapped_) + entry->e.offset;
            tx_available_.push_front(std::move(entry));
        }

        return ZX_OK;
    }

    zx_status_t Start() {
        zx_status_t call_status = ZX_OK;
        zx_status_t status = fuchsia_hardware_ethernet_DeviceStart(svc_.get(), &call_status);
        if (status != ZX_OK) {
            return status;
        }
        return call_status;
    }

    zx_status_t Stop() { return fuchsia_hardware_ethernet_DeviceStop(svc_.get()); }

    zx_status_t GetStatus(uint32_t* eth_status) {
        return fuchsia_hardware_ethernet_DeviceGetStatus(svc_.get(), eth_status);
    }

    zx_status_t SetPromisc(bool on) {
        zx_status_t call_status = ZX_OK;
        zx_status_t status =
            fuchsia_hardware_ethernet_DeviceSetPromiscuousMode(svc_.get(), on, &call_status);
        if (status != ZX_OK) {
            return status;
        }
        return call_status;
    }

    zx_status_t SetMulticastPromisc(bool on) {
        zx_status_t call_status, status;
        status = fuchsia_hardware_ethernet_DeviceConfigMulticastSetPromiscuousMode(svc_.get(), on,
                                                                                   &call_status);
        if (status != ZX_OK) {
            return status;
        }
        return call_status;
    }

    zx_status_t MulticastAddressAdd(uint8_t* mac_addr) {
        fuchsia_hardware_ethernet_MacAddress mac;
        memcpy(mac.octets, mac_addr, 6);

        zx_status_t call_status, status;
        status =
            fuchsia_hardware_ethernet_DeviceConfigMulticastAddMac(svc_.get(), &mac, &call_status);
        if (status != ZX_OK) {
            return status;
        }
        return call_status;
    }

    zx_status_t MulticastAddressDel(uint8_t* mac_addr) {
        fuchsia_hardware_ethernet_MacAddress mac;
        memcpy(mac.octets, mac_addr, 6);

        zx_status_t call_status, status;
        status = fuchsia_hardware_ethernet_DeviceConfigMulticastDeleteMac(svc_.get(), &mac,
                                                                          &call_status);
        if (status != ZX_OK) {
            return status;
        }
        return call_status;
    }

    // Delete this along with other "multicast_" related code once we have IGMP.
    // This tells the driver to turn off the on-by-default multicast-promisc.
    zx_status_t MulticastInitForTest() {
        zx_status_t call_status, status;
        status =
            fuchsia_hardware_ethernet_DeviceConfigMulticastTestFilter(svc_.get(), &call_status);
        if (status != ZX_OK) {
            return status;
        }
        return call_status;
    }

    fzl::fifo<fuchsia_hardware_ethernet_FifoEntry>* tx_fifo() { return &tx_; }
    fzl::fifo<fuchsia_hardware_ethernet_FifoEntry>* rx_fifo() { return &rx_; }
    uint32_t tx_depth() { return tx_depth_; }
    uint32_t rx_depth() { return rx_depth_; }

    uint8_t* GetRxBuffer(uint32_t offset) {
        return reinterpret_cast<uint8_t*>(mapped_) + offset;
    }

    fuchsia_hardware_ethernet_FifoEntry* GetTxBuffer() {
        auto entry_ptr = tx_available_.pop_front();
        fuchsia_hardware_ethernet_FifoEntry* entry = nullptr;
        if (entry_ptr != nullptr) {
            entry = &entry_ptr->e;
            tx_pending_.push_front(std::move(entry_ptr));
        }
        return entry;
    }

    void ReturnTxBuffer(fuchsia_hardware_ethernet_FifoEntry* entry) {
        auto entry_ptr = tx_pending_.erase_if(
                [entry](const FifoEntry& tx_entry) { return tx_entry.e.cookie == entry->cookie; });
        if (entry_ptr != nullptr) {
            tx_available_.push_front(std::move(entry_ptr));
        }
    }

  private:
    zx::channel svc_;

    uint64_t vmo_size_ = 0;
    zx::vmo buf_;
    uintptr_t mapped_ = 0;
    uint32_t nbufs_ = 0;
    uint16_t bufsize_ = 0;

    fzl::fifo<fuchsia_hardware_ethernet_FifoEntry> tx_;
    fzl::fifo<fuchsia_hardware_ethernet_FifoEntry> rx_;
    uint32_t tx_depth_ = 0;
    uint32_t rx_depth_ = 0;

    using FifoEntryPtr = fbl::unique_ptr<FifoEntry>;
    fbl::SinglyLinkedList<FifoEntryPtr> tx_available_;
    fbl::SinglyLinkedList<FifoEntryPtr> tx_pending_;
};

}  // namespace

#define HEADER_SIZE (sizeof(ethertap_socket_header_t))
#define READBUF_SIZE (ETHERTAP_MAX_MTU + HEADER_SIZE)

// Returns the number of reads
static int DrainSocket(zx::socket* sock) {
    zx_signals_t obs;
    uint8_t read_buf[READBUF_SIZE];
    size_t actual_sz = 0;
    int reads = 0;
    zx_status_t status = ZX_OK;

    while (ZX_OK == (status = sock->wait_one(ZX_SOCKET_READABLE, PROPAGATE_TIME, &obs))) {
        status = sock->read(0u, static_cast<void*>(read_buf), READBUF_SIZE, &actual_sz);
        ASSERT_EQ(ZX_OK, status);
        reads++;
    }
    ASSERT_EQ(status, ZX_ERR_TIMED_OUT);
    return reads;
}

static bool ExpectSockRead(zx::socket* sock, uint32_t type, size_t size, void* data,
                           const char* msg) {
    zx_signals_t obs;
    uint8_t read_buf[READBUF_SIZE];
    // The socket should be readable
    ASSERT_EQ(ZX_OK, sock->wait_one(ZX_SOCKET_READABLE, FAIL_TIMEOUT, &obs), msg);
    ASSERT_TRUE(obs & ZX_SOCKET_READABLE, msg);

    // Read the data from the socket, which should match what was written to the fifo
    size_t actual_sz = 0;
    ASSERT_EQ(ZX_OK, sock->read(0u, static_cast<void*>(read_buf), READBUF_SIZE, &actual_sz), msg);
    ASSERT_EQ(size, actual_sz - HEADER_SIZE, msg);
    auto header = reinterpret_cast<ethertap_socket_header*>(read_buf);
    ASSERT_EQ(type, header->type, msg);
    if (size > 0) {
        ASSERT_NONNULL(data, msg);
        ASSERT_BYTES_EQ(static_cast<uint8_t*>(data), read_buf + HEADER_SIZE, size, msg);
    }
    return true;
}

static bool ExpectPacketRead(zx::socket* sock, size_t size, void* data, const char* msg) {
    return ExpectSockRead(sock, ETHERTAP_MSG_PACKET, size, data, msg);
}

static bool ExpectSetParamRead(zx::socket* sock, uint32_t param, int32_t value,
                               size_t data_length, uint8_t* data, const char* msg) {
    ethertap_setparam_report_t report = {};
    report.param = param;
    report.value = value;
    report.data_length = data_length;
    ASSERT_LE(data_length, SETPARAM_REPORT_DATA_SIZE, "Report can't return that much data");
    if (data_length > 0 && data != nullptr) {
        memcpy(report.data, data, data_length);
    }
    return ExpectSockRead(sock, ETHERTAP_MSG_PARAM_REPORT, sizeof(report), &report, msg);
}

// Functions named ...Helper are intended to be called from every test function for
// setup and teardown of the ethdevs.
// To generate informative error messages in case they fail, use ASSERT_TRUE() when
// calling them.

// Note that the test framework will not return false from a helper function upon failure
// of an EXPECT_ unless the BEGIN_HELPER / END_HELPER macros are used in that function.
// See zircon/system/ulib/unittest/include/unittest/unittest.h and read carefully!

static bool AddClientHelper(zx::socket* sock,
                            EthernetClient* client,
                            const EthernetOpenInfo& openInfo) {
    // Open the ethernet device
    zx::channel svc;
    ASSERT_EQ(ZX_OK, OpenEthertapDev(&svc));
    ASSERT_TRUE(svc.is_valid());

    // Initialize the ethernet client
    ASSERT_EQ(ZX_OK, client->Register(std::move(svc), openInfo.name, 32, 2048));
    if (openInfo.online) {
        // Start the ethernet client
        ASSERT_EQ(ZX_OK, client->Start());
    }
    if (openInfo.multicast) {
        ASSERT_EQ(ZX_OK, client->MulticastInitForTest());
    }
    if (openInfo.options & ETHERTAP_OPT_REPORT_PARAM) {
        DrainSocket(sock); // internal driver setup probably has caused some reports
    }
    return true;
}

static bool OpenFirstClientHelper(zx::socket* sock,
                               EthernetClient* client,
                               const EthernetOpenInfo& openInfo) {
    // Create the ethertap device
    ASSERT_EQ(ZX_OK, CreateEthertapWithOption(1500, openInfo.name, sock, openInfo.options));

    if (openInfo.online) {
        // Set the link status to online
        sock->signal_peer(0, ETHERTAP_SIGNAL_ONLINE);
        // Sleep for just long enough to let the signal propagate
        zx::nanosleep(PROPAGATE_TIME);
    }

    ASSERT_TRUE(AddClientHelper(sock, client, openInfo));
    return true;
}

static bool EthernetCleanupHelper(zx::socket* sock,
                                  EthernetClient* client,
                                  EthernetClient* client2 = nullptr) {
    // Note: Don't keep adding client params; find another way if more than 2 clients.

    // Shutdown the ethernet client(s)
    ASSERT_EQ(ZX_OK, client->Stop());
    if (client2 != nullptr) {
        ASSERT_EQ(ZX_OK, client->Stop());
    }

    // Clean up the ethertap device
    sock->reset();

    ETHTEST_CLEANUP_DELAY;
    return true;
}

static bool EthernetStartTest() {
    BEGIN_TEST;

    zx::socket sock;
    EthernetClient client;
    EthernetOpenInfo info(__func__);
    info.online = false;
    ASSERT_TRUE(OpenFirstClientHelper(&sock, &client, info));

    // Verify no signals asserted on the rx fifo
    zx_signals_t obs = 0;
    client.rx_fifo()->wait_one(fuchsia_hardware_ethernet_SIGNAL_STATUS, zx::time(), &obs);
    EXPECT_FALSE(obs & fuchsia_hardware_ethernet_SIGNAL_STATUS);

    // Start the ethernet client
    EXPECT_EQ(ZX_OK, client.Start());

    // Verify that the ethernet driver signaled a status change for the initial state.
    obs = 0;
    EXPECT_EQ(ZX_OK, client.rx_fifo()->wait_one(fuchsia_hardware_ethernet_SIGNAL_STATUS,
                                                FAIL_TIMEOUT, &obs));
    EXPECT_TRUE(obs & fuchsia_hardware_ethernet_SIGNAL_STATUS);

    // Default link status should be OFFLINE
    uint32_t eth_status = 0;
    EXPECT_EQ(ZX_OK, client.GetStatus(&eth_status));
    EXPECT_EQ(0, eth_status);

    // Set the link status to online and verify
    sock.signal_peer(0, ETHERTAP_SIGNAL_ONLINE);

    EXPECT_EQ(ZX_OK, client.rx_fifo()->wait_one(fuchsia_hardware_ethernet_SIGNAL_STATUS,
                                                FAIL_TIMEOUT, &obs));
    EXPECT_TRUE(obs & fuchsia_hardware_ethernet_SIGNAL_STATUS);

    EXPECT_EQ(ZX_OK, client.GetStatus(&eth_status));
    EXPECT_EQ(fuchsia_hardware_ethernet_DEVICE_STATUS_ONLINE, eth_status);

    ASSERT_TRUE(EthernetCleanupHelper(&sock, &client));
    END_TEST;
}

static bool EthernetLinkStatusTest() {
    BEGIN_TEST;
    // Create the ethertap device
    zx::socket sock;
    EthernetClient client;
    EthernetOpenInfo info(__func__);
    ASSERT_TRUE(OpenFirstClientHelper(&sock, &client, info));

    // Verify that the ethernet driver signaled a status change for the initial state.
    zx_signals_t obs = 0;
    EXPECT_EQ(ZX_OK, client.rx_fifo()->wait_one(fuchsia_hardware_ethernet_SIGNAL_STATUS,
                                                FAIL_TIMEOUT, &obs));
    EXPECT_TRUE(obs & fuchsia_hardware_ethernet_SIGNAL_STATUS);

    // Link status should be ONLINE since it's set in OpenFirstClientHelper
    uint32_t eth_status = 0;
    EXPECT_EQ(ZX_OK, client.GetStatus(&eth_status));
    EXPECT_EQ(fuchsia_hardware_ethernet_DEVICE_STATUS_ONLINE, eth_status);

    // Now the device goes offline
    sock.signal_peer(0, ETHERTAP_SIGNAL_OFFLINE);

    // Verify the link status
    obs = 0;
    EXPECT_EQ(ZX_OK, client.rx_fifo()->wait_one(fuchsia_hardware_ethernet_SIGNAL_STATUS,
                                                FAIL_TIMEOUT, &obs));
    EXPECT_TRUE(obs & fuchsia_hardware_ethernet_SIGNAL_STATUS);

    EXPECT_EQ(ZX_OK, client.GetStatus(&eth_status));
    EXPECT_EQ(0, eth_status);

    ASSERT_TRUE(EthernetCleanupHelper(&sock, &client));
    END_TEST;
}

static bool EthernetSetPromiscMultiClientTest() {
    BEGIN_TEST;

    zx::socket sock;
    EthernetClient clientA;
    EthernetOpenInfo info("SetPromiscA");
    info.options = ETHERTAP_OPT_REPORT_PARAM;
    ASSERT_TRUE(OpenFirstClientHelper(&sock, &clientA, info));
    EthernetClient clientB;
    info.name = "SetPromiscB";
    ASSERT_TRUE(AddClientHelper(&sock, &clientB, info));

    ASSERT_EQ(ZX_OK, clientA.SetPromisc(true));

    ExpectSetParamRead(&sock, ETHMAC_SETPARAM_PROMISC, 1, 0, nullptr, "Promisc on (1)");

    // None of these should cause a change in promisc commands to ethermac.
    ASSERT_EQ(ZX_OK, clientA.SetPromisc(true)); // It was already requested by A.
    ASSERT_EQ(ZX_OK, clientB.SetPromisc(true));
    ASSERT_EQ(ZX_OK, clientA.SetPromisc(false)); // A should now not want it, but B still does.
    EXPECT_EQ(0, DrainSocket(&sock));

    // After the next line, no one wants promisc, so I should get a command to turn it off.
    ASSERT_EQ(ZX_OK, clientB.SetPromisc(false));
    ExpectSetParamRead(&sock, ETHMAC_SETPARAM_PROMISC, 0, 0, nullptr, "Promisc should be off (2)");

    ASSERT_TRUE(EthernetCleanupHelper(&sock, &clientA, &clientB));
    END_TEST;
}

static bool EthernetSetPromiscClearOnCloseTest() {
    BEGIN_TEST;
    zx::socket sock;
    EthernetClient client;
    EthernetOpenInfo info(__func__);
    info.options = ETHERTAP_OPT_REPORT_PARAM;
    ASSERT_TRUE(OpenFirstClientHelper(&sock, &client, info));

    ASSERT_EQ(ZX_OK, client.SetPromisc(true));

    ExpectSetParamRead(&sock, ETHMAC_SETPARAM_PROMISC, 1, 0, nullptr, "Promisc on (1)");

    // Shutdown the ethernet client.
    EXPECT_EQ(ZX_OK, client.Stop());
    client.Cleanup(); // This will free devfd

    // That should have caused promisc to turn off.
    ExpectSetParamRead(&sock, ETHMAC_SETPARAM_PROMISC, 0, 0, nullptr, "Closed: promisc off (2)");

    // Clean up the ethertap device.
    sock.reset();

    ETHTEST_CLEANUP_DELAY;
    END_TEST;
}

// Since we don't have IGMP, multicast promiscuous is on by default. Multicast-related tests
// need to turn it off. This test establishes that this is happening correctly. When IGMP is
// added and promisc-by-default is turned off, this test will fail. When that happens, delete
// the code related to EthernetOpenInfo.multicast.
static bool EthernetClearMulticastPromiscTest() {
    BEGIN_TEST;
    zx::socket sock;
    EthernetClient client;
    EthernetOpenInfo info(__func__);
    info.options = ETHERTAP_OPT_REPORT_PARAM;
    ASSERT_TRUE(OpenFirstClientHelper(&sock, &client, info));

    ASSERT_EQ(ZX_OK, client.MulticastInitForTest());
    ExpectSetParamRead(&sock, ETHMAC_SETPARAM_MULTICAST_PROMISC, 0, 0, nullptr, "promisc off");

    ASSERT_TRUE(EthernetCleanupHelper(&sock, &client));
    END_TEST;
}

static bool EthernetMulticastRejectsUnicastAddress() {
    BEGIN_TEST;
    zx::socket sock;
    EthernetClient client;
    EthernetOpenInfo info(__func__);
    info.options = ETHERTAP_OPT_REPORT_PARAM;
    info.multicast = true;
    ASSERT_TRUE(OpenFirstClientHelper(&sock, &client, info));

    uint8_t unicastMac[] = {2, 4, 6, 8, 10, 12}; // For multicast, LSb of MSB should be 1
    ASSERT_EQ(ZX_ERR_INVALID_ARGS, client.MulticastAddressAdd(unicastMac));

    ASSERT_TRUE(EthernetCleanupHelper(&sock, &client));
    END_TEST;
}

static bool EthernetMulticastSetsAddresses() {
    BEGIN_TEST;
    zx::socket sock;
    EthernetClient clientA;
    EthernetOpenInfo info("MultiAdrTestA");
    info.options = ETHERTAP_OPT_REPORT_PARAM;
    info.multicast = true;
    ASSERT_TRUE(OpenFirstClientHelper(&sock, &clientA, info));
    info.name = "MultiAdrTestB";
    EthernetClient clientB;
    ASSERT_TRUE(AddClientHelper(&sock, &clientB, info));

    uint8_t macA[] = {1,2,3,4,5,6};
    uint8_t macB[] = {7,8,9,10,11,12};
    uint8_t data[] = {6, 12};
    ASSERT_EQ(ZX_OK, clientA.MulticastAddressAdd(macA));
    ExpectSetParamRead(&sock, ETHMAC_SETPARAM_MULTICAST_FILTER, 1, 1, data, "first addr");
    ASSERT_EQ(ZX_OK, clientB.MulticastAddressAdd(macB));
    ExpectSetParamRead(&sock, ETHMAC_SETPARAM_MULTICAST_FILTER, 2, 2, data, "second addr");

    ASSERT_TRUE(EthernetCleanupHelper(&sock, &clientA, &clientB));
    END_TEST;
}

// This value is implementation dependent, set in zircon/system/dev/ethernet/ethernet/ethernet.c
#define MULTICAST_LIST_LIMIT 32

static bool EthernetMulticastPromiscOnOverflow() {
    BEGIN_TEST;
    zx::socket sock;
    EthernetClient clientA;
    EthernetOpenInfo info("McPromOvA");
    info.options = ETHERTAP_OPT_REPORT_PARAM;
    info.multicast = true;
    ASSERT_TRUE(OpenFirstClientHelper(&sock, &clientA, info));
    EthernetClient clientB;
    info.name = "McPromOvB";
    ASSERT_TRUE(AddClientHelper(&sock, &clientB, info));
    uint8_t mac[] = {1,2,3,4,5,0};
    uint8_t data[MULTICAST_LIST_LIMIT];
    ASSERT_LT(MULTICAST_LIST_LIMIT, 255); // If false, add code to avoid duplicate mac addresses
    uint8_t next_val = 0x11; // Any value works; starting at 0x11 makes the dump extra readable.
    uint32_t n_data = 0;
    for (uint32_t i = 0; i < MULTICAST_LIST_LIMIT-1; i++) {
        mac[5] = next_val;
        data[n_data++] = next_val++;
        ASSERT_EQ(ZX_OK, clientA.MulticastAddressAdd(mac));
        ASSERT_TRUE(ExpectSetParamRead(&sock, ETHMAC_SETPARAM_MULTICAST_FILTER,
                                       n_data, n_data, data, "loading filter"));
    }
    ASSERT_EQ(n_data, MULTICAST_LIST_LIMIT-1); // There should be 1 space left
    mac[5] = next_val;
    data[n_data++] = next_val++;
    ASSERT_EQ(ZX_OK, clientB.MulticastAddressAdd(mac));
    ASSERT_TRUE(ExpectSetParamRead(&sock, ETHMAC_SETPARAM_MULTICAST_FILTER, n_data, n_data, data,
                                   "b - filter should be full"));
    mac[5] = next_val++;
    ASSERT_EQ(ZX_OK, clientB.MulticastAddressAdd(mac));
    ASSERT_TRUE(ExpectSetParamRead(&sock, ETHMAC_SETPARAM_MULTICAST_FILTER, -1, 0, nullptr,
                                   "overloaded B"));
    ASSERT_EQ(ZX_OK, clientB.Stop());
    n_data--;
    ASSERT_TRUE(ExpectSetParamRead(&sock, ETHMAC_SETPARAM_MULTICAST_FILTER, n_data, n_data, data,
                                   "deleted B - filter should have 31"));
    mac[5] = next_val;
    data[n_data++] = next_val++;
    ASSERT_EQ(ZX_OK, clientA.MulticastAddressAdd(mac));
    ASSERT_TRUE(ExpectSetParamRead(&sock, ETHMAC_SETPARAM_MULTICAST_FILTER, n_data, n_data, data,
                                   "a - filter should be full"));
    mac[5] = next_val++;
    ASSERT_EQ(ZX_OK, clientA.MulticastAddressAdd(mac));
    ASSERT_TRUE(ExpectSetParamRead(&sock, ETHMAC_SETPARAM_MULTICAST_FILTER, -1, 0, nullptr,
                                   "overloaded A"));
    ASSERT_TRUE(EthernetCleanupHelper(&sock, &clientA));
    END_TEST;
}

static bool EthernetSetMulticastPromiscMultiClientTest() {
    BEGIN_TEST;
    zx::socket sock;
    EthernetClient clientA;
    EthernetOpenInfo info("MultiPromiscA");
    info.options = ETHERTAP_OPT_REPORT_PARAM;
    info.multicast = true;
    ASSERT_TRUE(OpenFirstClientHelper(&sock, &clientA, info));
    EthernetClient clientB;
    info.name = "MultiPromiscB";
    ASSERT_TRUE(AddClientHelper(&sock, &clientB, info));

    clientA.SetMulticastPromisc(true);
    ExpectSetParamRead(&sock, ETHMAC_SETPARAM_MULTICAST_PROMISC, 1, 0, nullptr, "Promisc on (1)");

    // None of these should cause a change in promisc commands to ethermac.
    clientA.SetMulticastPromisc(true); // It was already requested by A.
    clientB.SetMulticastPromisc(true);
    clientA.SetMulticastPromisc(false); // A should now not want it, but B still does.
    EXPECT_EQ(0, DrainSocket(&sock));

    // After the next line, no one wants promisc, so I should get a command to turn it off.
    clientB.SetMulticastPromisc(false);
    // That should have caused promisc to turn off.
    ExpectSetParamRead(&sock, ETHMAC_SETPARAM_MULTICAST_PROMISC, 0, 0, nullptr,
                       "Closed: promisc off (2)");

    ASSERT_TRUE(EthernetCleanupHelper(&sock, &clientA, &clientB));
    END_TEST;
}

static bool EthernetSetMulticastPromiscClearOnCloseTest() {
    BEGIN_TEST;
    zx::socket sock;
    EthernetClient client;
    EthernetOpenInfo info(__func__);
    info.options = ETHERTAP_OPT_REPORT_PARAM;
    info.multicast = true;
    ASSERT_TRUE(OpenFirstClientHelper(&sock, &client, info));

    ASSERT_EQ(ZX_OK, client.SetPromisc(true));

    ExpectSetParamRead(&sock, ETHMAC_SETPARAM_PROMISC, 1, 0, nullptr, "Promisc on (1)");

    // Shutdown the ethernet client.
    EXPECT_EQ(ZX_OK, client.Stop());
    client.Cleanup(); // This will free devfd

    // That should have caused promisc to turn off.
    ExpectSetParamRead(&sock, ETHMAC_SETPARAM_PROMISC, 0, 0, nullptr, "Closed: promisc off (2)");

    // Clean up the ethertap device.
    sock.reset();

    ETHTEST_CLEANUP_DELAY;
    END_TEST;
}

static bool EthernetDataTest_Send() {
    BEGIN_TEST;
    zx::socket sock;
    EthernetClient client;
    EthernetOpenInfo info(__func__);
    ASSERT_TRUE(OpenFirstClientHelper(&sock, &client, info));

    // Ensure that the fifo is writable
    zx_signals_t obs;
    EXPECT_EQ(ZX_OK, client.tx_fifo()->wait_one(ZX_FIFO_WRITABLE, zx::time(), &obs));
    ASSERT_TRUE(obs & ZX_FIFO_WRITABLE);

    // Grab an available TX fifo entry
    auto entry = client.GetTxBuffer();
    ASSERT_TRUE(entry != nullptr);

    // Populate some data
    uint8_t* buf = reinterpret_cast<uint8_t*>(entry->cookie);
    for (int i = 0; i < 32; i++) {
        buf[i] = static_cast<uint8_t>(i & 0xff);
    }
    entry->length = 32;

    // Write to the TX fifo
    ASSERT_EQ(ZX_OK, client.tx_fifo()->write_one(*entry));

    ExpectPacketRead(&sock, 32, buf, "");

    // Now the TX completion entry should be available to read from the TX fifo
    EXPECT_EQ(ZX_OK, client.tx_fifo()->wait_one(ZX_FIFO_READABLE, FAIL_TIMEOUT, &obs));
    ASSERT_TRUE(obs & ZX_FIFO_READABLE);

    fuchsia_hardware_ethernet_FifoEntry return_entry;
    ASSERT_EQ(ZX_OK, client.tx_fifo()->read_one(&return_entry));

    // Check the flags on the returned entry
    EXPECT_TRUE(return_entry.flags & fuchsia_hardware_ethernet_FIFO_TX_OK);
    return_entry.flags = 0;

    // Verify the bytes from the rest of the entry match what we wrote
    auto expected_entry = reinterpret_cast<uint8_t*>(entry);
    auto actual_entry = reinterpret_cast<uint8_t*>(&return_entry);
    EXPECT_BYTES_EQ(expected_entry, actual_entry, sizeof(fuchsia_hardware_ethernet_FifoEntry), "");

    // Return the buffer to our client; the client destructor will make sure no TXs are still
    // pending at the end of te test.
    client.ReturnTxBuffer(&return_entry);

    ASSERT_TRUE(EthernetCleanupHelper(&sock, &client));
    END_TEST;
}

static bool EthernetDataTest_Recv() {
    BEGIN_TEST;
    zx::socket sock;
    EthernetClient client;
    EthernetOpenInfo info(__func__);
    ASSERT_TRUE(OpenFirstClientHelper(&sock, &client, info));

    // The socket should be writable
    zx_signals_t obs;
    EXPECT_EQ(ZX_OK, sock.wait_one(ZX_SOCKET_WRITABLE, zx::time(), &obs));
    ASSERT_TRUE(obs & ZX_SOCKET_WRITABLE);

    // Send a buffer through the socket
    uint8_t buf[32];
    for (int i = 0; i < 32; i++) {
        buf[i] = static_cast<uint8_t>(i & 0xff);
    }
    size_t actual = 0;
    EXPECT_EQ(ZX_OK, sock.write(0, static_cast<void*>(buf), 32, &actual));
    EXPECT_EQ(32, actual);

    // The fifo should be readable
    EXPECT_EQ(ZX_OK, client.rx_fifo()->wait_one(ZX_FIFO_READABLE, FAIL_TIMEOUT, &obs));
    ASSERT_TRUE(obs & ZX_FIFO_READABLE);

    // Read the RX fifo
    fuchsia_hardware_ethernet_FifoEntry entry;
    EXPECT_EQ(ZX_OK, client.rx_fifo()->read_one(&entry));

    // Check the bytes in the VMO compared to what we sent through the socket
    auto return_buf = client.GetRxBuffer(entry.offset);
    EXPECT_BYTES_EQ(buf, return_buf, entry.length, "");

    // RX fifo should be readable, and we can return the buffer to the driver
    EXPECT_EQ(ZX_OK, client.rx_fifo()->wait_one(ZX_FIFO_WRITABLE, zx::time(), &obs));
    ASSERT_TRUE(obs & ZX_FIFO_WRITABLE);

    entry.length = 2048;
    EXPECT_EQ(ZX_OK, client.rx_fifo()->write_one(entry));

    ASSERT_TRUE(EthernetCleanupHelper(&sock, &client));
    END_TEST;
}

BEGIN_TEST_CASE(EthernetSetupTests)
RUN_TEST_MEDIUM(EthernetStartTest)
RUN_TEST_MEDIUM(EthernetLinkStatusTest)
END_TEST_CASE(EthernetSetupTests)

BEGIN_TEST_CASE(EthernetConfigTests)
RUN_TEST_MEDIUM(EthernetSetPromiscMultiClientTest)
RUN_TEST_MEDIUM(EthernetSetPromiscClearOnCloseTest)
RUN_TEST_MEDIUM(EthernetClearMulticastPromiscTest)
RUN_TEST_MEDIUM(EthernetMulticastRejectsUnicastAddress)
RUN_TEST_MEDIUM(EthernetMulticastSetsAddresses)
RUN_TEST_MEDIUM(EthernetMulticastPromiscOnOverflow)
RUN_TEST_MEDIUM(EthernetSetMulticastPromiscMultiClientTest)
RUN_TEST_MEDIUM(EthernetSetMulticastPromiscClearOnCloseTest)
END_TEST_CASE(EthernetConfigTests)

BEGIN_TEST_CASE(EthernetDataTests)
RUN_TEST_MEDIUM(EthernetDataTest_Send)
RUN_TEST_MEDIUM(EthernetDataTest_Recv)
END_TEST_CASE(EthernetDataTests)

int main(int argc, char* argv[]) {
    bool success = unittest_run_all_tests(argc, argv);
    return success ? 0 : -1;
}
