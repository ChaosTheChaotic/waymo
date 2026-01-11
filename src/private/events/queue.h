#ifndef QUEUE_H
#define QUEUE_H

#include "events/commands.h"
#include <pthread.h>
#include <stdatomic.h>

typedef struct {
  command **commands;
  unsigned int num_commands;
  unsigned int front;
  unsigned int back;
  unsigned int max_capacity;
  pthread_mutex_t mutex;
  int fd;
  atomic_bool shutdown;
} command_queue;

command_queue* create_queue(unsigned int max_commands);
void destroy_queue(command_queue *q);

bool add_queue(command_queue *q, command *cmd);
command* remove_queue(command_queue *q);


#endif
