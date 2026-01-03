#include "waycon.h"
#include "wvp.h"

bool waymoctx_pointer(waymoctx *ctx) {
  ctx->ptr = zwlr_virtual_pointer_manager_v1_create_virtual_pointer(ctx->pman, ctx->seat);
  return true;
}

void waymoctx_destroy_pointer(waymoctx *ctx) {
  zwlr_virtual_pointer_v1_destroy(ctx->ptr);
  zwlr_virtual_pointer_manager_v1_destroy(ctx->pman);
}
