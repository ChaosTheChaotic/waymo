#include "waycon.h"
#include <stdlib.h>
#include <wayland-client-core.h>

waymoctx *init_waymoctx(char *layout) {
  waymoctx *ctx = calloc(1, sizeof(waymoctx));
  if (!ctx)
    return NULL;
  if (!waymoctx_connect(ctx))
    goto err_cleanup;
  if (!waymoctx_kbd(ctx, layout))
    goto err_cleanup;
  if (!waymoctx_pointer(ctx))
    goto err_cleanup;
  wl_display_roundtrip(ctx->display);
  return ctx;
err_cleanup:
  free(ctx);
  ctx = NULL;
  return NULL;
}

void destroy_waymoctx(waymoctx *ctx) {
  if (!ctx)
    return;

  if (ctx->ptr) waymoctx_destroy_pointer(ctx);
  if (ctx->kbd) waymoctx_destroy_kbd(ctx);
  waymoctx_destroy_connect(ctx);
  free(ctx);
  ctx = NULL;
  return;
}
