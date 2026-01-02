#ifndef ELT_H
#define ELT_H

#include <stdbool.h>
#include <pthread.h>

typedef enum {
  CMD_MOUSE_MOVE, // Takes x, y
  CMD_MOUSE_CLICK, // Takes the button and num clicks
  CMD_KEYBOARD_TYPE, // Takes key and num clicks
  CMD_QUIT,
} command_type;

typedef union {
  struct { int x, y; } pos;
  struct { int button; int clicks; } mouse;
  struct { char *txt; } kbd;
} command_param;

typedef struct {
  command_type type;
  command_param param;
} command;

typedef struct {
  command *commands;
  unsigned int num_commands;
  command *front;
  command *back;
  unsigned int max_capacity;
} command_queue;

typedef struct {
  pthread_t thread;
  command_queue *queue;
} waymo_event_loop;

waymo_event_loop* create_event_loop(unsigned int max_commands);
void destroy_event_loop(waymo_event_loop* loop);
void send_command(waymo_event_loop *loop, command *cmd);

command* create_mouse_move_cmd(int x, int y);
command* create_mouse_click_cmd(int button, int clicks);
command* create_keyboard_type_cmd(const char* text);
command* create_quit_cmd();
void free_command(command* cmd);

#endif
