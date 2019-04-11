// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fbl/array.h>
#include <unittest/unittest.h>
#include <zircon/syscalls.h>

namespace {

zx_signals_t get_satisfied_signals(zx_handle_t handle) {
    zx_signals_t pending = 0;
    zx_object_wait_one(handle, 0u, 0u, &pending);
    return pending;
}

bool socket_basic() {
    BEGIN_TEST;

    zx_status_t status;
    size_t count;

    zx_handle_t h[2];
    uint32_t read_data[] = {0, 0};

    status = zx_socket_create(0, h, h + 1);
    ASSERT_EQ(status, ZX_OK, "");

    // Check that koids line up.
    zx_info_handle_basic_t info[2] = {};
    status = zx_object_get_info(h[0], ZX_INFO_HANDLE_BASIC, &info[0], sizeof(info[0]), NULL, NULL);
    ASSERT_EQ(status, ZX_OK, "");
    status = zx_object_get_info(h[1], ZX_INFO_HANDLE_BASIC, &info[1], sizeof(info[1]), NULL, NULL);
    ASSERT_EQ(status, ZX_OK, "");
    ASSERT_NE(info[0].koid, 0u, "zero koid!");
    ASSERT_NE(info[0].related_koid, 0u, "zero peer koid!");
    ASSERT_NE(info[1].koid, 0u, "zero koid!");
    ASSERT_NE(info[1].related_koid, 0u, "zero peer koid!");
    ASSERT_EQ(info[0].koid, info[1].related_koid, "mismatched koids!");
    ASSERT_EQ(info[1].koid, info[0].related_koid, "mismatched koids!");

    status = zx_socket_read(h[0], 0u, read_data, sizeof(read_data), &count);
    EXPECT_EQ(status, ZX_ERR_SHOULD_WAIT, "");

    constexpr uint32_t write_data[] = {0xdeadbeef, 0xc0ffee};
    status = zx_socket_write(h[0], 0u, &write_data[0], sizeof(write_data[0]), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, sizeof(write_data[0]), "");
    status = zx_socket_write(h[0], 0u, &write_data[1], sizeof(write_data[1]), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, sizeof(write_data[1]), "");

    status = zx_socket_read(h[1], 0u, read_data, sizeof(read_data), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, sizeof(read_data), "");
    EXPECT_EQ(read_data[0], write_data[0], "");
    EXPECT_EQ(read_data[1], write_data[1], "");

    status = zx_socket_write(h[0], 0u, write_data, sizeof(write_data), NULL);
    EXPECT_EQ(status, ZX_OK, "");
    memset(read_data, 0, sizeof(read_data));
    status = zx_socket_read(h[1], 0u, read_data, sizeof(read_data), NULL);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(read_data[0], write_data[0], "");
    EXPECT_EQ(read_data[1], write_data[1], "");

    zx_handle_close(h[1]);

    status = zx_socket_write(h[0], 0u, &write_data[1], sizeof(write_data[1]), &count);
    EXPECT_EQ(status, ZX_ERR_PEER_CLOSED, "");

    zx_handle_close(h[0]);
    END_TEST;
}

bool socket_peek() {
    BEGIN_TEST;

    size_t count;
    zx_handle_t h[2];
    uint32_t read_data[] = {0, 0};
    constexpr uint32_t write_data[] = {0xdeadbeef, 0xc0ffee};

    ASSERT_EQ(zx_socket_create(0, h, h + 1), ZX_OK, "");

    EXPECT_EQ(zx_socket_write(h[0], 0u, &write_data[0], sizeof(write_data[0]), &count), ZX_OK, "");
    EXPECT_EQ(count, sizeof(write_data[0]), "");
    EXPECT_EQ(zx_socket_write(h[0], 0u, &write_data[1], sizeof(write_data[1]), &count), ZX_OK, "");
    EXPECT_EQ(count, sizeof(write_data[1]), "");

    EXPECT_EQ(zx_socket_read(h[1], ZX_SOCKET_PEEK, read_data, sizeof(read_data), &count),
              ZX_OK, "");
    EXPECT_EQ(count, sizeof(read_data), "");
    EXPECT_EQ(read_data[0], write_data[0], "");
    EXPECT_EQ(read_data[1], write_data[1], "");

    // The message should still be pending for h1 to read.
    EXPECT_EQ(get_satisfied_signals(h[0]), ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(get_satisfied_signals(h[1]), ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE, "");

    memset(read_data, 0, sizeof(read_data));
    EXPECT_EQ(zx_socket_read(h[1], 0u, read_data, sizeof(read_data), &count), ZX_OK, "");
    EXPECT_EQ(count, sizeof(read_data), "");
    EXPECT_EQ(read_data[0], write_data[0], "");
    EXPECT_EQ(read_data[1], write_data[1], "");

    zx_handle_close(h[0]);
    zx_handle_close(h[1]);

    END_TEST;
}

bool socket_peek_empty() {
    BEGIN_TEST;

    zx_handle_t h[2];
    ASSERT_EQ(zx_socket_create(0, h, h + 1), ZX_OK, "");

    size_t count;
    char data;
    EXPECT_EQ(zx_socket_read(h[0], ZX_SOCKET_PEEK, &data, sizeof(data), &count),
              ZX_ERR_SHOULD_WAIT, "");

    zx_handle_close(h[0]);
    zx_handle_close(h[1]);

    END_TEST;
}

bool socket_signals() {
    BEGIN_TEST;

    zx_status_t status;
    size_t count;

    zx_handle_t h0, h1;
    status = zx_socket_create(0, &h0, &h1);
    ASSERT_EQ(status, ZX_OK, "");

    zx_signals_t signals0 = get_satisfied_signals(h0);
    zx_signals_t signals1 = get_satisfied_signals(h1);

    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE, "");

    const size_t kAllSize = 128 * 1024;
    fbl::Array<char> big_buf(new char[kAllSize], kAllSize);
    ASSERT_NONNULL(big_buf.get(), "");

    memset(big_buf.get(), 0x66, kAllSize);

    status = zx_socket_write(h0, 0u, big_buf.get(), kAllSize / 16, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, kAllSize / 16, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);

    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_READABLE | ZX_SOCKET_WRITABLE, "");

    status = zx_socket_read(h1, 0u, big_buf.get(), kAllSize, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, kAllSize / 16, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);

    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE, "");

    status = zx_object_signal_peer(h0, ZX_SOCKET_WRITABLE, 0u);
    EXPECT_EQ(status, ZX_ERR_INVALID_ARGS, "");

    status = zx_object_signal_peer(h0, 0u, ZX_USER_SIGNAL_1);
    EXPECT_EQ(status, ZX_OK, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);

    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE | ZX_USER_SIGNAL_1, "");

    zx_handle_close(h1);

    signals0 = get_satisfied_signals(h0);
    EXPECT_EQ(signals0, ZX_SOCKET_PEER_CLOSED, "");

    zx_handle_close(h0);

    END_TEST;
}

bool socket_signals2() {
    BEGIN_TEST;

    zx_status_t status;
    size_t count;
    zx_handle_t h0, h1;
    status = zx_socket_create(0, &h0, &h1);
    ASSERT_EQ(status, ZX_OK, "socket_create");

    /* Set some invalid threshold values and verify */
    count = 0;
    status = zx_object_set_property(h0, ZX_PROP_SOCKET_RX_THRESHOLD,
                                    &count, sizeof(size_t));
    ASSERT_EQ(status, ZX_OK, "object_set_property");
    count = 0xefffffff;
    status = zx_object_set_property(h0, ZX_PROP_SOCKET_RX_THRESHOLD,
                                    &count, sizeof(size_t));
    ASSERT_EQ(status, ZX_ERR_INVALID_ARGS, "object_set_property");
    count = 0;
    status = zx_object_set_property(h1, ZX_PROP_SOCKET_TX_THRESHOLD,
                                    &count, sizeof(size_t));
    ASSERT_EQ(status, ZX_OK, "object_set_property");
    count = 0xefffffff;
    status = zx_object_set_property(h1, ZX_PROP_SOCKET_TX_THRESHOLD,
                                    &count, sizeof(size_t));
    ASSERT_EQ(status, ZX_ERR_INVALID_ARGS, "object_set_property");

    /*
     * In the code below, we are going to trigger the READ threshold
     * signal as soon as 101 bytes are available to read, and triger
     * the write threshold as long as we have 103 bytes we can write/
     */

    /* Set valid Read/Write thresholds and verify */
    constexpr size_t SOCKET2_SIGNALTEST_RX_THRESHOLD = 101;
    count = SOCKET2_SIGNALTEST_RX_THRESHOLD;
    status = zx_object_set_property(h0, ZX_PROP_SOCKET_RX_THRESHOLD,
                                    &count, sizeof(size_t));
    ASSERT_EQ(status, ZX_OK, "object_set_property");
    status = zx_object_get_property(h0, ZX_PROP_SOCKET_RX_THRESHOLD,
                                    &count, sizeof(size_t));
    ASSERT_EQ(status, ZX_OK, "object_get_property");
    ASSERT_EQ(count, (size_t)SOCKET2_SIGNALTEST_RX_THRESHOLD, "");

    zx_info_socket_t info;
    memset(&info, 0, sizeof(info));
    status = zx_object_get_info(h1, ZX_INFO_SOCKET, &info, sizeof(info), NULL, NULL);
    ASSERT_EQ(status, ZX_OK, "zx_object_get_info");
    size_t write_threshold = info.tx_buf_max - (SOCKET2_SIGNALTEST_RX_THRESHOLD + 2);
    status = zx_object_set_property(h1, ZX_PROP_SOCKET_TX_THRESHOLD,
                                    &write_threshold, sizeof(size_t));
    ASSERT_EQ(status, ZX_OK, "object_set_property");
    status = zx_object_get_property(h1, ZX_PROP_SOCKET_TX_THRESHOLD,
                                    &count, sizeof(size_t));
    ASSERT_EQ(status, ZX_OK, "object_get_property");
    ASSERT_EQ(count, write_threshold, "");

    /* Make sure duplicates get the same thresholds ! */
    zx_handle_t h0_clone, h1_clone;
    status = zx_handle_duplicate(h0, ZX_RIGHT_SAME_RIGHTS, &h0_clone);
    ASSERT_EQ(status, ZX_OK, "handle_duplicate");
    status = zx_handle_duplicate(h1, ZX_RIGHT_SAME_RIGHTS, &h1_clone);
    ASSERT_EQ(status, ZX_OK, "handle_duplicate");

    status = zx_object_get_property(h0_clone,
                                    ZX_PROP_SOCKET_RX_THRESHOLD,
                                    &count, sizeof(size_t));
    ASSERT_EQ(status, ZX_OK, "object_get_property");
    ASSERT_EQ(count, (size_t)SOCKET2_SIGNALTEST_RX_THRESHOLD, "");
    status = zx_object_get_property(h1_clone,
                                    ZX_PROP_SOCKET_TX_THRESHOLD,
                                    &count, sizeof(size_t));
    ASSERT_EQ(status, ZX_OK, "object_get_property");
    ASSERT_EQ(count, write_threshold, "");

    /* Test starting signal state after setting thresholds */
    zx_signals_t signals0 = get_satisfied_signals(h0);
    zx_signals_t signals1 = get_satisfied_signals(h1);
    zx_signals_t signals0_clone = get_satisfied_signals(h0_clone);
    zx_signals_t signals1_clone = get_satisfied_signals(h1_clone);

    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals0_clone, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE | ZX_SOCKET_WRITE_THRESHOLD, "");
    EXPECT_EQ(signals1_clone,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_WRITE_THRESHOLD, "");

    /* Write data and test signals */
    size_t bufsize = SOCKET2_SIGNALTEST_RX_THRESHOLD - 1;
    char buf[SOCKET2_SIGNALTEST_RX_THRESHOLD - 1];
    status = zx_socket_write(h1, 0u, buf, bufsize, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, bufsize, "");

    /*
     * We wrote less than the read and write thresholds. So we expect
     * the READ_THRESHOLD signal to be de-asserted and the WRITE_THRESHOLD
     * signal to be asserted.
     */
    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);
    signals0_clone = get_satisfied_signals(h0_clone);
    signals1_clone = get_satisfied_signals(h1_clone);

    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE, "");
    EXPECT_EQ(signals0_clone, ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE, "");
    EXPECT_EQ(signals1_clone,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_WRITE_THRESHOLD, "");

    /*
     * Now write exactly enough data to hit the read threshold
     */
    bufsize = 1;
    status = zx_socket_write(h1, 0u, buf, bufsize, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, bufsize, "");
    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);
    signals0_clone = get_satisfied_signals(h0_clone);
    signals1_clone = get_satisfied_signals(h1_clone);
    EXPECT_EQ(signals0,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE | ZX_SOCKET_READ_THRESHOLD,
              "");
    EXPECT_EQ(signals0_clone,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE | ZX_SOCKET_READ_THRESHOLD,
              "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE | ZX_SOCKET_WRITE_THRESHOLD, "");
    EXPECT_EQ(signals1_clone, ZX_SOCKET_WRITABLE | ZX_SOCKET_WRITE_THRESHOLD, "");

    /*
     * Bump up the read threshold and make sure the READ THRESHOLD signal gets
     * deasserted (and then restore the read threshold back).
     */
    count = SOCKET2_SIGNALTEST_RX_THRESHOLD + 50;
    status = zx_object_set_property(h0, ZX_PROP_SOCKET_RX_THRESHOLD,
                                    &count, sizeof(size_t));
    ASSERT_EQ(status, ZX_OK, "object_set_property");
    signals0 = get_satisfied_signals(h0);
    signals0_clone = get_satisfied_signals(h0_clone);
    EXPECT_EQ(signals0,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE,
              "");
    EXPECT_EQ(signals0_clone,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE,
              "");
    count = SOCKET2_SIGNALTEST_RX_THRESHOLD;
    status = zx_object_set_property(h0, ZX_PROP_SOCKET_RX_THRESHOLD,
                                    &count, sizeof(size_t));
    signals0 = get_satisfied_signals(h0);
    signals0_clone = get_satisfied_signals(h0_clone);
    EXPECT_EQ(signals0,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE | ZX_SOCKET_READ_THRESHOLD,
              "");
    EXPECT_EQ(signals0_clone,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE | ZX_SOCKET_READ_THRESHOLD,
              "");

    /*
     * Bump the write threshold way up and make sure the WRITE THRESHOLD signal gets
     * deasserted (and then restore the write threshold back).
     */
    count = info.tx_buf_max - 10;
    status = zx_object_set_property(h1, ZX_PROP_SOCKET_TX_THRESHOLD,
                                    &count, sizeof(size_t));
    ASSERT_EQ(status, ZX_OK, "object_set_property");
    signals1 = get_satisfied_signals(h1);
    signals1_clone = get_satisfied_signals(h1_clone);
    EXPECT_EQ(signals1,
              ZX_SOCKET_WRITABLE,
              "");
    EXPECT_EQ(signals1_clone,
              ZX_SOCKET_WRITABLE,
              "");
    count = write_threshold;
    status = zx_object_set_property(h1, ZX_PROP_SOCKET_TX_THRESHOLD,
                                    &count, sizeof(size_t));
    signals1 = get_satisfied_signals(h1);
    signals1_clone = get_satisfied_signals(h1_clone);
    EXPECT_EQ(signals1,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_WRITE_THRESHOLD,
              "");
    EXPECT_EQ(signals1_clone,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_WRITE_THRESHOLD,
              "");

    /*
     * Next write enough data to de-assert WRITE Threshold
     */
    bufsize = write_threshold - (SOCKET2_SIGNALTEST_RX_THRESHOLD + 1);
    fbl::Array<char> buf2(new char[bufsize], bufsize);
    status = zx_socket_write(h1, 0u, buf2.get(), bufsize, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, bufsize, "");
    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);
    signals0_clone = get_satisfied_signals(h0_clone);
    signals1_clone = get_satisfied_signals(h1_clone);
    EXPECT_EQ(signals0,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE | ZX_SOCKET_READ_THRESHOLD,
              "");
    EXPECT_EQ(signals0_clone,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE | ZX_SOCKET_READ_THRESHOLD,
              "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals1_clone, ZX_SOCKET_WRITABLE, "");

    /*
     * Finally read enough data to de-assert the read threshold and
     * re-assert the write threshold signals.
     */
    bufsize += 10;
    buf2.reset(new char[bufsize], bufsize);
    status = zx_socket_read(h0, 0u, buf2.get(), bufsize, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, bufsize, "");
    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);
    signals0_clone = get_satisfied_signals(h0_clone);
    signals1_clone = get_satisfied_signals(h1_clone);
    EXPECT_EQ(signals0,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE,
              "");
    EXPECT_EQ(signals0_clone,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE,
              "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE | ZX_SOCKET_WRITE_THRESHOLD, "");
    EXPECT_EQ(signals1_clone, ZX_SOCKET_WRITABLE | ZX_SOCKET_WRITE_THRESHOLD, "");

    zx_handle_close(h0);
    zx_handle_close(h1);
    zx_handle_close(h0_clone);
    zx_handle_close(h1_clone);
    END_TEST;
}

bool socket_peer_closed_signal() {
    BEGIN_TEST;

    zx_handle_t socket[2];
    ASSERT_EQ(zx_socket_create(0, &socket[0], &socket[1]), ZX_OK, "");
    ASSERT_EQ(zx_handle_close(socket[1]), ZX_OK, "");
    ASSERT_EQ(zx_object_signal_peer(socket[0], 0u, ZX_USER_SIGNAL_0), ZX_ERR_PEER_CLOSED, "");
    ASSERT_EQ(zx_handle_close(socket[0]), ZX_OK, "");

    END_TEST;
}

bool socket_peer_closed_set_property() {
    BEGIN_TEST;

    zx_handle_t socket[2];
    ASSERT_EQ(zx_socket_create(0, &socket[0], &socket[1]), ZX_OK, "");
    size_t t = 1;
    ASSERT_EQ(zx_object_set_property(socket[0], ZX_PROP_SOCKET_TX_THRESHOLD, &t, sizeof(t)),
              ZX_OK, "");
    ASSERT_EQ(zx_handle_close(socket[1]), ZX_OK, "");
    ASSERT_EQ(zx_object_set_property(socket[0], ZX_PROP_SOCKET_TX_THRESHOLD, &t, sizeof(t)),
              ZX_ERR_PEER_CLOSED, "");
    ASSERT_EQ(zx_handle_close(socket[0]), ZX_OK, "");

    END_TEST;
}

bool socket_shutdown_write() {
    BEGIN_TEST;

    zx_status_t status;
    size_t count;
    zx_signals_t signals0, signals1;

    zx_handle_t h0, h1;
    status = zx_socket_create(0, &h0, &h1);
    ASSERT_EQ(status, ZX_OK, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);

    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE, "");

    status = zx_socket_write(h1, 0u, "12345", 5u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");

    status = zx_socket_shutdown(h1, ZX_SOCKET_SHUTDOWN_WRITE);
    EXPECT_EQ(status, ZX_OK, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);

    EXPECT_EQ(signals0,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE | ZX_SOCKET_PEER_WRITE_DISABLED,
              "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITE_DISABLED, "");

    status = zx_socket_write(h0, 0u, "abcde", 5u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");

    signals1 = get_satisfied_signals(h1);
    EXPECT_EQ(signals1, ZX_SOCKET_READABLE | ZX_SOCKET_WRITE_DISABLED, "");

    status = zx_socket_write(h1, 0u, "fghij", 5u, &count);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");

    char rbuf[10] = {0};

    status = zx_socket_read(h0, 0u, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");
    EXPECT_EQ(memcmp(rbuf, "12345", 5), 0, "");

    status = zx_socket_read(h0, 0u, rbuf, 1u, &count);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");

    signals0 = get_satisfied_signals(h0);
    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_PEER_WRITE_DISABLED, "");

    status = zx_socket_read(h1, 0u, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");
    EXPECT_EQ(memcmp(rbuf, "abcde", 5), 0, "");

    zx_handle_close(h0);

    // Calling shutdown after the peer is closed is completely valid.
    status = zx_socket_shutdown(h1, ZX_SOCKET_SHUTDOWN_READ);
    EXPECT_EQ(status, ZX_OK, "");

    signals1 = get_satisfied_signals(h1);
    EXPECT_EQ(signals1,
              ZX_SOCKET_PEER_WRITE_DISABLED | ZX_SOCKET_WRITE_DISABLED | ZX_SOCKET_PEER_CLOSED,
              "");

    zx_handle_close(h1);

    END_TEST;
}

bool socket_shutdown_read() {
    BEGIN_TEST;

    zx_status_t status;
    size_t count;
    zx_signals_t signals0, signals1;

    zx_handle_t h0, h1;
    status = zx_socket_create(0, &h0, &h1);
    ASSERT_EQ(status, ZX_OK, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);

    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE, "");

    status = zx_socket_write(h1, 0u, "12345", 5u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");

    status = zx_socket_shutdown(h0, ZX_SOCKET_SHUTDOWN_READ);
    EXPECT_EQ(status, ZX_OK, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);

    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE | ZX_SOCKET_PEER_WRITE_DISABLED,
              "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITE_DISABLED, "");

    status = zx_socket_write(h0, 0u, "abcde", 5u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");

    signals1 = get_satisfied_signals(h1);
    EXPECT_EQ(signals1, ZX_SOCKET_READABLE | ZX_SOCKET_WRITE_DISABLED, "");

    status = zx_socket_write(h1, 0u, "fghij", 5u, &count);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");

    char rbuf[10] = {0};

    status = zx_socket_read(h0, 0u, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");
    EXPECT_EQ(memcmp(rbuf, "12345", 5), 0, "");

    status = zx_socket_read(h0, 0u, rbuf, 1u, &count);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");

    signals0 = get_satisfied_signals(h0);
    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_PEER_WRITE_DISABLED, "");

    status = zx_socket_read(h1, 0u, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");
    EXPECT_EQ(memcmp(rbuf, "abcde", 5), 0, "");

    zx_handle_close(h0);
    zx_handle_close(h1);

    END_TEST;
}

bool socket_bytes_outstanding() {
    BEGIN_TEST;

    zx_status_t status;
    size_t count;

    zx_handle_t h[2];
    uint32_t read_data[] = {0, 0};

    status = zx_socket_create(0, h, h + 1);
    ASSERT_EQ(status, ZX_OK, "");

    status = zx_socket_read(h[0], 0u, read_data, sizeof(read_data), &count);
    EXPECT_EQ(status, ZX_ERR_SHOULD_WAIT, "");

    constexpr uint32_t write_data[] = {0xdeadbeef, 0xc0ffee};
    status = zx_socket_write(h[0], 0u, &write_data[0], sizeof(write_data[0]), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, sizeof(write_data[0]), "");
    status = zx_socket_write(h[0], 0u, &write_data[1], sizeof(write_data[1]), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, sizeof(write_data[1]), "");

    // Check the number of bytes outstanding.
    zx_info_socket_t info;
    memset(&info, 0, sizeof(info));
    status = zx_object_get_info(h[1], ZX_INFO_SOCKET, &info, sizeof(info),
                                NULL, NULL);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(info.rx_buf_available, sizeof(write_data), "");

    // Check that the prior zx_socket_read call didn't disturb the pending data.
    status = zx_socket_read(h[1], 0u, read_data, sizeof(read_data), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, sizeof(read_data), "");
    EXPECT_EQ(read_data[0], write_data[0], "");
    EXPECT_EQ(read_data[1], write_data[1], "");

    zx_handle_close(h[1]);

    status = zx_socket_write(h[0], 0u, &write_data[1], sizeof(write_data[1]), &count);
    EXPECT_EQ(status, ZX_ERR_PEER_CLOSED, "");

    zx_handle_close(h[0]);

    END_TEST;
}

bool socket_bytes_outstanding_shutdown_write() {
    BEGIN_TEST;

    zx_status_t status;
    size_t count;
    zx_signals_t signals0, signals1;

    zx_handle_t h0, h1;
    status = zx_socket_create(0, &h0, &h1);
    ASSERT_EQ(status, ZX_OK, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);

    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE, "");

    status = zx_socket_write(h1, 0u, "12345", 5u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");

    status = zx_socket_shutdown(h1, ZX_SOCKET_SHUTDOWN_WRITE);
    EXPECT_EQ(status, ZX_OK, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);

    EXPECT_EQ(signals0,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE | ZX_SOCKET_PEER_WRITE_DISABLED,
              "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITE_DISABLED, "");

    status = zx_socket_write(h0, 0u, "abcde", 5u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");

    signals1 = get_satisfied_signals(h1);
    EXPECT_EQ(signals1, ZX_SOCKET_READABLE | ZX_SOCKET_WRITE_DISABLED, "");

    status = zx_socket_write(h1, 0u, "fghij", 5u, &count);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");

    char rbuf[10] = {0};

    zx_info_socket_t info;
    memset(&info, 0, sizeof(info));
    status = zx_object_get_info(h0, ZX_INFO_SOCKET, &info, sizeof(info),
                                NULL, NULL);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(info.rx_buf_available, 5u, "");
    count = 0;

    status = zx_socket_read(h0, 0u, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");
    EXPECT_EQ(memcmp(rbuf, "12345", 5), 0, "");

    status = zx_socket_read(h0, 0u, rbuf, 1u, &count);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");

    signals0 = get_satisfied_signals(h0);
    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_PEER_WRITE_DISABLED, "");

    status = zx_socket_read(h1, 0u, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");
    EXPECT_EQ(memcmp(rbuf, "abcde", 5), 0, "");

    zx_handle_close(h0);
    zx_handle_close(h1);

    END_TEST;
}

bool socket_bytes_outstanding_shutdown_read() {
    BEGIN_TEST;

    zx_status_t status;
    size_t count;
    zx_signals_t signals0, signals1;

    zx_handle_t h0, h1;
    status = zx_socket_create(0, &h0, &h1);
    ASSERT_EQ(status, ZX_OK, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);

    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE, "");

    status = zx_socket_write(h1, 0u, "12345", 5u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");

    status = zx_socket_shutdown(h0, ZX_SOCKET_SHUTDOWN_READ);
    EXPECT_EQ(status, ZX_OK, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);

    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_READABLE | ZX_SOCKET_PEER_WRITE_DISABLED,
              "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITE_DISABLED, "");

    status = zx_socket_write(h0, 0u, "abcde", 5u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");

    signals1 = get_satisfied_signals(h1);
    EXPECT_EQ(signals1, ZX_SOCKET_READABLE | ZX_SOCKET_WRITE_DISABLED, "");

    status = zx_socket_write(h1, 0u, "fghij", 5u, &count);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");

    char rbuf[10] = {0};

    zx_info_socket_t info;
    memset(&info, 0, sizeof(info));
    status = zx_object_get_info(h0, ZX_INFO_SOCKET, &info, sizeof(info),
                                NULL, NULL);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(info.rx_buf_available, 5u, "");
    count = 0;

    status = zx_socket_read(h0, 0u, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");
    EXPECT_EQ(memcmp(rbuf, "12345", 5), 0, "");

    status = zx_socket_read(h0, 0u, rbuf, 1u, &count);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");

    signals0 = get_satisfied_signals(h0);
    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_PEER_WRITE_DISABLED, "");

    status = zx_socket_read(h1, 0u, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");
    EXPECT_EQ(memcmp(rbuf, "abcde", 5), 0, "");

    zx_handle_close(h0);
    zx_handle_close(h1);

    END_TEST;
}

bool socket_short_write() {
    BEGIN_TEST;

    zx_status_t status;

    zx_handle_t h0, h1;
    status = zx_socket_create(0, &h0, &h1);
    ASSERT_EQ(status, ZX_OK, "");

    // TODO(qsr): Request socket buffer and use (socket_buffer + 1).
    const size_t buffer_size = 256 * 1024 + 1;
    fbl::Array<char> buffer(new char[buffer_size], buffer_size);
    size_t written = ~(size_t)0; // This should get overwritten by the syscall.
    status = zx_socket_write(h0, 0u, buffer.get(), buffer_size, &written);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_LT(written, buffer_size, "");

    zx_handle_close(h0);
    zx_handle_close(h1);

    END_TEST;
}

bool socket_datagram() {
    BEGIN_TEST;

    size_t count;
    zx_status_t status;
    zx_handle_t h0, h1;
    unsigned char rbuf[4096] = {0}; // bigger than an mbuf

    status = zx_socket_create(ZX_SOCKET_DATAGRAM, &h0, &h1);
    ASSERT_EQ(status, ZX_OK, "");

    status = zx_socket_write(h0, 0u, "packet1", 8u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 8u, "");

    status = zx_socket_write(h0, 0u, "pkt2", 5u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");

    rbuf[0] = 'a';
    rbuf[1000] = 'b';
    rbuf[2000] = 'c';
    rbuf[3000] = 'd';
    rbuf[4000] = 'e';
    rbuf[4095] = 'f';
    status = zx_socket_write(h0, 0u, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, sizeof(rbuf), "");

    zx_info_socket_t info;
    memset(&info, 0, sizeof(info));
    status = zx_object_get_info(h1, ZX_INFO_SOCKET, &info, sizeof(info),
                                NULL, NULL);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(info.rx_buf_available, 8u, "");
    count = 0;

    bzero(rbuf, sizeof(rbuf));
    status = zx_socket_read(h1, 0u, rbuf, 3, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 3u, "");
    EXPECT_EQ(memcmp(rbuf, "pac", 4), 0, ""); // short read "packet1"
    count = 0;

    memset(&info, 0, sizeof(info));
    status = zx_object_get_info(h1, ZX_INFO_SOCKET, &info, sizeof(info),
                                NULL, NULL);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(info.rx_buf_available, 5u, "");
    count = 0;

    status = zx_socket_read(h1, 0u, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");
    EXPECT_EQ(memcmp(rbuf, "pkt2", 5), 0, "");

    status = zx_socket_read(h1, 0u, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, sizeof(rbuf), "");
    EXPECT_EQ(rbuf[0], 'a', "");
    EXPECT_EQ(rbuf[1000], 'b', "");
    EXPECT_EQ(rbuf[2000], 'c', "");
    EXPECT_EQ(rbuf[3000], 'd', "");
    EXPECT_EQ(rbuf[4000], 'e', "");
    EXPECT_EQ(rbuf[4095], 'f', "");

    memset(&info, 0, sizeof(info));
    status = zx_object_get_info(h1, ZX_INFO_SOCKET, &info, sizeof(info),
                                NULL, NULL);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(info.rx_buf_available, 0u, "");

    zx_handle_close(h0);
    zx_handle_close(h1);

    END_TEST;
}

bool socket_datagram_peek() {
    BEGIN_TEST;

    size_t count;
    zx_handle_t h0, h1;

    ASSERT_EQ(zx_socket_create(ZX_SOCKET_DATAGRAM, &h0, &h1), ZX_OK, "");

    EXPECT_EQ(zx_socket_write(h0, 0u, "pkt1", 5u, &count), ZX_OK, "");
    EXPECT_EQ(zx_socket_write(h0, 0u, "pkt2", 5u, &count), ZX_OK, "");

    char buffer[16];

    // Short peek.
    EXPECT_EQ(zx_socket_read(h1, ZX_SOCKET_PEEK, buffer, 3, &count), ZX_OK, "");
    EXPECT_EQ(count, 3u, "");
    EXPECT_EQ(memcmp(buffer, "pkt", 3), 0, "");

    // Full peek should still see the 1st packet.
    EXPECT_EQ(zx_socket_read(h1, ZX_SOCKET_PEEK, buffer, 5, &count), ZX_OK, "");
    EXPECT_EQ(count, 5u, "");
    EXPECT_EQ(memcmp(buffer, "pkt1", 5), 0, "");

    // Read and consume the 1st packet.
    EXPECT_EQ(zx_socket_read(h1, 0u, buffer, 5, &count), ZX_OK, "");

    // Now peek should see the 2nd packet.
    EXPECT_EQ(zx_socket_read(h1, ZX_SOCKET_PEEK, buffer, 5, &count), ZX_OK, "");
    EXPECT_EQ(count, 5u, "");
    EXPECT_EQ(memcmp(buffer, "pkt2", 5), 0, "");

    zx_handle_close(h0);
    zx_handle_close(h1);

    END_TEST;
}

bool socket_datagram_peek_empty() {
    BEGIN_TEST;

    zx_handle_t h[2];
    ASSERT_EQ(zx_socket_create(ZX_SOCKET_DATAGRAM, h, h + 1), ZX_OK, "");

    size_t count;
    char data;
    EXPECT_EQ(zx_socket_read(h[0], ZX_SOCKET_PEEK, &data, sizeof(data), &count),
              ZX_ERR_SHOULD_WAIT, "");

    zx_handle_close(h[0]);
    zx_handle_close(h[1]);

    END_TEST;
}

bool socket_datagram_no_short_write() {
    BEGIN_TEST;

    zx_status_t status;

    zx_handle_t h0, h1;
    status = zx_socket_create(ZX_SOCKET_DATAGRAM, &h0, &h1);
    ASSERT_EQ(status, ZX_OK, "");

    zx_info_socket_t info;
    memset(&info, 0, sizeof(info));
    status = zx_object_get_info(h1, ZX_INFO_SOCKET, &info, sizeof(info), NULL, NULL);
    ASSERT_EQ(status, ZX_OK, "zx_object_get_info");
    EXPECT_GT(info.tx_buf_max, 0u, "");

    // Pick a size for a huge datagram, and make sure not to overflow.
    size_t buffer_size = info.tx_buf_max * 2;
    EXPECT_GT(buffer_size, 0u, "");

    fbl::Array<char> buffer(new char[buffer_size]{}, buffer_size);
    EXPECT_NONNULL(buffer.get(), "");

    size_t written = ~0u;
    status = zx_socket_write(h0, 0u, buffer.get(), buffer_size, &written);
    EXPECT_EQ(status, ZX_ERR_OUT_OF_RANGE, "");
    // Since the syscall failed, it should not have overwritten this output
    // parameter.
    EXPECT_EQ(written, ~0u, "");

    zx_handle_close(h0);
    zx_handle_close(h1);

    END_TEST;
}

bool socket_control_plane_absent() {
    BEGIN_TEST;

    zx_status_t status;

    zx_handle_t h0, h1;
    status = zx_socket_create(0, &h0, &h1);
    ASSERT_EQ(status, ZX_OK, "");

    status = zx_socket_write(h0, ZX_SOCKET_CONTROL, "hi", 2u, NULL);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");

    status = zx_socket_write(h1, ZX_SOCKET_CONTROL, "hi", 2u, NULL);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");

    size_t count;
    char rbuf[10] = {0};

    status = zx_socket_read(h0, ZX_SOCKET_CONTROL, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");

    status = zx_socket_read(h1, ZX_SOCKET_CONTROL, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");

    END_TEST;
}

bool socket_control_plane() {
    BEGIN_TEST;

    zx_status_t status;

    zx_handle_t h0, h1;
    status = zx_socket_create(ZX_SOCKET_HAS_CONTROL, &h0, &h1);
    ASSERT_EQ(status, ZX_OK, "");

    zx_signals_t signals0 = get_satisfied_signals(h0);
    zx_signals_t signals1 = get_satisfied_signals(h1);
    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_WRITABLE, "");

    // Write to the control plane.
    size_t count;
    status = zx_socket_write(h0, ZX_SOCKET_CONTROL, "hello1", 6u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 6u, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);
    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals1,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_READABLE | ZX_SOCKET_CONTROL_WRITABLE, "");

    status = zx_socket_write(h0, ZX_SOCKET_CONTROL, "hi", 2u, NULL);
    EXPECT_EQ(status, ZX_ERR_SHOULD_WAIT, "");

    status = zx_socket_write(h1, ZX_SOCKET_CONTROL, "hello0", 6u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 6u, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);
    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_READABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_READABLE, "");

    status = zx_socket_write(h1, ZX_SOCKET_CONTROL, "hi", 2u, NULL);
    EXPECT_EQ(status, ZX_ERR_SHOULD_WAIT, "");

    char rbuf[10] = {0};

    // The control plane is independent of normal reads and writes.
    status = zx_socket_read(h0, 0, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_ERR_SHOULD_WAIT, "");
    status = zx_socket_read(h1, 0, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_ERR_SHOULD_WAIT, "");
    status = zx_socket_write(h0, 0, "normal", 7u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 7u, "");
    status = zx_socket_read(h1, 0, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 7u, "");
    EXPECT_EQ(memcmp(rbuf, "normal", 7), 0, "");

    // Read from the control plane.
    status = zx_socket_read(h0, ZX_SOCKET_CONTROL, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 6u, "");
    EXPECT_EQ(memcmp(rbuf, "hello0", 6), 0, "");

    status = zx_socket_read(h0, ZX_SOCKET_CONTROL, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_ERR_SHOULD_WAIT, "");

    status = zx_socket_read(h1, ZX_SOCKET_CONTROL, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 6u, "");
    EXPECT_EQ(memcmp(rbuf, "hello1", 6), 0, "");

    status = zx_socket_read(h1, ZX_SOCKET_CONTROL, rbuf, sizeof(rbuf), &count);
    EXPECT_EQ(status, ZX_ERR_SHOULD_WAIT, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);
    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_WRITABLE, "");

    END_TEST;
}

bool socket_control_plane_peek() {
    BEGIN_TEST;

    zx_handle_t h0, h1;
    ASSERT_EQ(zx_socket_create(ZX_SOCKET_HAS_CONTROL, &h0, &h1), ZX_OK, "");

    size_t count;
    EXPECT_EQ(zx_socket_write(h0, ZX_SOCKET_CONTROL, "hello0", 6u, &count), ZX_OK, "");
    EXPECT_EQ(count, 6u, "");

    char rbuf[10] = {0};

    EXPECT_EQ(zx_socket_read(h1, ZX_SOCKET_CONTROL | ZX_SOCKET_PEEK, rbuf, sizeof(rbuf), &count),
              ZX_OK, "");
    EXPECT_EQ(count, 6u, "");
    EXPECT_EQ(memcmp(rbuf, "hello0", 6), 0, "");

    // The message should still be pending for h1 to read.
    EXPECT_EQ(get_satisfied_signals(h0), ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(get_satisfied_signals(h1),
              ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_WRITABLE | ZX_SOCKET_CONTROL_READABLE, "");

    rbuf[0] = '\0';
    EXPECT_EQ(zx_socket_read(h1, ZX_SOCKET_CONTROL, rbuf, sizeof(rbuf), &count), ZX_OK, "");
    EXPECT_EQ(count, 6u, "");
    EXPECT_EQ(memcmp(rbuf, "hello0", 6), 0, "");

    EXPECT_EQ(get_satisfied_signals(h0), ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_WRITABLE, "");
    EXPECT_EQ(get_satisfied_signals(h1), ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_WRITABLE, "");

    zx_handle_close(h0);
    zx_handle_close(h1);

    END_TEST;
}

bool socket_control_plane_peek_empty() {
    BEGIN_TEST;

    zx_handle_t h[2];
    ASSERT_EQ(zx_socket_create(ZX_SOCKET_HAS_CONTROL, h, h + 1), ZX_OK, "");

    size_t count;
    char data;
    EXPECT_EQ(zx_socket_read(h[0], ZX_SOCKET_CONTROL | ZX_SOCKET_PEEK, &data, sizeof(data), &count),
              ZX_ERR_SHOULD_WAIT, "");

    zx_handle_close(h[0]);
    zx_handle_close(h[1]);

    END_TEST;
}

bool socket_control_plane_shutdown() {
    BEGIN_TEST;

    zx_status_t status;
    size_t count;

    zx_handle_t h0, h1;
    status = zx_socket_create(ZX_SOCKET_HAS_CONTROL, &h0, &h1);
    ASSERT_EQ(status, ZX_OK, "");

    zx_signals_t signals0 = get_satisfied_signals(h0);
    zx_signals_t signals1 = get_satisfied_signals(h1);
    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_WRITABLE, "");

    status = zx_socket_write(h1, 0u, "12345", 5u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 5u, "");

    status = zx_socket_shutdown(h1, ZX_SOCKET_SHUTDOWN_WRITE);
    EXPECT_EQ(status, ZX_OK, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);
    EXPECT_EQ(signals0,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_WRITABLE | ZX_SOCKET_READABLE |
                  ZX_SOCKET_PEER_WRITE_DISABLED,
              "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITE_DISABLED | ZX_SOCKET_CONTROL_WRITABLE, "");

    status = zx_socket_write(h0, ZX_SOCKET_CONTROL, "hello1", 6u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 6u, "");

    status = zx_socket_write(h1, ZX_SOCKET_CONTROL, "hello0", 6u, &count);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(count, 6u, "");

    signals0 = get_satisfied_signals(h0);
    signals1 = get_satisfied_signals(h1);
    EXPECT_EQ(signals0,
              ZX_SOCKET_WRITABLE | ZX_SOCKET_CONTROL_READABLE | ZX_SOCKET_READABLE |
                  ZX_SOCKET_PEER_WRITE_DISABLED,
              "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITE_DISABLED | ZX_SOCKET_CONTROL_READABLE, "");

    END_TEST;
}

bool socket_accept() {
    BEGIN_TEST;

    zx_status_t status;

    zx_handle_t a0, a1;
    status = zx_socket_create(ZX_SOCKET_HAS_ACCEPT, &a0, &a1);
    ASSERT_EQ(status, ZX_OK, "");

    zx_handle_t b0, b1;
    status = zx_socket_create(0, &b0, &b1);
    ASSERT_EQ(status, ZX_OK, "");

    zx_handle_t c0, c1;
    status = zx_socket_create(0, &c0, &c1);
    ASSERT_EQ(status, ZX_OK, "");

    zx_signals_t signals0 = get_satisfied_signals(a0);
    zx_signals_t signals1 = get_satisfied_signals(a1);
    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_SHARE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE | ZX_SOCKET_SHARE, "");

    // cannot share a HAS_ACCEPT socket
    status = zx_socket_share(b0, a0);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");
    zx_handle_close(a1);
    status = zx_socket_create(ZX_SOCKET_HAS_ACCEPT, &a0, &a1);
    ASSERT_EQ(status, ZX_OK, "");

    // cannot share via a non-HAS_ACCEPT socket
    status = zx_socket_share(b0, c0);
    EXPECT_EQ(status, ZX_ERR_NOT_SUPPORTED, "");

    // cannot share a socket via itself (either direction)
    status = zx_socket_share(a0, a0);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");
    zx_handle_close(a1);
    status = zx_socket_create(ZX_SOCKET_HAS_ACCEPT, &a0, &a1);
    ASSERT_EQ(status, ZX_OK, "");

    status = zx_socket_share(a0, a1);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");
    zx_handle_close(a0);
    status = zx_socket_create(ZX_SOCKET_HAS_ACCEPT, &a0, &a1);
    ASSERT_EQ(status, ZX_OK, "");

    status = zx_socket_share(a1, a0);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");
    zx_handle_close(a1);
    status = zx_socket_create(ZX_SOCKET_HAS_ACCEPT, &a0, &a1);
    ASSERT_EQ(status, ZX_OK, "");

    status = zx_socket_share(a1, a1);
    EXPECT_EQ(status, ZX_ERR_BAD_STATE, "");
    zx_handle_close(a0);
    status = zx_socket_create(ZX_SOCKET_HAS_ACCEPT, &a0, &a1);
    ASSERT_EQ(status, ZX_OK, "");

    // cannot accept from a non-HAS_ACCEPT socket
    zx_handle_t h;
    status = zx_socket_accept(b0, &h);
    EXPECT_EQ(status, ZX_ERR_NOT_SUPPORTED, "");

    status = zx_socket_share(a0, b0);
    EXPECT_EQ(status, ZX_OK, "");

    signals0 = get_satisfied_signals(a0);
    signals1 = get_satisfied_signals(a1);
    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE | ZX_SOCKET_SHARE | ZX_SOCKET_ACCEPT, "");

    // queue is only one deep
    status = zx_socket_share(a0, b1);
    EXPECT_EQ(status, ZX_ERR_SHOULD_WAIT, "");

    status = zx_socket_accept(a1, &h);
    EXPECT_EQ(status, ZX_OK, "");
    b0 = h;

    // queue is only one deep
    status = zx_socket_accept(a0, &h);
    EXPECT_EQ(status, ZX_ERR_SHOULD_WAIT, "");

    signals0 = get_satisfied_signals(a0);
    signals1 = get_satisfied_signals(a1);
    EXPECT_EQ(signals0, ZX_SOCKET_WRITABLE | ZX_SOCKET_SHARE, "");
    EXPECT_EQ(signals1, ZX_SOCKET_WRITABLE | ZX_SOCKET_SHARE, "");

    zx_handle_close(a0);
    zx_handle_close(a1);
    zx_handle_close(b0);
    zx_handle_close(b1);
    zx_handle_close(c0);
    zx_handle_close(c1);

    END_TEST;
}

bool socket_share_invalid_handle() {
    BEGIN_TEST;

    zx_handle_t socket[2];
    zx_status_t status = zx_socket_create(0, &socket[0], &socket[1]);
    EXPECT_EQ(status, ZX_OK, "");

    status = zx_socket_share(socket[0], ZX_HANDLE_INVALID);
    EXPECT_EQ(status, ZX_ERR_BAD_HANDLE, "");

    zx_handle_close(socket[0]);
    zx_handle_close(socket[1]);

    END_TEST;
}

bool socket_zero_size() {
    BEGIN_TEST;

    zx_handle_t socket[2];
    zx_status_t status = zx_socket_create(0, &socket[0], &socket[1]);
    EXPECT_EQ(status, ZX_OK, "");

    char buffer;
    status = zx_socket_read(socket[0], 0, &buffer, 0, NULL);
    EXPECT_EQ(status, ZX_ERR_SHOULD_WAIT, "");

    status = zx_socket_write(socket[0], 0, "a", 1, NULL);
    EXPECT_EQ(status, ZX_OK, "");

    status = zx_socket_read(socket[0], 0, &buffer, 0, NULL);
    EXPECT_EQ(status, ZX_OK, "");

    zx_handle_close(socket[0]);
    zx_handle_close(socket[1]);

    status = zx_socket_create(ZX_SOCKET_DATAGRAM, &socket[0], &socket[1]);
    EXPECT_EQ(status, ZX_OK, "");

    status = zx_socket_read(socket[0], 0, &buffer, 0, NULL);
    EXPECT_EQ(status, ZX_ERR_SHOULD_WAIT, "");

    status = zx_socket_write(socket[0], 0, "a", 1, NULL);
    EXPECT_EQ(status, ZX_OK, "");

    status = zx_socket_read(socket[0], 0, &buffer, 0, NULL);
    EXPECT_EQ(status, ZX_OK, "");

    zx_handle_close(socket[0]);
    zx_handle_close(socket[1]);

    END_TEST;
}

bool socket_share_consumes_on_failure() {
    BEGIN_TEST;

    zx_handle_t socket[2];
    zx_status_t status = zx_socket_create(0, &socket[0], &socket[1]);
    EXPECT_EQ(status, ZX_OK, "");

    zx_handle_t eventpair[2];
    status = zx_eventpair_create(0, &eventpair[0], &eventpair[1]);
    EXPECT_EQ(status, ZX_OK, "");

    status = zx_socket_share(socket[0], eventpair[0]);
    EXPECT_EQ(status, ZX_ERR_WRONG_TYPE, "");

    // eventpair[0] should have been closed, which we can detect by
    // waiting on its peer.
    zx_signals_t signals;
    status = zx_object_wait_one(eventpair[1], ZX_EVENTPAIR_PEER_CLOSED, 0u, &signals);
    EXPECT_EQ(status, ZX_OK, "");
    EXPECT_EQ(signals & ZX_EVENTPAIR_PEER_CLOSED, ZX_EVENTPAIR_PEER_CLOSED, "");

    zx_handle_close(socket[0]);
    zx_handle_close(socket[1]);
    zx_handle_close(eventpair[1]);

    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(socket_tests)
RUN_TEST(socket_basic)
RUN_TEST(socket_peek)
RUN_TEST(socket_peek_empty)
RUN_TEST(socket_signals)
RUN_TEST(socket_peer_closed_signal)
RUN_TEST(socket_peer_closed_set_property)
RUN_TEST(socket_shutdown_write)
RUN_TEST(socket_shutdown_read)
RUN_TEST(socket_bytes_outstanding)
RUN_TEST(socket_bytes_outstanding_shutdown_write)
RUN_TEST(socket_bytes_outstanding_shutdown_read)
RUN_TEST(socket_short_write)
RUN_TEST(socket_datagram)
RUN_TEST(socket_datagram_peek)
RUN_TEST(socket_datagram_peek_empty)
RUN_TEST(socket_datagram_no_short_write)
RUN_TEST(socket_control_plane_absent)
RUN_TEST(socket_control_plane)
RUN_TEST(socket_control_plane_peek)
RUN_TEST(socket_control_plane_peek_empty)
RUN_TEST(socket_control_plane_shutdown)
RUN_TEST(socket_accept)
RUN_TEST(socket_share_invalid_handle)
RUN_TEST(socket_share_consumes_on_failure)
RUN_TEST(socket_signals2)
END_TEST_CASE(socket_tests)

#ifndef BUILD_COMBINED_TESTS
int main(int argc, char** argv) {
    return unittest_run_all_tests(argc, argv) ? 0 : -1;
}
#endif
