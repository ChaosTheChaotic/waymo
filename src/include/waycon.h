#ifndef WAYCON_H
#define WAYCON_H

#include "event_loop.h"
#include "wvk.h"
#include "wvp.h"
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <linux/input-event-codes.h>

typedef struct {
  struct wl_display *display;
  uint32_t screen_width;
  uint32_t screen_height;
  int32_t scale_factor;
  struct wl_registry *registry;
  struct wl_seat *seat;
  struct zwp_virtual_keyboard_manager_v1 *kman;
  struct zwp_virtual_keyboard_v1 *kbd;
  struct zwlr_virtual_pointer_manager_v1 *pman;
  struct zwlr_virtual_pointer_v1 *ptr;
} waymoctx;

waymoctx *init_waymoctx(char *layout, _Atomic loop_status *status);
void destroy_waymoctx(waymoctx *ctx);

bool waymoctx_connect(waymoctx *ctx, _Atomic loop_status *status);
void waymoctx_destroy_connect(waymoctx *ctx);

bool waymoctx_kbd(waymoctx *ctx, char *layout);
void waymoctx_destroy_kbd(waymoctx *ctx);

bool waymoctx_pointer(waymoctx *ctx);
void waymoctx_destroy_pointer(waymoctx *ctx);

void emouse_move(waymoctx *ctx, command_param *param);
void emouse_click(waymoctx *ctx, command_param *param);
void emouse_btn(waymoctx *ctx, command_param *param);

void ekbd_type(waymoctx *ctx, command_param *param);
void ekbd_key(waymoctx *ctx, command_param *param);

static inline uint32_t timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static inline uint32_t mbtnstoliec(MBTNS btn) {
  switch (btn) {
    case MBTN_LEFT:
      return BTN_LEFT;
    case MBTN_RIGHT:
      return BTN_RIGHT;
    case MBTN_MID:
      return BTN_MIDDLE;
  }
}

#endif
