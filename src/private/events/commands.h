#ifndef COMMANDS_H
#define COMMANDS_H

#include "waymo/actions_internal.h"
#include "waymo/btns.h"
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
    uint32_t *interval_ms;
    union {
      bool down;
      uint32_t hold_ms;
    } keyboard_key_mod;
  } keyboard_key;
  struct {
    char *txt;
    uint32_t *interval_ms;
  } kbd;
} command_param;

typedef struct command {
  command_type type;
  command_param param;
  int done_fd;
} command;

void execute_command(struct waymo_event_loop *loop, struct waymoctx *ctx,
                     command *cmd);

void free_command(command *cmd);

command *create_quit_cmd();

#endif
