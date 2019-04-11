// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <threads.h>

#include <fbl/unique_fd.h>
#include <fbl/unique_ptr.h>
#include <fuchsia/device/test/c/fidl.h>
#include <lib/devmgr-integration-test/fixture.h>
#include <lib/fdio/util.h>
#include <lib/zx/channel.h>
#include <lib/zx/socket.h>
#include <lib/zx/time.h>
#include <unittest/unittest.h>
#include <zircon/device/device.h>
#include <zircon/status.h>
#include <zircon/syscalls.h>

#define DRIVER_TEST_DIR "/boot/driver/test"

using devmgr_integration_test::IsolatedDevmgr;

namespace {

void do_one_test(const fbl::unique_ptr<IsolatedDevmgr>& devmgr, const zx::channel& test_root,
                 const char* drv_libname, const zx::socket& output,
                 fuchsia_device_test_TestReport* report) {
    // Initialize the report with a failure state to handle early returns
    *report = {
        .test_count = 1,
        .success_count = 0,
        .failure_count = 1,
    };

    char devpath[fuchsia_device_test_MAX_DEVICE_PATH_LEN+1];
    size_t devpath_count;
    zx_status_t call_status;
    zx_status_t status = fuchsia_device_test_RootDeviceCreateDevice(
            test_root.get(), drv_libname, strlen(drv_libname),
            &call_status, devpath, sizeof(devpath) - 1, &devpath_count);
    if (status == ZX_OK) {
        status = call_status;
    }
    if (status != ZX_OK) {
        printf("driver-tests: error %s creating device for %s\n", zx_status_get_string(status),
               drv_libname);
        return;
    }
    devpath[devpath_count] = 0;

    const char* kDevPrefix = "/dev/";
    if (strncmp(devpath, kDevPrefix, strlen(kDevPrefix))) {
        printf("driver-tests: bad path when creating device for %s: %s\n", drv_libname, devpath);
        return;
    }

    const char* relative_devpath = devpath + strlen(kDevPrefix);

    // TODO some waiting needed before opening..,
    usleep(1000);

    fbl::unique_fd fd;
    int retry = 0;
    do {
        fd.reset(openat(devmgr->devfs_root().get(), relative_devpath, O_RDWR));
        if (fd.is_valid()) {
            break;
        }
        usleep(1000);
    } while (++retry < 100);

    if (retry == 100) {
        printf("driver-tests: failed to open %s\n", devpath);
        return;
    }

    char libpath[PATH_MAX];
    int n = snprintf(libpath, sizeof(libpath), "%s/%s", DRIVER_TEST_DIR, drv_libname);
    ssize_t rc = ioctl_device_bind(fd.get(), libpath, n);
    if (rc < 0) {
        printf("driver-tests: error %zd binding to %s\n", rc, libpath);
        // TODO(teisenbe): I think fuchsia_device_test_DeviceDestroy() should be called
        // here?
        return;
    }

    zx::channel test_channel;
    status = fdio_get_service_handle(fd.release(), test_channel.reset_and_get_address());
    if (status != ZX_OK) {
        printf("driver-tests: failed to get channel %s\n", zx_status_get_string(status));
        return;
    }

    zx::socket output_copy;
    status = output.duplicate(ZX_RIGHT_SAME_RIGHTS, &output_copy);
    if (status != ZX_OK) {
        printf("driver-tests: error %d duplicating output socket\n", status);
        // TODO(teisenbe): I think fuchsia_device_test_DeviceDestroy() should be called
        // here?
        return;
    }

    fuchsia_device_test_DeviceSetOutputSocket(test_channel.get(), output_copy.release());

    status = fuchsia_device_test_DeviceRunTests(test_channel.get(), &call_status, report);
    if (status == ZX_OK) {
        status = call_status;
    }
    if (status != ZX_OK) {
        printf("driver-tests: error %s running tests\n", zx_status_get_string(status));
        *report = {
            .test_count = 1,
            .success_count = 0,
            .failure_count = 1,
        };
    }

    fuchsia_device_test_DeviceDestroy(test_channel.get());
}

int output_thread(void* arg) {
    zx::socket h(static_cast<zx_handle_t>(reinterpret_cast<uintptr_t>(arg)));
    char buf[1024];
    for (;;) {
        zx_status_t status = h.wait_one(ZX_SOCKET_READABLE | ZX_SOCKET_PEER_CLOSED,
                                        zx::time::infinite(), NULL);
        if (status != ZX_OK) {
            break;
        }
        size_t bytes = 0;
        status = h.read(0u, buf, sizeof(buf), &bytes);
        if (status != ZX_OK) {
            break;
        }
        size_t written = 0;
        while (written < bytes) {
            ssize_t rc = write(STDERR_FILENO, buf + written, bytes - written);
            if (rc < 0) {
                break;
            }
            written += rc;
        }
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    auto args = IsolatedDevmgr::DefaultArgs();

    fbl::unique_ptr<IsolatedDevmgr> devmgr;
    zx_status_t status = IsolatedDevmgr::Create(std::move(args), &devmgr);
    if (status != ZX_OK) {
        printf("driver-tests: failed to create isolated devmgr\n");
        return -1;
    }

    zx::socket local_socket, remote_socket;
    status = zx::socket::create(0u, &local_socket, &remote_socket);
    if (status != ZX_OK) {
        printf("driver-tests: error creating socket\n");
        return -1;
    }

    // Wait for /dev/test/test to appear
    fbl::unique_fd fd;
    status = devmgr_integration_test::RecursiveWaitForFile(devmgr->devfs_root(), "test/test",
                                                           zx::deadline_after(zx::sec(5)), &fd);
    if (status != ZX_OK) {
        printf("driver-tests: failed to find /dev/test/test\n");
        return -1;
    }

    zx::channel test_root;
    status = fdio_get_service_handle(fd.release(), test_root.reset_and_get_address());
    if (status != ZX_OK) {
        printf("driver-tests: failed to get root channel %s\n", zx_status_get_string(status));
        return -1;
    }

    thrd_t t;
    int rc = thrd_create_with_name(&t, output_thread,
                                   reinterpret_cast<void*>(local_socket.release()),
                                   "driver-test-output");
    if (rc != thrd_success) {
        printf("driver-tests: error %d creating output thread\n", rc);
        return -1;
    }

    fuchsia_device_test_TestReport final_report = {};

    DIR* dir = opendir(DRIVER_TEST_DIR);
    if (dir == NULL) {
        printf("driver-tests: failed to open %s\n", DRIVER_TEST_DIR);
        return -1;
    }
    int dfd = dirfd(dir);
    if (dfd < 0) {
        printf("driver-tests: failed to get fd for %s\n", DRIVER_TEST_DIR);
        return -1;
    }
    struct dirent* de;
    // bind test drivers
    while ((de = readdir(dir)) != NULL) {
        if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0)) {
            continue;
        }
        // Don't try to bind the fake sysdev or the mock device
        if (strcmp(de->d_name, "sysdev.so") == 0 ||
            strcmp(de->d_name, "mock-device.so") == 0) {
            continue;
        }
        fuchsia_device_test_TestReport one_report = {};
        do_one_test(devmgr, test_root, de->d_name, remote_socket, &one_report);

        final_report.test_count += one_report.test_count;
        final_report.success_count += one_report.success_count;
        final_report.failure_count += one_report.failure_count;
    }

    // close this handle before thrd_join to get PEER_CLOSED in output thread
    remote_socket.reset();

    thrd_join(t, NULL);

    unittest_printf_critical(
            "\n====================================================\n");
    unittest_printf_critical(
            "    CASES:  %d     SUCCESS:  %d     FAILED:  %d   ",
            final_report.test_count, final_report.success_count, final_report.failure_count);
    unittest_printf_critical(
            "\n====================================================\n");

    return final_report.failure_count == 0 ? 0 : -1;
}
