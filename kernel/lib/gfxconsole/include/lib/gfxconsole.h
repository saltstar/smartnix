
#pragma once

#include <lib/gfx.h>
#include <zircon/compiler.h>
#include <zircon/types.h>

__BEGIN_CDECLS

zx_status_t gfxconsole_display_get_info(struct display_info* info);
void gfxconsole_start(gfx_surface* surface, gfx_surface* hw_surface);
void gfxconsole_bind_display(struct display_info* info, void* raw_sw_fb);
void gfxconsole_putpixel(unsigned x, unsigned y, unsigned color);
void gfxconsole_flush(void);

__END_CDECLS
