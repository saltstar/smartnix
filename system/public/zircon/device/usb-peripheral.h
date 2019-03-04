
#pragma once


#include <stdint.h>

typedef uint32_t usb_mode_t;
#define USB_MODE_NONE ((usb_mode_t)0)
#define USB_MODE_HOST ((usb_mode_t)1)
#define USB_MODE_PERIPHERAL ((usb_mode_t)2)
#define USB_MODE_OTG ((usb_mode_t)3)
