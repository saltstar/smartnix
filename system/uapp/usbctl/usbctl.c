
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lib/fdio/util.h>
#include <zircon/device/usb-peripheral.h>
#include <zircon/hw/usb.h>
#include <zircon/hw/usb-cdc.h>
#include <zircon/usb/peripheral/c/fidl.h>
#include <zircon/syscalls.h>
#include <zircon/types.h>


#include <zircon/types.h>

#define DEV_USB_PERIPHERAL_DIR  "/dev/class/usb-peripheral"

#define GOOGLE_VID      0x18D1
#define GOOGLE_CDC_PID  0xA020
#define GOOGLE_UMS_PID  0xA021

#define MANUFACTURER_STRING "Zircon"
#define CDC_PRODUCT_STRING  "CDC Ethernet"
#define UMS_PRODUCT_STRING  "USB Mass Storage"
#define SERIAL_STRING       "12345678"

typedef zircon_usb_peripheral_FunctionDescriptor usb_function_descriptor_t;

const usb_function_descriptor_t cdc_function_desc = {
    .interface_class = USB_CLASS_COMM,
    .interface_subclass = USB_CDC_SUBCLASS_ETHERNET,
    .interface_protocol = 0,
};

const usb_function_descriptor_t ums_function_desc = {
    .interface_class = USB_CLASS_MSC,
    .interface_subclass = USB_SUBCLASS_MSC_SCSI,
    .interface_protocol = USB_PROTOCOL_MSC_BULK_ONLY,
};

typedef struct {
    const usb_function_descriptor_t* desc;
    const char* product_string;
    uint16_t vid;
    uint16_t pid;
} usb_function_t;

static const usb_function_t cdc_function = {
    .desc = &cdc_function_desc,
    .product_string = CDC_PRODUCT_STRING,
    .vid = GOOGLE_VID,
    .pid = GOOGLE_CDC_PID,
};

static const usb_function_t ums_function = {
    .desc = &ums_function_desc,
    .product_string = UMS_PRODUCT_STRING,
    .vid = GOOGLE_VID,
    .pid = GOOGLE_UMS_PID,
};

static zircon_usb_peripheral_DeviceDescriptor device_desc = {
    .bcdUSB = htole16(0x0200),
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
//   idVendor and idProduct are filled in later
    .bcdDevice = htole16(0x0100),
//    iManufacturer, iProduct and iSerialNumber are filled in later
    .bNumConfigurations = 1,
};

static int open_usb_device(void) {
    struct dirent* de;
    DIR* dir = opendir(DEV_USB_PERIPHERAL_DIR);
    if (!dir) {
        printf("Error opening %s\n", DEV_USB_PERIPHERAL_DIR);
        return -1;
    }

    while ((de = readdir(dir)) != NULL) {
       char devname[128];

        snprintf(devname, sizeof(devname), "%s/%s", DEV_USB_PERIPHERAL_DIR, de->d_name);
        int fd = open(devname, O_RDWR);
        if (fd < 0) {
            printf("Error opening %s\n", devname);
            continue;
        }

        closedir(dir);
        return fd;
    }

    closedir(dir);
    return -1;
}

static zx_status_t device_init(zx_handle_t svc, const usb_function_t* function) {
    zx_status_t status2;

    device_desc.idVendor = htole16(function->vid);
    device_desc.idProduct = htole16(function->pid);

    // allocate string descriptors
    zx_status_t status = zircon_usb_peripheral_DeviceAllocStringDesc(svc, MANUFACTURER_STRING,
                                                                     strlen(MANUFACTURER_STRING) + 1,
                                                                     &status2, &device_desc.iManufacturer);
    if (status == ZX_OK) status = status2;
    if (status != ZX_OK) {
        fprintf(stderr, "zircon_usb_peripheral_DeviceAllocStringDesc failed: %d\n", status);
        return status;
    }
    status = zircon_usb_peripheral_DeviceAllocStringDesc(svc, function->product_string,
                                                         strlen(function->product_string) + 1,
                                                         &status2, &device_desc.iProduct);
    if (status == ZX_OK) status = status2;
    if (status != ZX_OK) {
        fprintf(stderr, "zircon_usb_peripheral_DeviceAllocStringDesc failed: %d\n", status);
        return status;
    }
    status = zircon_usb_peripheral_DeviceAllocStringDesc(svc, SERIAL_STRING, strlen(SERIAL_STRING) + 1,
                                                         &status2, &device_desc.iSerialNumber);
    if (status == ZX_OK) status = status2;
    if (status != ZX_OK) {
        fprintf(stderr, "zircon_usb_peripheral_DeviceAllocStringDesc failed: %d\n", status);
        return status;
    }

    // set device descriptor
    status = zircon_usb_peripheral_DeviceSetDeviceDescriptor(svc, &device_desc, &status2);
    if (status == ZX_OK) status = status2;
    if (status != ZX_OK) {
        fprintf(stderr, "zircon_usb_peripheral_DeviceSetDeviceDescriptor failed: %d\n", status);
        return status;
    }

    status = zircon_usb_peripheral_DeviceAddFunction(svc, function->desc, &status2);
    if (status == ZX_OK) status = status2;
    if (status != ZX_OK) {
        fprintf(stderr, "zircon_usb_peripheral_DeviceAddFunction failed: %d\n", status);
        return status;
    }

    status = zircon_usb_peripheral_DeviceBindFunctions(svc, &status2);
    if (status == ZX_OK) status = status2;
    if (status != ZX_OK) {
        fprintf(stderr, "zircon_usb_peripheral_DeviceBindFunctions failed: %d\n", status);
    }

    return status;
}

static int peripheral_command(zx_handle_t svc, int argc, const char* argv[]) {
    if (argc != 2) {
        goto usage;
    }

    zx_status_t status, status2;

    const char* command = argv[1];
    if (!strcmp(command, "reset")) {
        status = zircon_usb_peripheral_DeviceClearFunctions(svc, &status2);
        if (status == ZX_OK) status = status2;
    } else if (!strcmp(command, "init-cdc")) {
        status = device_init(svc, &cdc_function);
    } else if (!strcmp(command, "init-ums")) {
        status = device_init(svc, &ums_function);
     } else {
        goto usage;
    }

    return status == ZX_OK ? 0 : -1;

usage:
    fprintf(stderr, "usage: usbctl device [reset|init-ums]\n");
    return -1;
}

static int mode_command(zx_handle_t svc, int argc, const char* argv[]) {
    zx_status_t status = ZX_OK;
    zx_status_t status2;

    if (argc == 1) {
        // print current mode
        usb_mode_t mode;
        status = zircon_usb_peripheral_DeviceGetMode(svc, &status2, &mode);
        if (status == ZX_OK) status = status2;
        if (status != ZX_OK) {
            fprintf(stderr, "zircon_usb_peripheral_DeviceGetMode failed: %d\n", status);
        } else {
            switch (mode) {
            case USB_MODE_NONE:
                printf("NONE\n");
                break;
            case USB_MODE_HOST:
                printf("HOST\n");
                break;
            case USB_MODE_PERIPHERAL:
                printf("PERIPHERAL\n");
                break;
            case USB_MODE_OTG:
                printf("OTG\n");
                break;
            default:
                printf("unknown mode %d\n", mode);
                break;
            }
         }
    } else {
        usb_mode_t mode;
        if (strcasecmp(argv[1], "none") == 0) {
            mode = USB_MODE_NONE;
        } else if (strcasecmp(argv[1], "host") == 0) {
            mode = USB_MODE_HOST;
        } else if (strcasecmp(argv[1], "peripheral") == 0) {
            mode = USB_MODE_PERIPHERAL;
        } else if (strcasecmp(argv[1], "otg") == 0) {
            mode = USB_MODE_OTG;
        } else {
            fprintf(stderr, "unknown USB mode %s\n", argv[1]);
            status = ZX_ERR_INVALID_ARGS;
        }

        if (status == ZX_OK) {
            status = zircon_usb_peripheral_DeviceSetMode(svc, mode, &status2);
            if (status == ZX_OK) status = status2;
            if (status != ZX_OK) {
                fprintf(stderr, "zircon_usb_peripheral_DeviceSetMode failed: %d\n", status);
            }
        }
    }

    return status;
}

typedef struct {
    const char* name;
    int (*command)(zx_handle_t svc, int argc, const char* argv[]);
    const char* description;
} usbctl_command_t;

static usbctl_command_t commands[] = {
    {
        "peripheral",
        peripheral_command,
        "peripheral [reset|init-cdc|init-ums] resets the peripheral or "
        "initializes the UMS function"
    },
    {
        "mode",
        mode_command,
        "mode [none|host|peripheral|otg] sets the current USB mode. "
        "Returns the current mode if no additional arugment is provided."
    },
    { NULL, NULL, NULL },
};

static void usage(void) {
    fprintf(stderr, "usage: \"usbctl <command>\", where command is one of:\n");

    usbctl_command_t* command = commands;
    while (command->name) {
        fprintf(stderr, "    %s\n", command->description);
        command++;
    }
}

int main(int argc, const char** argv) {
    if (argc < 2) {
        usage();
        return -1;
    }

    int fd = open_usb_device();
    if (fd < 0) {
        fprintf(stderr, "could not find a device in %s\n", DEV_USB_PERIPHERAL_DIR);
        return fd;
    }

    zx_handle_t svc;
    zx_status_t status = fdio_get_service_handle(fd, &svc);
    if (status != ZX_OK) {
        close(fd);
        return status;
    }

    const char* command_name = argv[1];
    usbctl_command_t* command = commands;
    while (command->name) {
        if (!strcmp(command_name, command->name)) {
            status = command->command(svc, argc - 1, argv + 1);
            goto done;
        }
        command++;
    }
    // if we fall through, print usage
    usage();
    status = ZX_ERR_INVALID_ARGS;

done:
    zx_handle_close(svc);
    close(fd);
    return status;
}
