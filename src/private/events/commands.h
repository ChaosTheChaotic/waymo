#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>
#include <stdint.h>

struct waymoctx;
struct waymo_event_loop;

typedef enum {
  CMD_MOUSE_MOVE,    // Takes x, y and if movement should be relative
  CMD_MOUSE_CLICK,   // Takes the button and num clicks
  CMD_MOUSE_BTN,     // Takes button and if down
  CMD_KEYBOARD_TYPE, // Takes key and num clicks
  CMD_KEYBOARD_KEY,  // Takes key and if down
  CMD_QUIT,
} command_type;

typedef enum {
  MBTN_LEFT,
  MBTN_RIGHT,
  MBTN_MID,
}MBTNS;

enum KMODOPT {
  DOWN,
  HOLD,
};

typedef union {
  struct {
    int x, y;
    bool relative;
  } pos;
  struct {
    MBTNS button;
    unsigned int clicks;
    uint32_t click_ms;
  } mouse_click;
  struct {
    MBTNS button;
    bool down;
  } mouse_btn;
  struct {
    char key;
    enum KMODOPT active_opt;
    union {
      bool down;
      uint32_t hold_ms;
    } keyboard_key_mod;
  } keyboard_key;
  struct {
    char *txt;
  } kbd;
} command_param;

typedef struct command {
  command_type type;
  command_param param;
} command;

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

void execute_command(struct waymo_event_loop *loop, struct waymoctx *ctx, command *cmd);

#endif
