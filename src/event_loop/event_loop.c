#include "event_loop.h"
#include "waycon.h"
#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

void update_timer(waymo_event_loop *loop) {
  if (!loop->pending_head)
    return;

  uint64_t now = timestamp();
  uint64_t diff = (loop->pending_head->expiry_ms > now)
                      ? (loop->pending_head->expiry_ms - now)
                      : 1;

  struct itimerspec new_val = {
      .it_value = {.tv_sec = diff / 1000, .tv_nsec = (diff % 1000) * 1000000}};
  timerfd_settime(loop->timer_fd, 0, &new_val, NULL);
}

void schedule_action(waymo_event_loop *loop, struct pending_action *action) {
  pthread_mutex_lock(&loop->pending_mutex);
  struct pending_action **curr = &loop->pending_head;
  while (*curr && (*curr)->expiry_ms < action->expiry_ms) {
    curr = &((*curr)->next);
  }
  action->next = *curr;
  *curr = action;
  update_timer(loop);
  pthread_mutex_unlock(&loop->pending_mutex);
}

void clear_pending_actions(waymo_event_loop *loop) {
  pthread_mutex_lock(&loop->pending_mutex);
  struct pending_action *curr = loop->pending_head;
  while (curr) {
    struct pending_action *next = curr->next;
    if (curr->type == ACTION_TYPE_STEP && curr->data.type_txt.txt) {
      free(curr->data.type_txt.txt);
    }
    free(curr);
    curr = next;
  }
  loop->pending_head = NULL;
  pthread_mutex_unlock(&loop->pending_mutex);
}

void handle_timer_expiry(waymo_event_loop *loop, waymoctx *ctx) {
  pthread_mutex_lock(&loop->pending_mutex);
  uint64_t now = timestamp();

  while (loop->pending_head && loop->pending_head->expiry_ms <= now) {
    struct pending_action *act = loop->pending_head;
    loop->pending_head = act->next;

    if (act->type == ACTION_KEY_RELEASE) {
      zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), act->data.key.keycode,
                                  WL_KEYBOARD_KEY_STATE_RELEASED);
      if (act->data.key.shift)
        zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), KEY_LEFTSHIFT,
                                    WL_KEYBOARD_KEY_STATE_RELEASED);
    } else if (act->type == ACTION_MOUSE_RELEASE) {
      zwlr_virtual_pointer_v1_button(ctx->ptr, timestamp(),
                                     act->data.mouse.button,
                                     WL_POINTER_BUTTON_STATE_RELEASED);
      zwlr_virtual_pointer_v1_frame(ctx->ptr);
    } else if (act->type == ACTION_CLICK_STEP) {
      uint32_t state = act->data.click.is_down
                           ? WL_POINTER_BUTTON_STATE_RELEASED
                           : WL_POINTER_BUTTON_STATE_PRESSED;
      zwlr_virtual_pointer_v1_button(ctx->ptr, timestamp(),
                                     act->data.click.button, state);
      zwlr_virtual_pointer_v1_frame(ctx->ptr);

      if (act->data.click.is_down || act->data.click.remaining > 1) {
        struct pending_action *next_step =
            malloc(sizeof(struct pending_action));
        if (next_step) {
          *next_step = *act;
          next_step->expiry_ms = now + act->data.click.ms;
          next_step->data.click.is_down = !act->data.click.is_down;
          if (!next_step->data.click.is_down)
            next_step->data.click.remaining--;
          schedule_action(loop, next_step);
        }
      }
    } else if (act->type == ACTION_TYPE_STEP) {
      char c = act->data.type_txt.txt[act->data.type_txt.index];
      if (c != '\0') {
        struct Key key = chartokey(c);
        if (key.keycode != 0) {
          if (key.shift)
            zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), KEY_LEFTSHIFT,
                                        WL_KEYBOARD_KEY_STATE_PRESSED);
          zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), key.keycode,
                                      WL_KEYBOARD_KEY_STATE_PRESSED);
          zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), key.keycode,
                                      WL_KEYBOARD_KEY_STATE_RELEASED);
          if (key.shift)
            zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), KEY_LEFTSHIFT,
                                        WL_KEYBOARD_KEY_STATE_RELEASED);
        }

        if (act->data.type_txt.txt[act->data.type_txt.index + 1] != '\0') {
          struct pending_action *next_char =
              malloc(sizeof(struct pending_action));
          if (next_char) {
            *next_char = *act;
            next_char->data.type_txt.index++;
            schedule_action(loop, next_char);

            act->data.type_txt.txt = NULL;
          }
        }
      }
      if (act->data.type_txt.txt) {
        free(act->data.type_txt.txt);
      }
    }

    wl_display_flush(ctx->display);
    free(act);
  }
  update_timer(loop);
  pthread_mutex_unlock(&loop->pending_mutex);
}

command *create_quit_cmd() {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_QUIT;
  return cmd;
}

command *create_mouse_move_cmd(int x, int y, bool relative) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_MOUSE_MOVE;
  cmd->param = (command_param){.pos = {.x = x, .y = y, .relative = relative}};
  return cmd;
}

command *create_mouse_click_cmd(int button, int clicks, uint32_t click_ms) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_MOUSE_CLICK;
  cmd->param = (command_param){.mouse_click = {.button = button,
                                               .clicks = clicks,
                                               .click_ms = click_ms}};
  return cmd;
}

command *create_mouse_button_cmd(int button, bool down) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_MOUSE_BTN;
  cmd->param = (command_param){.mouse_btn = {.button = button, .down = down}};
  return cmd;
}

command *create_keyboard_key_cmd_b(char key, bool down) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_KEYBOARD_KEY;
  cmd->param = (command_param){
      .keyboard_key = {
          .key = key, .active_opt = DOWN, .keyboard_key_mod = {.down = down}}};
  return cmd;
}

command *create_keyboard_key_cmd_uintt(char key, uint32_t hold_ms) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  cmd->type = CMD_KEYBOARD_KEY;
  cmd->param = (command_param){
      .keyboard_key = {.key = key,
                       .active_opt = HOLD,
                       .keyboard_key_mod = {.hold_ms = hold_ms}}};
  return cmd;
}

command *create_keyboard_type_cmd(const char *text) {
  command *cmd = malloc(sizeof(command));
  if (!cmd)
    return NULL;

  char *txt = strdup(text);

  cmd->type = CMD_KEYBOARD_TYPE;
  cmd->param = (command_param){.kbd = {.txt = txt}};
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

void execute_command(waymo_event_loop *loop, waymoctx *ctx, command *cmd) {
  if (!ctx || !cmd)
    return;

  switch (cmd->type) {
  case CMD_MOUSE_MOVE:
    if (!ctx->ptr)
      break;
    emouse_move(ctx, &cmd->param);
    break;
  case CMD_MOUSE_CLICK:
    if (!ctx->ptr)
      break;
    emouse_click(loop, ctx, &cmd->param);
    break;
  case CMD_MOUSE_BTN:
    if (!ctx->ptr)
      break;
    emouse_btn(ctx, &cmd->param);
    break;
  case CMD_KEYBOARD_TYPE:
    if (!ctx->kbd)
      break;
    ekbd_type(loop, ctx, &cmd->param);
    break;
  case CMD_KEYBOARD_KEY:
    if (!ctx->kbd)
      break;
    ekbd_key(loop, ctx, &cmd->param);
    break;
  default:
    break;
  }

  int ret;
  while ((ret = wl_display_flush(ctx->display)) > 0)
    ;
  if (ret < 0)
    return;
}

void *event_loop(void *arg) {
  waymo_event_loop *loop = (waymo_event_loop *)arg;

  waymoctx *ctx = init_waymoctx(loop->layout, &loop->status);
  sem_post(&loop->ready_sem);
  if (!ctx)
    return NULL;

  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  int wayland_fd = wl_display_get_fd(ctx->display);

  loop->timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (loop->timer_fd == -1)
    goto loop_exit;

  struct epoll_event ev_wayland = {.events = EPOLLIN, .data.fd = wayland_fd};
  struct epoll_event ev_cmd = {.events = EPOLLIN, .data.fd = loop->queue->fd};
  struct epoll_event ev_timer = {.events = EPOLLIN, .data.fd = loop->timer_fd};

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, wayland_fd, &ev_wayland) == -1)
    goto loop_exit;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, loop->queue->fd, &ev_cmd) == -1)
    goto loop_exit;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, loop->timer_fd, &ev_timer) == -1)
    goto loop_exit;

#define EVENTS_NUM 3

  struct epoll_event events[EVENTS_NUM];

  while (true) {
    // Dispatch any internal Wayland events before sleeping
    while (wl_display_prepare_read(ctx->display) != 0) {
      wl_display_dispatch_pending(ctx->display);
    }
    wl_display_flush(ctx->display);

    int nfds =
        epoll_wait(epoll_fd, events, EVENTS_NUM, -1); // Block until event
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
        if (read(loop->queue->fd, &u, sizeof(uint64_t)) == -1)
          goto loop_exit;

        command *cmd;
        while ((cmd = remove_queue(loop->queue))) {
          if (cmd->type == CMD_QUIT) {
            free_command(cmd);
            goto loop_exit;
          }
          execute_command(loop, ctx, cmd);
          free_command(cmd);
        }
      } else if (events[i].data.fd == loop->timer_fd) {
        uint64_t expirations;
        if (read(loop->timer_fd, &expirations, sizeof(uint64_t)) == -1) {
          goto loop_exit;
        }
        handle_timer_expiry(loop, ctx);
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
  if (epoll_fd >= 0)
    close(epoll_fd);
  if (loop->timer_fd >= 0)
    close(loop->timer_fd);
  clear_pending_actions(loop);
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

  loop->timer_fd = -1;
  loop->pending_head = NULL;
  atomic_init(&loop->status, STATUS_OK);

  if (pthread_mutex_init(&loop->pending_mutex, NULL) != 0) {
    destroy_queue(loop->queue);
    free(loop->layout);
    sem_destroy(&loop->ready_sem);
    free(loop);
    return NULL;
  }

  sem_init(&loop->ready_sem, 0, 0);

  if (pthread_create(&loop->thread, NULL, event_loop, loop) != 0) {
    pthread_mutex_destroy(&loop->pending_mutex);
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
  if (unlikely(!loop || !cmd))
    return;

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
  if (qcmd && !add_queue(loop->queue, qcmd)) {
    free_command(qcmd);
  }
  // Wait for the thread to finish processing
  pthread_join(loop->thread, NULL);

  if (loop->queue != NULL)
    destroy_queue(loop->queue);

  if (loop->timer_fd >= 0)
    close(loop->timer_fd);

  pthread_mutex_destroy(&loop->pending_mutex);

  free(loop->layout);
  free(loop);
}
