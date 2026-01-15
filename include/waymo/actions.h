#ifndef ACTIONS_H
#define ACTIONS_H

#include "waymo/actions_internal.h"
#include <unistd.h>
#include <sys/eventfd.h>

static inline void move_mouse(waymo_event_loop *loop, int x, int y, bool relative) {
  WAIT_COMPLETE(_send_command, loop, _create_mouse_move_cmd(x, y, relative));
}

static inline void click_mouse(waymo_event_loop *loop, MBTNS btn, unsigned int clicks, uint32_t hold_ms) {
  WAIT_COMPLETE(_send_command, loop, _create_mouse_click_cmd(btn, clicks, hold_ms));
}

static inline void press_mouse(waymo_event_loop *loop, MBTNS btn, bool down) {
  WAIT_COMPLETE(_send_command, loop, _create_mouse_button_cmd(btn, down));
}

static inline void press_key(waymo_event_loop *loop, char key, uint32_t *interval_ms, bool down) {
  _send_command(loop, _create_keyboard_key_cmd(key, interval_ms, down), -1);
}

static inline void hold_key(waymo_event_loop *loop, char key, uint32_t *interval_ms, uint32_t hold_ms) {
  _send_command(loop, _create_keyboard_key_cmd(key, interval_ms, hold_ms), -1);
}

static inline void type(waymo_event_loop *loop, const char *text, uint32_t *interval_ms) {
  WAIT_COMPLETE(_send_command, loop, _create_keyboard_type_cmd(text, interval_ms));
}

#endif
