#include "waycon.h"
#include <stdio.h>
#include <string.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>


// The following was taken from wtype and modified for the uess of this library
static void handle_wl_event(
  void *data,
  struct wl_registry *registry,
  uint32_t name,
  const char *interface,
  uint32_t version
) {
  waymoctx *wctx = data;
  if (!strcmp(interface, wl_seat_interface.name)) {
    wctx->seat = wl_registry_bind(
      registry,
      name,
      &wl_seat_interface,
      version <= 7 ? version : 7
    );
  } else if (!strcmp(interface, zwp_virtual_keyboard_manager_v1_interface.name)) {
    wctx->kman = wl_registry_bind(
      registry,
      name,
      &zwp_virtual_keyboard_manager_v1_interface,
      1
    );
  } else if (!strcmp(interface, zwlr_virtual_pointer_manager_v1_interface.name)) {
    wctx->pman = wl_registry_bind(
      registry,
      name,
      &zwlr_virtual_pointer_manager_v1_interface,
      2
    );
  }
}

static const struct wl_registry_listener registry_listener = {
  .global = handle_wl_event,
};

bool waymoctx_connect(waymoctx *ctx) {
  if (ctx == NULL) return false;

  ctx->display = wl_display_connect(NULL);
  if (ctx->display == NULL) {
    fprintf(stderr, "Wayland connection failed\n");
    return false;
  }
  ctx->registry = wl_display_get_registry(ctx->display);
  wl_registry_add_listener(ctx->registry, &registry_listener, ctx);
  wl_display_dispatch(ctx->display);
  wl_display_roundtrip(ctx->display);

  if (ctx->kman == NULL) {
    fprintf(stderr, "Compositor does not support the virtual keyboard protocol\n");
    return false;
  }
  if (ctx->seat == NULL) {
    fprintf(stderr, "No seat found\n");
    return false;
  }
  return true;
}

void waymoctx_destroy_connect(waymoctx *ctx) {
  wl_registry_destroy(ctx->registry);
  wl_display_disconnect(ctx->display);
}
