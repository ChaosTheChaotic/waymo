#include "events/pendings.h"
#include "events/queue.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

void *event_loop(void *arg) {
  waymo_event_loop *loop = (waymo_event_loop *)arg;

  waymoctx *ctx = init_waymoctx(loop->kbd_layout, &loop->status);
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
    wl_display_roundtrip(ctx->display);
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

waymo_event_loop *create_event_loop(const struct eloop_params *params) {
  const char *layout = "us";
  unsigned int max_cmds = 50;
  unsigned int action_cooldown_ms = 1;

  if (params) {
    // Only override if the user provided valid values
    if (params->kbd_layout) {
      layout = params->kbd_layout;
    }
    max_cmds = params->max_commands;
    action_cooldown_ms = params->action_cooldown_ms;
  }

  waymo_event_loop *loop = malloc(sizeof(waymo_event_loop));
  if (!loop)
    return NULL;

  loop->kbd_layout = strdup(layout);
  loop->queue = create_queue(max_cmds);
  loop->action_cooldown_ms = action_cooldown_ms;
  if (!loop->queue || !loop->kbd_layout) {
    free(loop->kbd_layout);
    if (loop->queue)
      destroy_queue(loop->queue);
    free(loop);
    return NULL;
  }

  loop->timer_fd = -1;
  loop->pending_head = NULL;
  atomic_init(&loop->status, STATUS_OK);

  if (pthread_mutex_init(&loop->pending_mutex, NULL) != 0) {
    destroy_queue(loop->queue);
    free(loop->kbd_layout);
    sem_destroy(&loop->ready_sem);
    free(loop);
    return NULL;
  }

  sem_init(&loop->ready_sem, 0, 0);

  if (pthread_create(&loop->thread, NULL, event_loop, loop) != 0) {
    pthread_mutex_destroy(&loop->pending_mutex);
    destroy_queue(loop->queue);
    free(loop->kbd_layout);
    sem_destroy(&loop->ready_sem);
    free(loop);
    return NULL;
  }
  sem_wait(&loop->ready_sem);
  sem_destroy(&loop->ready_sem);
  return loop;
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

  free(loop->kbd_layout);
  free(loop);
}

loop_status get_event_loop_status(waymo_event_loop *loop) {
  return loop->status;
}
