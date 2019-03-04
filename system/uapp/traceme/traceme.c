
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <zircon/device/ktrace.h>

// 1. Run:            zircon> traceme
// 2. Stop tracing:   zircon> dm ktraceoff
// 3. Grab trace:     host> netcp :/dev/misc/ktrace test.trace
// 4. Examine trace:  host> tracevic test.trace

int main(int argc, char** argv) {
    int fd;
    if ((fd = open("/dev/misc/ktrace", O_RDWR)) < 0) {
        fprintf(stderr, "cannot open trace device\n");
        return -1;
    }

    // obtain the handle needed to emit probes
    zx_handle_t kth;
    if (ioctl_ktrace_get_handle(fd, &kth) < 0) {
        fprintf(stderr, "cannot get ktrace handle\n");
        return -1;
    }

    // for each probe/event, register its name and get its id
    uint32_t id;
    if (ioctl_ktrace_add_probe(fd, "trace-me", &id) < 0) {
        fprintf(stderr, "cannot register ktrace probe\n");
        return -1;
    }

    // once all probes are registered, you can close the device
    close(fd);

    // use the ktrace handle to emit probes into the trace stream
    zx_ktrace_write(kth, id, 1, 0);
    printf("hello, ktrace! id = %u\n", id);
    zx_ktrace_write(kth, id, 2, 0);

    return 0;
}
