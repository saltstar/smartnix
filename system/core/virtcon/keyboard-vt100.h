
#pragma once

#include <stdint.h>
#include <stdlib.h>

#include <hid/hid.h>

// Converts the given HID keycode into an equivalent VT100/ANSI byte
// sequence, for the given modifier key state and keymap.  This writes the
// result into |buf| and returns the number of bytes that were written.
uint32_t hid_key_to_vt100_code(uint8_t keycode, int modifiers,
                               keychar_t* keymap, char* buf, size_t buf_size);
