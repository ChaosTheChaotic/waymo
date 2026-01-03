#ifndef WAYCON_H
#define WAYCON_H

#include "wvk.h"
#include "wvp.h"
#include <wayland-client-core.h>
#include <stdbool.h>

typedef struct {
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_seat *seat;
  struct zwp_virtual_keyboard_manager_v1 *kman;
  struct zwp_virtual_keyboard_v1 *kbd;
  struct zwlr_virtual_pointer_manager_v1 *pman;
  struct zwlr_virtual_pointer_v1 *ptr;
} waymoctx;

waymoctx* init_waymoctx();
void destroy_waymoctx(waymoctx *ctx);

bool waymoctx_connect(waymoctx *ctx);
void waymoctx_destroy_connect(waymoctx *ctx);

bool waymoctx_kbd(waymoctx *ctx, char *layout);
void waymoctx_destroy_kbd(waymoctx *ctx);

bool waymoctx_pointer(waymoctx *ctx);
void waymoctx_destroy_pointer(waymoctx *ctx);

#endif
