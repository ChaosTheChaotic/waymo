#include "events/event_loop.h"
#include "events/pendings.h"
#include "utils.h"
#include "wayland/waycon.h"
#include <stdlib.h>
#include <unistd.h>

bool waymoctx_pointer(waymoctx *ctx) {
  if (unlikely(!ctx->pman || !ctx->seat))
    return false;
  ctx->ptr = zwlr_virtual_pointer_manager_v1_create_virtual_pointer(ctx->pman,
                                                                    ctx->seat);
  return true;
}

void waymoctx_destroy_pointer(waymoctx *ctx) {
  if (unlikely(!ctx))
    return;
  if (ctx->ptr) {
    zwlr_virtual_pointer_v1_destroy(ctx->ptr);
    ctx->ptr = NULL;
  }
}

void emouse_move(waymoctx *ctx, command_param *param) {
  if (unlikely(!ctx || !ctx->ptr || !param))
    return;

  if (param->pos.relative) {
    zwlr_virtual_pointer_v1_motion(ctx->ptr, timestamp(),
                                   wl_fixed_from_int(param->pos.x),
                                   wl_fixed_from_int(param->pos.y));
  } else {
    if (param->pos.x < 0 || param->pos.y < 0)
      return;
    zwlr_virtual_pointer_v1_motion_absolute(
        ctx->ptr, timestamp(), (uint32_t)param->pos.x, (uint32_t)param->pos.y,
        ctx->screen_width, ctx->screen_height);
  }
  zwlr_virtual_pointer_v1_frame(ctx->ptr);
  wl_display_flush(ctx->display);
}

void emouse_btn(waymoctx *ctx, command_param *param) {
  if (unlikely(!ctx || !ctx->ptr || !param))
    return;

  uint32_t button = mbtnstoliec(param->mouse_btn.button);
  if (button == 0)
    return;

  uint32_t state = param->mouse_btn.down ? WL_POINTER_BUTTON_STATE_PRESSED
                                         : WL_POINTER_BUTTON_STATE_RELEASED;

  zwlr_virtual_pointer_v1_button(ctx->ptr, timestamp(), button, state);
  zwlr_virtual_pointer_v1_frame(ctx->ptr);
  wl_display_flush(ctx->display);
}

void emouse_click(waymo_event_loop *loop, waymoctx *ctx, command_param *param,
                  int fd) {
  if (unlikely(!ctx || !ctx->ptr || !param))
    return;

  uint32_t button = mbtnstoliec(param->mouse_click.button);
  if (button == 0)
    return;

  zwlr_virtual_pointer_v1_button(ctx->ptr, timestamp(), button,
                                 WL_POINTER_BUTTON_STATE_PRESSED);
  zwlr_virtual_pointer_v1_frame(ctx->ptr);
  wl_display_flush(ctx->display);

  // Schedule the release and other clicks
  struct pending_action *act = malloc(sizeof(struct pending_action));
  if (!act)
    return;
  act->done_fd = fd;
  act->expiry_ms = timestamp() + param->mouse_click.click_ms;
  act->type = ACTION_CLICK_STEP;
  act->data.click.button = button;
  act->data.click.ms = param->mouse_click.click_ms;
  act->data.click.remaining = param->mouse_click.clicks;
  act->data.click.is_down = true; // We are currently down, next step is up
  schedule_action(loop, act);
}
