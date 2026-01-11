#ifndef PEVENTS_H
#define PEVENTS_H

#include <pthread.h>

typedef struct eloop_params {
  unsigned int max_commands;
  char *kbd_layout;
} eloop_params;

typedef enum {
  STATUS_OK = 0,
  STATUS_INIT_FAILED = 1 << 0, // This error is fatal
  STATUS_KBD_FAILED = 1 << 1,
  STATUS_PTR_FAILED = 1 << 2,
} loop_status;

typedef struct waymo_event_loop waymo_event_loop;
typedef struct command command;

waymo_event_loop* create_event_loop(const eloop_params *params);
void destroy_event_loop(waymo_event_loop *loop);
void send_command(waymo_event_loop *loop, command *cmd);

#endif
