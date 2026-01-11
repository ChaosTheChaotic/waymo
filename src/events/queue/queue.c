#include "events/queue.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <unistd.h>

command_queue *create_queue(unsigned int max_commands) {
  command_queue *q = malloc(sizeof(command_queue));
  if (!q)
    return NULL;

  q->commands = calloc(max_commands, sizeof(command *));
  q->fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

  pthread_mutex_init(&q->mutex, NULL);
  q->num_commands = 0;
  q->max_capacity = max_commands;
  q->front = 0;
  q->back = 0;
  q->shutdown = false;
  return q;
}

void destroy_queue(command_queue *q) {
  if (!q)
    return;

  pthread_mutex_lock(&q->mutex);
  atomic_store(&q->shutdown, true);

  while (q->num_commands > 0) {
    command *cmd = q->commands[q->front];
    if (cmd) {
      free_command(cmd);
    }
    q->commands[q->front] = NULL;
    q->front = (q->front + 1) % q->max_capacity;
    q->num_commands--;
  }
  pthread_mutex_unlock(&q->mutex);

  pthread_mutex_destroy(&q->mutex);

  if (q->fd >= 0) {
    close(q->fd);
    q->fd = -1;
  }

  free(q->commands);
  free(q);
}

bool add_queue(command_queue *q, command *cmd) {
  pthread_mutex_lock(&q->mutex);
  if (q->num_commands >= q->max_capacity || atomic_load(&q->shutdown)) {
    pthread_mutex_unlock(&q->mutex);
    return false;
  }

  q->commands[q->back] = cmd;
  unsigned int new_back = (q->back + 1) % q->max_capacity;

  // Signal the epoll loop
  uint64_t u = 1;
  if (write(q->fd, &u, sizeof(uint64_t)) == -1) {
    // Rollback on write failure
    q->commands[q->back] = NULL;
    pthread_mutex_unlock(&q->mutex);
    return false;
  }

  q->back = new_back;
  q->num_commands++;
  pthread_mutex_unlock(&q->mutex);
  return true;
}

command *remove_queue(command_queue *q) {
  pthread_mutex_lock(&q->mutex);
  if (q->num_commands == 0) {
    pthread_mutex_unlock(&q->mutex);
    return NULL;
  }
  command *cmd = q->commands[q->front];
  q->commands[q->front] = NULL;
  q->front = (q->front + 1) % q->max_capacity;
  q->num_commands--;
  pthread_mutex_unlock(&q->mutex);
  return cmd;
}
