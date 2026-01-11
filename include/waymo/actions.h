#ifndef ACTIONS_H
#define ACTIONS_H

#include "waymo/actions_internal.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/eventfd.h>

static inline void move_mouse(waymo_event_loop *loop, int x, int y, bool relative) {
  _send_command(loop, _create_mouse_move_cmd(x, y, relative));
}

static inline void click_mouse(waymo_event_loop *loop, MBTNS btn, unsigned int clicks, uint32_t hold_ms) {
  _send_command(loop, _create_mouse_click_cmd(btn, clicks, hold_ms));
}

static inline void press_mouse(waymo_event_loop *loop, MBTNS btn, bool down) {
  _send_command(loop, _create_mouse_button_cmd(btn, down));
}

static inline void hold_key(waymo_event_loop *loop, char key, uint32_t hold_ms) {
  _send_command(loop, _create_keyboard_key_cmd(key, hold_ms));
}

static inline void press_key(waymo_event_loop *loop, char key, bool down) {
  _send_command(loop, _create_keyboard_key_cmd(key, down));
}

static inline void type(waymo_event_loop *loop, const char *text) {
  // Create FD and do allat
  int efd = eventfd(0, EFD_CLOEXEC);

  _send_command(loop, _create_keyboard_type_cmd(text, efd));

  uint64_t res;
  ssize_t n;
  while ((n = read(efd, &res, sizeof(res))) == -1) {
      if (errno == EINTR) continue; // Interrupted by a signal, try again
      
      perror("read from eventfd failed");
      break;
    }
  close(efd);
}

#endif
