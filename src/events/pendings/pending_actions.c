#include "events/pendings.h"
#include "utils.h"
#include <pthread.h>
#include <stdlib.h>
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

inline void schedule_action_locked(waymo_event_loop *loop,
                                   struct pending_action *action) {
  struct pending_action **curr = &loop->pending_head;
  while (*curr && (*curr)->expiry_ms < action->expiry_ms) {
    curr = &((*curr)->next);
  }
  action->next = *curr;
  *curr = action;
  update_timer(loop);
}

void schedule_action(waymo_event_loop *loop, struct pending_action *action) {
  pthread_mutex_lock(&loop->pending_mutex);
  schedule_action_locked(loop, action);
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

      wl_display_flush(ctx->display);
      signal_done(act->done_fd, loop->action_cooldown_ms);
    } else if (act->type == ACTION_MOUSE_RELEASE) {
      zwlr_virtual_pointer_v1_button(ctx->ptr, timestamp(),
                                     act->data.mouse.button,
                                     WL_POINTER_BUTTON_STATE_RELEASED);
      zwlr_virtual_pointer_v1_frame(ctx->ptr);
      wl_display_flush(ctx->display);
      signal_done(act->done_fd, loop->action_cooldown_ms);
    } else if (act->type == ACTION_CLICK_STEP) {
      uint32_t state = act->data.click.is_down
                           ? WL_POINTER_BUTTON_STATE_RELEASED
                           : WL_POINTER_BUTTON_STATE_PRESSED;
      zwlr_virtual_pointer_v1_button(ctx->ptr, timestamp(),
                                     act->data.click.button, state);
      zwlr_virtual_pointer_v1_frame(ctx->ptr);
      wl_display_flush(ctx->display);

      if (act->data.click.is_down || act->data.click.remaining > 1) {
        struct pending_action *next_step =
            malloc(sizeof(struct pending_action));
        if (next_step) {
          *next_step = *act;
          next_step->expiry_ms = now + act->data.click.ms;
          next_step->data.click.is_down = !act->data.click.is_down;
          if (!next_step->data.click.is_down)
            next_step->data.click.remaining--;
          schedule_action_locked(loop, next_step);
        }
      } else {
        signal_done(act->done_fd, loop->action_cooldown_ms);
      }
    } else if (act->type == ACTION_TYPE_STEP) {
      char c = act->data.type_txt.txt[act->data.type_txt.index];
      if (c != '\0') {
        wchar_t wc;
        mbtowc(&wc, &c, 1);

        uint32_t keycode = waymoctx_get_keycode(ctx, wc);

        zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), keycode,
                                    WL_KEYBOARD_KEY_STATE_PRESSED);
        zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), keycode,
                                    WL_KEYBOARD_KEY_STATE_RELEASED);
        wl_display_flush(ctx->display);

        if (act->data.type_txt.txt[act->data.type_txt.index + 1] != '\0') {
          struct pending_action *next_char =
              malloc(sizeof(struct pending_action));
          if (next_char) {
            *next_char = *act;
            next_char->data.type_txt.index++;
            next_char->expiry_ms = now + 15;
            schedule_action_locked(loop, next_char);
            act->data.type_txt.txt = NULL;
          }
        } else {
          signal_done(act->done_fd, loop->action_cooldown_ms);
        }
      }
    }
    wl_display_flush(ctx->display);
    free(act);
  }
  update_timer(loop);
  pthread_mutex_unlock(&loop->pending_mutex);
}
