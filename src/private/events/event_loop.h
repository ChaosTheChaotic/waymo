#ifndef ELT_H
#define ELT_H

#include <semaphore.h>
#include <stdatomic.h>
#include "events/queue.h"
#include "waymo/events.h"

typedef struct waymo_event_loop {
  pthread_t thread;
  command_queue *queue;
  char *kbd_layout;
  _Atomic loop_status status;
  sem_t ready_sem;
  int timer_fd;
  pthread_mutex_t pending_mutex;
  struct pending_action *pending_head;
} waymo_event_loop;

#endif
