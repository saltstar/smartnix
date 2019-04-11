// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_DEV_BUS_VIRTIO_SOCKET_H_
#define ZIRCON_SYSTEM_DEV_BUS_VIRTIO_SOCKET_H_

#include "device.h"
#include "ring.h"

#include <ddk/debug.h>
#include <ddk/io-buffer.h>
#include <fbl/auto_lock.h>
#include <fbl/intrusive_double_list.h>
#include <fbl/intrusive_hash_table.h>
#include <fbl/ref_counted.h>
#include <lib/async-loop/cpp/loop.h>
#include <lib/async/cpp/wait.h>
#include <lib/zx/channel.h>
#include <lib/zx/socket.h>
#include <lib/zx/timer.h>
#include <utility>
#include <virtio/vsock.h>

#include <fuchsia/hardware/vsock/c/fidl.h>
#include <zircon/thread_annotations.h>

namespace virtio {

using vsock_Addr = fuchsia_hardware_vsock_Addr;

class SocketDevice : public Device {
public:
    class ConnectionKey;

    explicit SocketDevice(zx_device_t* device, zx::bti, fbl::unique_ptr<Backend> backend);
    virtual ~SocketDevice() override;

    // Handlers for the incoming FIDL message endpoint. Public as the messages
    // get forwarded by C dispatch code.
    void MessageStart(zx::channel callbacks);
    zx_status_t MessageSendRst(const ConnectionKey& key);
    zx_status_t MessageSendShutdown(const ConnectionKey& key);
    zx_status_t MessageSendRequest(const ConnectionKey& key, zx::socket data);
    zx_status_t MessageSendResponse(const ConnectionKey& key, zx::socket data);
    zx_status_t MessageSendVmo(const ConnectionKey& key, zx::vmo vmo, uint64_t off, uint64_t len);
    uint32_t MessageGetCid();

    zx_status_t Init() override;
    void Release() override;

    // VirtIO callbacks
    void IrqRingUpdate() override;
    void IrqConfigChange() override;
    const char* tag() const override { return "virtio-vsock"; }

    // ConnectionKey is mostly a wrapper around vsock_Addr that provides
    // an equality operation for use as the Key in a HashMap.
    class ConnectionKey {
    public:
        ConnectionKey(const vsock_Addr& addr) : addr_(addr) {}
        ConnectionKey(uint32_t local_port, uint32_t remote_cid, uint32_t remote_port)
            : addr_({.local_port = local_port,
                     .remote_cid = remote_cid,
                     .remote_port = remote_port}) {}
        static ConnectionKey FromHdr(virtio_vsock_hdr_t* hdr) {
            return ConnectionKey(hdr->dst_port, static_cast<uint32_t>(hdr->src_cid), hdr->src_port);
        }
        vsock_Addr addr_;
        bool operator==(const ConnectionKey& addr) const {
            return addr_.local_port == addr.addr_.local_port &&
                   addr_.remote_cid == addr.addr_.remote_cid &&
                   addr_.remote_port == addr.addr_.remote_port;
        }
    };

    struct CreditInfo {
        CreditInfo() : buf_alloc(0), fwd_count(0) {}
        CreditInfo(uint32_t buf, uint32_t fwd) : buf_alloc(buf), fwd_count(fwd) {}
        uint32_t buf_alloc;
        uint32_t fwd_count;
    };

private:
    // Wrapper around a virtio Ring that uses a single contiguous io_buffer to
    // fill the descriptors.
    class IoBufferRing {
    public:
        IoBufferRing(Device* device, uint16_t count, uint32_t buf_size, bool host_write_only);
        virtual ~IoBufferRing();
        // Initialize the Ring and allocate the io_buffer. index is the virtio ring index
        // in the device. This must be called prior to using any other members of
        // this class.
        zx_status_t Init(uint16_t index, const zx::bti& bti);
        // Frees the resources allocated by this ring.
        void FreeBuffers();

        inline void Kick() {
            assert(io_buffer_is_valid(&io_buffer_));
            ring_.Kick();
        }

    protected:
        void* GetRawDesc(uint16_t id, uint32_t len, uint32_t offset = 0) {
            assert(len + offset <= buf_size_);
            assert(io_buffer_is_valid(&io_buffer_));
            uintptr_t base = reinterpret_cast<uintptr_t>(io_buffer_virt(&io_buffer_));
            return reinterpret_cast<void*>(base + id * buf_size_ + offset);
        }

        Ring ring_;
        bool host_write_only_;
        io_buffer_t io_buffer_;
        uint16_t count_;
        uint32_t buf_size_;
    };

    class RxIoBufferRing : public IoBufferRing {
    public:
        RxIoBufferRing(Device* device, uint16_t count, uint32_t buf_size);

        // Submit descriptors into the ring. Typically only needs to be called on
        // init, as ProcessDescriptors will automatically call this.
        void RefillRing();

        // Calls the provided function on any completed descriptors giving header
        // and any extra data. Drops any descriptors that are chained.
        // Will automatically refill and kick the ring.
        template <typename H, typename F> void ProcessDescriptors(F func);
    };

    class TxIoBufferRing : public IoBufferRing {
    public:
        TxIoBufferRing(Device* device, uint16_t count, uint32_t buf_size);
        // Allocates a descriptor returning a pointer to the location to fill with
        // data.
        void* AllocInPlace(uint16_t* id);

        // Allocate a descriptor chain for sending an indirect payload. The
        // chain should have SetIndirectPayload and SetHeader called on it
        // prior to being given to SubmitChain.
        bool AllocIndirect(const ConnectionKey& key, uint16_t* id);

        // Attaches the indirect payload to the descriptor chain allocated by
        // AllocIndirect.
        void SetIndirectPayload(uint16_t id, uintptr_t payload);

        void SetHeader(uint16_t id, const virtio_vsock_hdr_t& hdr) {
            *reinterpret_cast<virtio_vsock_hdr_t*>(GetRawDesc(id, sizeof(virtio_vsock_hdr_t))) =
                hdr;
        }

        // Submit a chain for TX. Does not kick the ring.
        void SubmitChain(uint16_t id, uint32_t data_len);

        void FreeChain(uint16_t id);

        // Processes the completed tx descriptors and calls the provided function
        // with the key and indirect payload for any indirect descriptors.
        template <typename F> void ProcessDescriptors(F func);
    };

    // A Connection may get placed in three different containers. A hash table
    // (connections_), which requires the SinglyLinkedListable storage, a double
    // linked list (has_pending_tx_), and a second double linked list (has_pending_op_).
    // As DoubleLinkedListable can only be inherited once we define additional
    // storage in QueuedOpTraits.
    class Connection : public fbl::RefCounted<Connection>,
                       public fbl::SinglyLinkedListable<fbl::RefPtr<Connection>>,
                       public fbl::DoublyLinkedListable<fbl::RefPtr<Connection>> {
    private:
        // A connection moves through different states over its lifetime. These
        // states have a very simple transition system in that they can only
        // go 'forward', hence we consider a connection progressing through
        // a lifetime at different rates. A connection can jump into existence
        // at either CON_WAIT_RESPONSE or CON_ACTIVE, and can be deleted from
        // any state. CON_SHUTTING_DOWN is an optional 'grace' state and
        // CON_ZOMBIE is only needed in cases where the connection cannot be
        // immediately deleted.
        enum State {
            // A connection is attempting to be established and is waiting for
            // a response from a remote.
            CON_WAIT_RESPONSE,
            // The 'normal' state of a connection. It can TX/RX, has valid
            // credit.
            CON_ACTIVE,
            // If a graceful shutdown is requested, but there is still pending TX
            // data, then this state indicates that no more TX data should be
            // accepted, but we have not yet told the remote we are shutting down.
            CON_WILL_SHUT_DOWN,
            // Trying to perform a graceful shutdown. Any pending TX will happen
            // and RX will still be passed on, but further TX is denied.
            CON_SHUTTING_DOWN,
            // Connection is considered terminated but resource cleanup is still
            // happening due to race conditions with dispatchers.
            CON_ZOMBIE,
        };

    public:
        using SignalHandler =
            fbl::Function<void(zx_status_t, const zx_packet_signal_t*, fbl::RefPtr<Connection>)>;
        Connection(const ConnectionKey& key, zx::socket data, SignalHandler wait_handler,
                   uint32_t cid, fbl::Mutex& lock);
        ~Connection() {}

        bool PendingTx();

        // Whether or not the connection is in the process of closing.
        bool IsShuttingDown();

        // Tell the connection to begin a client requested graceful shutdown.
        // This means we will drain any pending TX before completing the shutdown.
        bool BeginShutdown();

        // Informs the connection that a TX has completed that was sending the
        // specified |paddr|. This returns 'true' to indicate the current VMO has
        // completed sending.
        bool NotifyVmoTxComplete(uintptr_t paddr);

        void UpdateCredit(uint32_t buf, uint32_t fwd);

        // Marks a connection as active and able to send/receive data. Is ignored
        // if the connection is shutting down.
        void MakeActive(async_dispatcher_t* disp);

        // Receive data on the connection. Returns false if there is a client error
        // and the connection should be RST.
        bool Rx(void* data, size_t len);

        // Returns the credit information for this connection.
        CreditInfo GetCreditInfo();

        // Helper for making a header that is filled out with our connection key
        // and credit information.
        virtio_vsock_hdr_t MakeHdr(uint16_t op);

        // Close a connection indicating no more data shall be sent and received and
        // it should enter the zombie state until it gets fully deleted.
        void Close(async_dispatcher_t* dispatcher);

        // Performs any outstanding TX for this connection by filling the providing
        // ring with descriptors. This may generate credit requests and HasPendingOp
        // can be checked afterwards to see if this is the case. This returns
        // ZX_OK if there is no more pending tx, ZX_ERR_SHOULD_WAIT if there is
        // still data to send and we should retry when there is either more credit
        // or more TX descriptors. Any other return status indicates an underlying
        // transport error and the connection should be closed.
        zx_status_t ContinueTx(bool force_credit_request, TxIoBufferRing& tx,
                               async_dispatcher_t* dispatcher);

        zx_status_t SetVmo(zx::bti& bti, zx::vmo vmo, uint64_t offset, uint64_t len,
                           uint64_t bti_contiguity);

        void QueueOp(uint16_t new_op);
        bool HasPendingOp();
        uint16_t TakePendingOp();

        static size_t GetHash(const ConnectionKey& addr);
        const ConnectionKey& GetKey() const;
        // Trait struct that allows us to define state for the additional
        // linked list this Connection can be in.
        struct QueuedOpTraits {
            static fbl::DoublyLinkedListNodeState<fbl::RefPtr<Connection>>&
            node_state(Connection& conn) {
                return conn.queued_op_traits_;
            }
        };

    private:
        // Helper class for walking the physical addresses of a VMO.
        class VmoWalker {
        public:
            VmoWalker() {}
            ~VmoWalker() { Release(); }
            zx_status_t Set(zx::bti& bti, zx::vmo vmo, uint64_t offset, uint64_t len,
                            uint64_t bti_contiguity);
            void Release();
            uint64_t NextChunkLen(uint64_t max);
            zx_paddr_t Consume(uint64_t len);
            zx::pmt pinned_pages_;
            zx::vmo vmo_;
            uint64_t contiguity_;
            size_t num_paddr_;
            uint64_t base_addr_;
            zx_paddr_t* paddrs_;

            uint64_t transfer_offset_;
            uint64_t transfer_length_;

            zx_paddr_t final_paddr_;
        };

        void CountTx(uint32_t len);
        bool SocketTxPending();
        bool DoVmoTx(bool force_credit_request, TxIoBufferRing& tx);
        zx_status_t DoSocketTx(bool force_credit_request, TxIoBufferRing& tx,
                               async_dispatcher_t* dispatcher);
        void BeginWait(async_dispatcher_t* disp);
        uint32_t GetPeerFree(bool request_credit);

        // Reference to the lock that we will hold when performing BeginWait.
        fbl::Mutex& lock_;

        ConnectionKey key_;

        State state_;
        // Free running tx counter
        uint32_t tx_count_;
        // Free running rx counter
        uint32_t rx_count_;
        // Last known peer buffer information.
        uint32_t buf_alloc_;
        uint32_t fwd_cnt_;
        // Socket for TX/RX to the application
        zx::socket data_;

        async::Wait wait_handler_;
        // To resolve races with the dispatcher whenever wait_handler_ has a valid
        // handler this ref ptr must be valid. This ref ptr can be cleared only
        // when the wait is successfully canceled, or by the handler itself once
        // it completes.
        fbl::RefPtr<Connection> wait_handler_ref_;
        // Are we trying to send a vmo?
        bool pending_vmo_;
        VmoWalker vmo_;

        bool has_pending_op_;
        uint16_t pending_op_;
        uint32_t cid_;

        // State for an additional linked list.
        friend struct QueuedOpTraits;
        fbl::DoublyLinkedListNodeState<fbl::RefPtr<Connection>> queued_op_traits_;

        DISALLOW_COPY_ASSIGN_AND_MOVE(Connection);
    };
    using ConnectionIterator = fbl::HashTable<ConnectionKey, fbl::RefPtr<Connection>>::iterator;
    friend Connection;

private:
    void ProcessRxDescriptor(virtio_vsock_hdr_t* header, void* data, uint32_t data_len)
        TA_REQ(lock_);
    void UpdateRxRingLocked() TA_REQ(lock_);
    void RxOpLocked(ConnectionIterator conn, const ConnectionKey& key, uint16_t op) TA_REQ(lock_);

    bool SendOp_RawLocked(const ConnectionKey& key, uint16_t op, const CreditInfo& credit)
        TA_REQ(lock_);
    void SendOpLocked(fbl::RefPtr<Connection> conn, uint16_t op) TA_REQ(lock_);

    void RetryTxLocked(bool ask_for_credit) TA_REQ(lock_);
    void ContinueTxLocked(bool force_credit_request, fbl::RefPtr<Connection> conn) TA_REQ(lock_);

    void SendRstLocked(const ConnectionKey& key) TA_REQ(lock_);
    void CleanupConLocked(fbl::RefPtr<Connection> conn) TA_REQ(lock_);
    void NotifyAndCleanupConLocked(fbl::RefPtr<Connection> conn) TA_REQ(lock_);

    // Forcibly cleans up any outstanding connection and sends an RST to the host
    // This does not notify the callbacks and is for use when there are no callbacks
    // or the service requested the rst
    void CleanupConAndRstLocked(const ConnectionKey& key) TA_REQ(lock_);

    void RemoveCallbacksLocked() TA_REQ(lock_);

    // Helper for ensuring we only use the callbacks_ handle to perform a callback
    // when we have a valid handle. Also serves to convert a ConnectionKey to a
    // raw vsock_Addr.
    void PerformCallbackLocked(zx_status_t (*func)(zx_handle_t, const vsock_Addr*),
                               const ConnectionKey& key) TA_REQ(lock_);

    bool QueuedForTxLocked(fbl::RefPtr<Connection> conn) TA_REQ(lock_);
    void QueueForTxLocked(fbl::RefPtr<Connection> conn) TA_REQ(lock_);
    void DequeueTxLocked(fbl::RefPtr<Connection> conn) TA_REQ(lock_);

    bool QueuedForOpLocked(fbl::RefPtr<Connection> conn) TA_REQ(lock_);
    void QueueForOpLocked(fbl::RefPtr<Connection> conn) TA_REQ(lock_);
    void DequeueOpLocked(fbl::RefPtr<Connection> conn) TA_REQ(lock_);

    void EnableTxRetryTimerLocked() TA_REQ(lock_);
    void TimerWaitHandler(async_dispatcher_t* dispatcher, async::WaitBase* wait, zx_status_t status,
                          const zx_packet_signal_t* signal);
    void CallbacksSignalled(async_dispatcher_t* dispatcher, async::WaitBase* wait,
                            zx_status_t status, const zx_packet_signal_t* signal);
    void ConnectionSocketSignalled(zx_status_t status, const zx_packet_signal_t* signal,
                                   fbl::RefPtr<Connection> conn);

    void UpdateCidLocked() TA_REQ(lock_);
    void ReleaseLocked() TA_REQ(lock_);
    void TransportResetLocked() TA_REQ(lock_);

    DISALLOW_COPY_ASSIGN_AND_MOVE(SocketDevice);
    uint32_t cid_ TA_GUARDED(lock_);
    async::Loop dispatch_loop_ TA_GUARDED(lock_);
    RxIoBufferRing rx_ TA_GUARDED(lock_);
    TxIoBufferRing tx_ TA_GUARDED(lock_);
    RxIoBufferRing event_ TA_GUARDED(lock_);
    zx::channel callbacks_ TA_GUARDED(lock_);

    // List of connections that have pending TX but are waiting for either more
    // credit from the remote, or more TX descriptors.
    fbl::DoublyLinkedList<fbl::RefPtr<Connection>> has_pending_tx_ TA_GUARDED(lock_);
    // List of connections that still need to send an op.
    fbl::DoublyLinkedList<fbl::RefPtr<Connection>, SocketDevice::Connection::QueuedOpTraits>
        has_pending_op_ TA_GUARDED(lock_);

    fbl::HashTable<ConnectionKey, fbl::RefPtr<Connection>> connections_ TA_GUARDED(lock_);
    bool have_timer_ TA_GUARDED(lock_);
    zx::timer tx_retry_timer_ TA_GUARDED(lock_);
    async::WaitMethod<SocketDevice, &SocketDevice::TimerWaitHandler>
        timer_wait_handler_ TA_GUARDED(lock_);
    async::WaitMethod<SocketDevice, &SocketDevice::CallbacksSignalled>
        callback_closed_handler_ TA_GUARDED(lock_);
    uint64_t bti_contiguity_ TA_GUARDED(lock_);
};

} // namespace virtio

#endif // ZIRCON_SYSTEM_DEV_BUS_VIRTIO_SOCKET_H_
