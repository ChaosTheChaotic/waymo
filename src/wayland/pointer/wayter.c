#include "event_loop.h"
#include "waycon.h"
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
  if (unlikely(!ctx || !ctx->ptr || !param || param->pos.x < 0 ||
               param->pos.y < 0))
    return;

  if (param->pos.relative) {
    zwlr_virtual_pointer_v1_motion(ctx->ptr, timestamp(),
                                   wl_fixed_from_int(param->pos.x),
                                   wl_fixed_from_int(param->pos.y));
  } else {
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

  uint32_t state = param->mouse_btn.down ? WL_POINTER_BUTTON_STATE_PRESSED
                                         : WL_POINTER_BUTTON_STATE_RELEASED;

  zwlr_virtual_pointer_v1_button(ctx->ptr, timestamp(), button, state);
  zwlr_virtual_pointer_v1_frame(ctx->ptr);
  wl_display_flush(ctx->display);
}

void emouse_click(waymoctx *ctx, command_param *param) {
  if (unlikely(!ctx || !ctx->ptr || !param))
    return;

  uint32_t button = mbtnstoliec(param->mouse_click.button);

  for (unsigned int i = 0; i < param->mouse_click.clicks; i++) {
    zwlr_virtual_pointer_v1_button(ctx->ptr, timestamp(), button,
                                   WL_POINTER_BUTTON_STATE_PRESSED);

    if (param->mouse_click.click_length > 0) {
      usleep(param->mouse_click.click_length);
    }

    zwlr_virtual_pointer_v1_button(ctx->ptr, timestamp(), button,
                                   WL_POINTER_BUTTON_STATE_RELEASED);
  }
}
