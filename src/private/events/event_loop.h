#ifndef ELT_H
#define ELT_H

#include <semaphore.h>
#include <stdatomic.h>
#include "events/queue.h"

struct eloop_params {
  unsigned int max_commands;
  char *layout;
};

typedef enum {
  STATUS_OK = 0,
  STATUS_INIT_FAILED = 1 << 0, // This error is fatal
  STATUS_KBD_FAILED = 1 << 1,
  STATUS_PTR_FAILED = 1 << 2,
} loop_status;

typedef struct waymo_event_loop {
  pthread_t thread;
  command_queue *queue;
  char *layout;
  _Atomic loop_status status;
  sem_t ready_sem;
  int timer_fd;
  pthread_mutex_t pending_mutex;
  struct pending_action *pending_head;
} waymo_event_loop;

waymo_event_loop *create_event_loop(const struct eloop_params *params);
void destroy_event_loop(waymo_event_loop *loop);
void send_command(waymo_event_loop *loop, command *cmd);

#endif
