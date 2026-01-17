#ifndef ELT_H
#define ELT_H

#include "events/queue.h"
#include "waymo/events.h"
#include <semaphore.h>
#include <stdatomic.h>

typedef struct waymo_event_loop {
  pthread_t thread;
  command_queue *queue;
  char *kbd_layout;
  _Atomic loop_status status;
  sem_t ready_sem;
  int timer_fd;
  pthread_mutex_t pending_mutex;
  struct pending_action *pending_head;
  uint32_t action_cooldown_ms;
} waymo_event_loop;

#endif
