#include "events/commands.h"
#include "utils.h"
#include "wayland/waycon.h"
#include <stdlib.h>
#include <string.h>

command *create_quit_cmd() {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_QUIT;
  return cmd;
}

command *_create_mouse_move_cmd(int x, int y, bool relative) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_MOUSE_MOVE;
  cmd->param = (command_param){.pos = {.x = x, .y = y, .relative = relative}};
  return cmd;
}

command *_create_mouse_click_cmd(MBTNS button, unsigned int clicks,
                                 uint32_t click_ms) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_MOUSE_CLICK;
  cmd->param = (command_param){.mouse_click = {.button = button,
                                               .clicks = clicks,
                                               .click_ms = click_ms}};
  return cmd;
}

command *_create_mouse_button_cmd(MBTNS button, bool down) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_MOUSE_BTN;
  cmd->param = (command_param){.mouse_btn = {.button = button, .down = down}};
  return cmd;
}

command *_create_keyboard_key_cmd_b(char key, bool down) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_KEYBOARD_KEY;
  cmd->param = (command_param){
      .keyboard_key = {
          .key = key, .active_opt = DOWN, .keyboard_key_mod = {.down = down}}};
  return cmd;
}

command *_create_keyboard_key_cmd_uintt(char key, uint32_t hold_ms) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_KEYBOARD_KEY;
  cmd->param = (command_param){
      .keyboard_key = {.key = key,
                       .active_opt = HOLD,
                       .keyboard_key_mod = {.hold_ms = hold_ms}}};
  return cmd;
}

command *_create_keyboard_type_cmd(const char *text) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  char *txt = strdup(text);
  if (!txt) {
    free(cmd);
    return NULL;
  }

  cmd->type = CMD_KEYBOARD_TYPE;
  cmd->param = (command_param){.kbd = {.txt = txt}};
  return cmd;
}

void free_command(command *cmd) {
  if (!cmd)
    return;

  switch (cmd->type) {
  case CMD_KEYBOARD_TYPE:
    free(cmd->param.kbd.txt);
    cmd->param.kbd.txt = NULL;
    break;
  default:
    break;
  }
  free(cmd);
  cmd = NULL;
}

void _send_command(waymo_event_loop *loop, command *cmd, int fd) {
  if (unlikely(!loop || !cmd))
    return;

  cmd->done_fd = fd;

  if (!add_queue(loop->queue, cmd)) {
    // Queue is full or shutting down
    free_command(cmd);
  }
}

void execute_command(waymo_event_loop *loop, waymoctx *ctx, command *cmd) {
  if (!ctx || !cmd)
    return;

  switch (cmd->type) {
  case CMD_MOUSE_MOVE:
    if (!ctx->ptr)
      break;
    emouse_move(ctx, &cmd->param);
    signal_done(cmd->done_fd, loop->action_cooldown_ms);
    break;
  case CMD_MOUSE_CLICK:
    if (!ctx->ptr)
      break;
    emouse_click(loop, ctx, &cmd->param, cmd->done_fd);
    break;
  case CMD_MOUSE_BTN:
    if (!ctx->ptr)
      break;
    emouse_btn(ctx, &cmd->param);
    signal_done(cmd->done_fd, loop->action_cooldown_ms);
    break;
  case CMD_KEYBOARD_TYPE:
    if (!ctx->kbd)
      break;
    ekbd_type(loop, ctx, &cmd->param, cmd->done_fd);
    break;
  case CMD_KEYBOARD_KEY:
    if (!ctx->kbd)
      break;
    ekbd_key(loop, ctx, &cmd->param, cmd->done_fd);
    break;
  default:
    break;
  }

  int ret;
  while ((ret = wl_display_flush(ctx->display)) > 0)
    ;
  if (ret < 0)
    return;
}
