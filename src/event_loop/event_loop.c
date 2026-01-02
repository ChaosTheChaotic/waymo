#include "event_loop.h"
#include <pthread.h>
#include <stdlib.h>

void free_command(command *cmd) {
  if (!cmd) return;

  switch (cmd->type) {
    case CMD_KEYBOARD_TYPE:
      free(cmd->param.kbd.txt);
      break;
    default:
      break;
  }
  free(cmd);
}

command_queue* create_qeue(unsigned int max_commands) {
  command_queue *q = malloc(sizeof(command_queue));
  if(!q) return NULL;

  q->max_capacity = max_commands;
  q->commands = malloc(sizeof(command) * max_commands);
  if (!q->commands) return NULL;

  q->num_commands = 0;
  q->front = q->back = NULL;

  return q;
}

void destroy_queue(command_queue *q) {
  for (unsigned int i = 0; i < q->num_commands; i++) {
    free_command(&q->commands[i]);
  }
  free(q->commands);
  free(q);
}

void add_queue(command_queue *q, command *cmd) {
  if (!q || !cmd || q->num_commands >= q->max_capacity) return;

  if (q->num_commands == 0) {
    q->front = q->commands;
    q->back = q->commands;
  } else {
    if (q->back == &q->commands[q->max_capacity - 1]) {
      q->back = q->commands; // Wrap to start of array if empty spots exist at the start
    } else {
      q->back++;
    }
  }

  *q->back = *cmd;
  q->num_commands++;
}

command* remove_queue(command_queue *q) {
  if (!q || q->num_commands == 0) {
    return NULL;
  }

  command *cmd = q->front;

  if (q->num_commands == 1) {
    q->front = q->back = NULL;
  } else {
    // Move front forward, wrapping if it hits the end of the array
    if (q->front == &q->commands[q->max_capacity - 1]) {
      q->front = q->commands;
    } else {
      q->front++;
    }
  }

  q->num_commands--;

  return cmd;
}

void* event_loop(void *arg) {
  waymo_event_loop* loop = (waymo_event_loop*)arg;
  // TODO: Get and execute commands
  return NULL;
}

waymo_event_loop* create_event_loop(unsigned int max_commands) {
  waymo_event_loop *loop = malloc(sizeof(waymo_event_loop));
  if (!loop) return NULL;

  loop->queue = create_qeue(max_commands);
  if (!loop->queue) {
    free(loop);
    return NULL;
  }

  if (pthread_create(&loop->thread, NULL, event_loop, loop) != 0) {
    destroy_queue(loop->queue);
    free(loop);
    return NULL;
  }
  return loop;
}

void destroy_event_loop(waymo_event_loop *loop) {
  if (!loop) return;

  // TODO: Check if thread running and stop it if so
}
