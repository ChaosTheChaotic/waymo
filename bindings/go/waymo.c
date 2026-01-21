#include "waymo.h"
#include "waymo/actions.h"

waymo_event_loop *waymo_create_event_loop(const eloop_params *params) {
  return create_event_loop(params);
}

void waymo_destroy_event_loop(waymo_event_loop *loop) {
  destroy_event_loop(loop);
}

loop_status waymo_get_event_loop_status(waymo_event_loop *loop) {
  return get_event_loop_status(loop);
}

void waymo_move_mouse(waymo_event_loop *loop, unsigned int x, unsigned int y,
                      int relative) {
  move_mouse(loop, x, y, relative);
}

void waymo_click_mouse(waymo_event_loop *loop, MBTNS btn, unsigned int clicks,
                       uint32_t hold_ms) {
  click_mouse(loop, btn, clicks, hold_ms);
}

void waymo_press_mouse(waymo_event_loop *loop, MBTNS btn, int down) {
  press_mouse(loop, btn, down);
}

void waymo_press_key(waymo_event_loop *loop, char key, uint32_t *interval_ms,
                     int down) {
  press_key(loop, key, interval_ms, down);
}

void waymo_hold_key(waymo_event_loop *loop, char key, uint32_t *interval_ms,
                    uint32_t hold_ms) {
  hold_key(loop, key, interval_ms, hold_ms);
}

void waymo_type(waymo_event_loop *loop, const char *text,
                uint32_t *interval_ms) {
  type(loop, text, interval_ms);
}
