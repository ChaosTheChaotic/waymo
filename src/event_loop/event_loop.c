#include "event_loop.h"
#include "waycon.h"
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <wayland-client-core.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

command* create_quit_cmd() {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_QUIT;
  return cmd;
}

command* create_mouse_move_cmd(int x, int y, bool relative) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_MOUSE_MOVE;
  cmd->param = (command_param){
    .pos = { .x = x, .y = y, .relative = relative }
  };
  return cmd;
}

command* create_mouse_click_cmd(int button, int clicks, unsigned long long click_length) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_MOUSE_CLICK;
  cmd->param = (command_param){
    .mouse_click = { .button = button, .clicks = clicks, .click_length = click_length}
  };
  return cmd;
}

command* create_mouse_button_cmd(int button, bool down) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_MOUSE_BTN;
  cmd->param = (command_param){
    .mouse_btn = { .button = button, .down = down }
  };
  return cmd;
}

command* create_keyboard_key_cmd(int key, bool down, unsigned long long *hold_len) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_KEYBOARD_KEY;
  cmd->param = (command_param){
    .keyboard_key = { .key = key, .down = down, .hold_len = hold_len }
  };
  return cmd;
}

command* create_keyboard_type_cmd(const char *text) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  char *txt = strdup(text);

  cmd->type = CMD_KEYBOARD_TYPE;
  cmd->param = (command_param){
    .kbd = { .txt = txt }
  };
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
  case CMD_KEYBOARD_KEY:
    free(cmd->param.keyboard_key.hold_len);
    cmd->param.keyboard_key.hold_len = NULL;
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
  q->shutdown = true;

  while (q->num_commands > 0) {
    command *cmd = q->commands[q->front];
    if (cmd) {
      free_command(cmd);
    }
    q->front = (q->front + 1) % q->max_capacity;
    q->num_commands--;
  }

  pthread_mutex_unlock(&q->mutex);

  if (q->fd >= 0)
    close(q->fd);

  pthread_mutex_destroy(&q->mutex);

  free(q->commands);
  q->commands = NULL;
  free(q);
  q = NULL;
}

bool add_queue(command_queue *q, command *cmd) {
  pthread_mutex_lock(&q->mutex);
  if (q->num_commands >= q->max_capacity || q->shutdown) {
    pthread_mutex_unlock(&q->mutex);
    return false;
  }
  q->commands[q->back] = cmd;
  q->back = (q->back + 1) % q->max_capacity;
  q->num_commands++;

  // Signal the epoll loop that a command is ready
  uint64_t u = 1;
  write(q->fd, &u, sizeof(uint64_t));

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

void execute_command(waymoctx *ctx, command *cmd) {
  if (!ctx || !cmd)
    return;

  switch (cmd->type) {
  case CMD_MOUSE_MOVE:
    if (!ctx->ptr)
      break;
    break;
  case CMD_MOUSE_CLICK:
    if (!ctx->ptr)
      break;
    break;
  case CMD_MOUSE_BTN:
    if (!ctx->ptr)
      break;
    break;
  case CMD_KEYBOARD_TYPE:
    if (!ctx->kbd)
      break;
    break;
  case CMD_KEYBOARD_KEY:
    if (!ctx->kbd)
      break;
    break;
  default:
    break;
  }

  while (wl_display_flush(ctx->display) > 0)
    ;
}

void *event_loop(void *arg) {
  waymo_event_loop *loop = (waymo_event_loop *)arg;

  waymoctx *ctx = init_waymoctx(loop->layout, &loop->status);
  sem_post(&loop->ready_sem);
  if (!ctx)
    return NULL;

  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  int wayland_fd = wl_display_get_fd(ctx->display);

  struct epoll_event ev_wayland = {.events = EPOLLIN, .data.fd = wayland_fd};
  struct epoll_event ev_cmd = {.events = EPOLLIN, .data.fd = loop->queue->fd};

  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, wayland_fd, &ev_wayland);
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, loop->queue->fd, &ev_cmd);

  struct epoll_event events[2];

  while (true) {
    // Dispatch any internal Wayland events before sleeping
    while (wl_display_prepare_read(ctx->display) != 0) {
      wl_display_dispatch_pending(ctx->display);
    }
    wl_display_flush(ctx->display);

    int nfds = epoll_wait(epoll_fd, events, 2, -1); // Block until event
    if (nfds < 0 && errno != EINTR)
      break;

    bool wayland_ready = false;
    for (int i = 0; i < nfds; i++) {
      if (events[i].data.fd == wayland_fd) {
        if (events[i].events & (EPOLLERR | EPOLLHUP)) {
          // Handle compositor disconnect
          goto loop_exit;
        }
        wayland_ready = true;
      } else if (events[i].data.fd == loop->queue->fd) {
        // Clear eventfd signal
        uint64_t u;
        read(loop->queue->fd, &u, sizeof(uint64_t));

        command *cmd;
        while ((cmd = remove_queue(loop->queue))) {
          if (cmd->type == CMD_QUIT) {
            free_command(cmd);
            goto loop_exit;
          }
          execute_command(ctx, cmd);
          free_command(cmd);
        }
      }
    }

    if (wayland_ready) {
      wl_display_read_events(ctx->display);
    } else {
      wl_display_cancel_read(ctx->display);
    }
    wl_display_dispatch_pending(ctx->display);
  }

loop_exit:
  close(epoll_fd);
  destroy_waymoctx(ctx);
  return NULL;
}

waymo_event_loop *create_event_loop(struct eloop_params *params) {
  waymo_event_loop *loop = malloc(sizeof(waymo_event_loop));
  if (!loop)
    return NULL;

  loop->layout = params->layout ? strdup(params->layout) : strdup("us");
  loop->queue = create_queue(params->max_commands);
  if (!loop->queue || !loop->layout) {
    free(loop->layout);
    free(loop);
    return NULL;
  }

  sem_init(&loop->ready_sem, 0, 0);

  if (pthread_create(&loop->thread, NULL, event_loop, loop) != 0) {
    destroy_queue(loop->queue);
    free(loop->layout);
    sem_destroy(&loop->ready_sem);
    free(loop);
    return NULL;
  }
  sem_wait(&loop->ready_sem);
  sem_destroy(&loop->ready_sem);
  return loop;
}

void send_command(waymo_event_loop *loop, command *cmd) {
  if (unlikely(!loop || !cmd)) return;

  if (!add_queue(loop->queue, cmd)) {
    // Queue is full or shutting down
    free_command(cmd);
  }
}

void destroy_event_loop(waymo_event_loop *loop) {
  if (!loop)
    return;

  // Signal shutdown to the background thread
  command *qcmd = create_quit_cmd();
  if (qcmd) {
    add_queue(loop->queue, qcmd);
    // Wait for the thread to finish processing
    pthread_join(loop->thread, NULL);
  }

  if (loop->queue != NULL) {
    destroy_queue(loop->queue);
  }

  free(loop->layout);
  free(loop);
}
