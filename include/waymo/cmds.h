#ifndef PCMDS_H
#define PCMDS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct command command;

command* create_mouse_move_cmd(int x, int y, bool relative);
command* create_mouse_click_cmd(int button, int clicks, uint32_t click_ms);
command* create_mouse_button_cmd(int button, bool down);

command* create_keyboard_key_cmd_b(char key, bool down);
command* create_keyboard_key_cmd_uintt(char key, uint32_t hold_ms);

#define create_keyboard_key_cmd(A, B)                                          \
  _Generic((B),                                                                \
      bool: create_keyboard_key_cmd_b,                                         \
      uint32_t: create_keyboard_key_cmd_uintt, )(A)

command* create_keyboard_type_cmd(const char *text);
command* create_quit_cmd();
void free_command(command *cmd);

#endif
