#ifndef WAYCON_H
#define WAYCON_H

#include "events/commands.h"
#include "events/event_loop.h"
#include "wvk.h"
#include "wvp.h"
#include <linux/input-event-codes.h>
#include <stdbool.h>
#include <stdint.h>
#include <xkbcommon/xkbcommon.h>

typedef struct waymoctx {
  struct wl_display *display;
  uint32_t screen_width;
  uint32_t screen_height;
  int32_t scale_factor;
  struct wl_output **outputs;
  size_t outputs_len;
  struct wl_registry *registry;
  struct wl_seat *seat;
  struct zwp_virtual_keyboard_manager_v1 *kman;
  struct zwp_virtual_keyboard_v1 *kbd;
  struct zwlr_virtual_pointer_manager_v1 *pman;
  struct zwlr_virtual_pointer_v1 *ptr;
  struct keymap_entry *keymap;
  size_t keymap_len;
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
void emouse_click(waymo_event_loop *loop, waymoctx *ctx, command_param *param,
                  int fd);
void emouse_btn(waymoctx *ctx, command_param *param);

void ekbd_type(waymo_event_loop *loop, waymoctx *ctx, command_param *param,
               int fd);
void ekbd_key(waymo_event_loop *loop, waymoctx *ctx, command_param *param,
              int fd);

uint32_t waymoctx_get_keycode(waymoctx *ctx, wchar_t ch);
void waymoctx_upload_keymap(waymoctx *ctx);

static inline uint32_t mbtnstoliec(MBTNS btn) {
  switch (btn) {
  case MBTN_LEFT:
    return BTN_LEFT;
  case MBTN_RIGHT:
    return BTN_RIGHT;
  case MBTN_MID:
    return BTN_MIDDLE;
  default:
    return 0;
  }
}

struct keymap_entry {
  xkb_keysym_t xkb;
  wchar_t wchr;
};

#endif
