#ifndef PCMDS_H
#define PCMDS_H

#include "waymo/btns.h"
#include "waymo/events.h"
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct command _command;

_command *_create_mouse_move_cmd(unsigned int x, unsigned int y, bool relative);
_command *_create_mouse_click_cmd(MBTNS button, unsigned int clicks,
                                  uint32_t click_ms);
_command *_create_mouse_button_cmd(MBTNS button, bool down);

_command *_create_keyboard_key_cmd_b(char key, uint32_t *interval_ms,
                                     bool down);
_command *_create_keyboard_key_cmd_uintt(char key, uint32_t *interval_ms,
                                         uint32_t hold_ms);

#define _create_keyboard_key_cmd(key, interval, mutation)                      \
  _Generic((mutation),                                                         \
      bool: _create_keyboard_key_cmd_b,                                        \
      uint32_t: _create_keyboard_key_cmd_uintt)((char)(key), (interval),       \
                                                (mutation))

_command *_create_keyboard_type_cmd(const char *text, uint32_t *interval_ms);

void _send_command(waymo_event_loop *loop, _command *cmd, int fd);

#define WAIT_COMPLETE(cmd_func, ...)                                           \
  do {                                                                         \
    int efd = eventfd(0, EFD_CLOEXEC);                                         \
    if (efd != -1) {                                                           \
      cmd_func(__VA_ARGS__, efd);                                              \
      uint64_t res;                                                            \
      while (read(efd, &res, sizeof(res)) == -1 && errno == EINTR)             \
        ;                                                                      \
      close(efd);                                                              \
    }                                                                          \
  } while (0)

#endif
