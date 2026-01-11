#include "wayland/waycon.h"
#include "events/event_loop.h"
#include <stdlib.h>
#include <wayland-client-core.h>

waymoctx *init_waymoctx(char *layout, _Atomic loop_status *status) {
  waymoctx *ctx = calloc(1, sizeof(waymoctx));
  if (!ctx) {
    atomic_fetch_or(status, STATUS_INIT_FAILED);
    return NULL;
  }
  if (!waymoctx_connect(ctx, status)) {
    goto err_cleanup;
  }
  if (!waymoctx_kbd(ctx, layout))
    atomic_fetch_or(status, STATUS_KBD_FAILED);
  if (!waymoctx_pointer(ctx))
    atomic_fetch_or(status, STATUS_PTR_FAILED);
  wl_display_roundtrip(ctx->display);
  atomic_fetch_or(status, STATUS_OK);
  return ctx;
err_cleanup:
  free(ctx);
  ctx = NULL;
  return NULL;
}

void destroy_waymoctx(waymoctx *ctx) {
  if (!ctx)
    return;

  if (ctx->ptr)
    waymoctx_destroy_pointer(ctx);
  if (ctx->kbd)
    waymoctx_destroy_kbd(ctx);
  waymoctx_destroy_connect(ctx);
  free(ctx);
  ctx = NULL;
  return;
}
