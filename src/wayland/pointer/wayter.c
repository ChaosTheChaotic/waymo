#include "waycon.h"
#include "wvp.h"

bool waymoctx_pointer(waymoctx *ctx) {
  if (!ctx->pman || !ctx->seat)
    return false;
  ctx->ptr = zwlr_virtual_pointer_manager_v1_create_virtual_pointer(ctx->pman,
                                                                    ctx->seat);
  return true;
}

void waymoctx_destroy_pointer(waymoctx *ctx) {
  if (!ctx)
    return;
  if (ctx->ptr) {
    zwlr_virtual_pointer_v1_destroy(ctx->ptr);
    ctx->ptr = NULL;
  }
  if (ctx->pman) {
    zwlr_virtual_pointer_manager_v1_destroy(ctx->pman);
    ctx->pman = NULL;
  }
}
