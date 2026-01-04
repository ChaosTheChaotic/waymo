#include "event_loop.h"
#include "waycon.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <wayland-client-core.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

command *create_quit_cmd() {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_QUIT;
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

command_queue *create_queue(unsigned int max_commands) {
  command_queue *q = malloc(sizeof(command_queue));
  if (!q)
    return NULL;

  q->commands = malloc(sizeof(command *) * max_commands);
  if (!q->commands) {
    free(q);
    q = NULL;
    return NULL;
  }

  for (unsigned int i = 0; i < max_commands; i++) {
    q->commands[i] = NULL;
  }

  q->num_commands = 0;
  q->max_capacity = max_commands;
  q->front = 0;
  q->back = 0;
  q->shutdown = false;

  pthread_mutex_init(&q->mutex, NULL);
  pthread_cond_init(&q->cond, NULL);

  return q;
}

void destroy_queue(command_queue *q) {
  if (!q)
    return;

  pthread_mutex_lock(&q->mutex);
  q->shutdown = true;
  pthread_cond_broadcast(&q->cond);

  while (q->num_commands > 0) {
    command *cmd = q->commands[q->front];
    if (cmd) {
      free_command(cmd);
    }
    q->front = (q->front + 1) % q->max_capacity;
    q->num_commands--;
  }

  pthread_mutex_unlock(&q->mutex);

  pthread_mutex_destroy(&q->mutex);
  pthread_cond_destroy(&q->cond);

  free(q->commands);
  q->commands = NULL;
  free(q);
  q = NULL;
}

bool add_queue(command_queue *q, command *cmd) {
  if (!q || !cmd)
    return false;

  pthread_mutex_lock(&q->mutex);

  while (q->num_commands >= q->max_capacity && !q->shutdown) {
    pthread_cond_wait(&q->cond, &q->mutex);
  }

  if (q->shutdown) {
    pthread_mutex_unlock(&q->mutex);
    free_command(cmd);
    return false;
  }

  // Store the command pointer
  q->commands[q->back] = cmd;
  q->back = (q->back + 1) % q->max_capacity;
  q->num_commands++;

  pthread_cond_signal(&q->cond);
  pthread_mutex_unlock(&q->mutex);

  return true;
}

command *remove_queue(command_queue *q) {
  if (!q)
    return NULL;

  pthread_mutex_lock(&q->mutex);

  while (q->num_commands == 0 && !q->shutdown) {
    pthread_cond_wait(&q->cond, &q->mutex);
  }

  if (q->shutdown && q->num_commands == 0) {
    pthread_mutex_unlock(&q->mutex);
    return NULL;
  }

  // Get the command pointer
  command *cmd = q->commands[q->front];
  q->commands[q->front] = NULL; // Clear the slot
  q->front = (q->front + 1) % q->max_capacity;
  q->num_commands--;

  pthread_cond_signal(&q->cond);
  pthread_mutex_unlock(&q->mutex);

  return cmd;
}

void execute_command(waymoctx *ctx, command *cmd) {
  if (!ctx || !cmd)
    return;

  switch (cmd->type) {
  case CMD_MOUSE_MOVE:
    break;
  case CMD_MOUSE_CLICK:
    break;
  case CMD_MOUSE_BTN:
    break;
  case CMD_KEYBOARD_TYPE:
    break;
  case CMD_KEYBOARD_KEY:
    break;
  default:
    break;
  }

  wl_display_flush(ctx->display);
}

struct eloop_tuple {
  waymo_event_loop *loop;
  struct eloop_params *params;
};

void* event_loop(void *arg) {
  waymo_event_loop *loop = (waymo_event_loop *)arg;
  
  if (!loop || !loop->wayctx) {
    return NULL;
  }

  while (true) {
    command *cmd = remove_queue(loop->queue);
    if (!cmd)
      break;

    if (cmd->type == CMD_QUIT) {
      free_command(cmd);
      break;
    }

    execute_command(loop->wayctx, cmd);
    free_command(cmd);

    wl_display_prepare_read(loop->wayctx->display);
    wl_display_read_events(loop->wayctx->display);
    wl_display_dispatch_pending(loop->wayctx->display);
  }
  return NULL;
}

waymo_event_loop *create_event_loop(struct eloop_params *params) {
  waymo_event_loop *loop = malloc(sizeof(waymo_event_loop));
  if (!loop)
    return NULL;

  loop->queue = create_queue(params->max_commands);
  if (!loop->queue) {
    free(loop);
    return NULL;
  }

  // Initialize the context in the main thread to handle errors safely
  loop->wayctx = init_waymoctx(params->layout);
  if (!loop->wayctx) {
    destroy_queue(loop->queue);
    free(loop);
    return NULL;
  }

  if (pthread_create(&loop->thread, NULL, event_loop, loop) != 0) {
    destroy_waymoctx(loop->wayctx);
    destroy_queue(loop->queue);
    free(loop);
    return NULL;
  }
  return loop;
}

void destroy_event_loop(waymo_event_loop *loop) {
  if (!loop)
    return;

  // Signal shutdown to the background thread 
  if (loop->queue != NULL) {
    pthread_mutex_lock(&loop->queue->mutex);
    loop->queue->shutdown = true;
    pthread_cond_broadcast(&loop->queue->cond);
    pthread_mutex_unlock(&loop->queue->mutex);
  }

  // Wait for the thread to finish processing
  pthread_join(loop->thread, NULL);

  if (loop->wayctx != NULL) {
    destroy_waymoctx(loop->wayctx);
  }

  if (loop->queue != NULL) {
    destroy_queue(loop->queue);
  }

  free(loop);
}
