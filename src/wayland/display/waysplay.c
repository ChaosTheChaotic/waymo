#include "wayland/waycon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

static void handle_output_mode(void *data, struct wl_output *wl_output,
                               uint32_t flags, int32_t width, int32_t height,
                               int32_t refresh) {
  waymoctx *wctx = data;
  if (flags & WL_OUTPUT_MODE_CURRENT) {
    wctx->screen_width = (uint32_t)width;
    wctx->screen_height = (uint32_t)height;
  }
}

// Incase this is needed later
static void handle_output_scale(void *data, struct wl_output *wl_output,
                                int32_t factor) {
  waymoctx *wctx = data;
  wctx->scale_factor = factor;
}

// Make the library happy
static void handle_output_geometry(void *data, struct wl_output *wl_output,
                                   int32_t x, int32_t y, int32_t physical_width,
                                   int32_t physical_height, int32_t subpixel,
                                   const char *make, const char *model,
                                   int32_t transform) {}

// Make the library happy
static void handle_output_done(void *data, struct wl_output *wl_output) {}

static const struct wl_output_listener output_listener = {
    .mode = handle_output_mode,
    .scale = handle_output_scale,
    .geometry = handle_output_geometry,
    .done = handle_output_done,
};

// The following was taken from wtype and modified for the uess of this library
static void handle_wl_event(void *data, struct wl_registry *registry,
                            uint32_t name, const char *interface,
                            uint32_t version) {
  waymoctx *wctx = data;
  if (!strcmp(interface, wl_seat_interface.name)) {
    wctx->seat = wl_registry_bind(registry, name, &wl_seat_interface,
                                  version <= 7 ? version : 7);
  } else if (!strcmp(interface,
                     zwp_virtual_keyboard_manager_v1_interface.name)) {
    wctx->kman = wl_registry_bind(
        registry, name, &zwp_virtual_keyboard_manager_v1_interface, 1);
  } else if (!strcmp(interface,
                     zwlr_virtual_pointer_manager_v1_interface.name)) {
    wctx->pman = wl_registry_bind(
        registry, name, &zwlr_virtual_pointer_manager_v1_interface, 2);
  }

  if (!strcmp(interface, wl_output_interface.name)) {
    struct wl_output *out =
        wl_registry_bind(registry, name, &wl_output_interface, 1);
    wl_output_add_listener(out, &output_listener, wctx);

    // Track the output for cleanup
    wctx->outputs = realloc(wctx->outputs, sizeof(struct wl_output *) *
                                               (wctx->outputs_len + 1));
    wctx->outputs[wctx->outputs_len] = out;
    wctx->outputs_len++;
  }
}

static const struct wl_registry_listener registry_listener = {
    .global = handle_wl_event,
};

bool waymoctx_connect(waymoctx *ctx, _Atomic loop_status *status) {
  if (ctx == NULL)
    return false;

  ctx->display = wl_display_connect(NULL);
  if (ctx->display == NULL) {
    fprintf(stderr, "Wayland connection failed\n");
    *status |= STATUS_INIT_FAILED;
    return false;
  }
  ctx->registry = wl_display_get_registry(ctx->display);
  wl_registry_add_listener(ctx->registry, &registry_listener, ctx);
  wl_display_dispatch(ctx->display);
  wl_display_roundtrip(ctx->display);

  if (ctx->kman == NULL) {
    fprintf(stderr,
            "Compositor does not support the virtual keyboard protocol\n");
    *status |= STATUS_KBD_FAILED;
  }
  if (ctx->pman == NULL) {
    fprintf(stderr,
            "Compositor does not support the virtual pointer protocol\n");
    *status |= STATUS_PTR_FAILED;
  }
  if (ctx->seat == NULL) {
    fprintf(stderr, "No seat found\n");
    waymoctx_destroy_connect(ctx);
    return false;
  }
  return true;
}

void waymoctx_destroy_connect(waymoctx *ctx) {
  if (!ctx)
    return;

  for (size_t i = 0; i < ctx->outputs_len; i++) {
    if (ctx->outputs[i]) {
      wl_output_destroy(ctx->outputs[i]);
    }
  }
  free(ctx->outputs);
  ctx->outputs = NULL;
  ctx->outputs_len = 0;

  if (ctx->seat)
    wl_seat_destroy(ctx->seat);
  if (ctx->kman)
    zwp_virtual_keyboard_manager_v1_destroy(ctx->kman);
  if (ctx->pman)
    zwlr_virtual_pointer_manager_v1_destroy(ctx->pman);

  if (ctx->registry)
    wl_registry_destroy(ctx->registry);
  if (ctx->display) {
    wl_display_disconnect(ctx->display);
    ctx->display = NULL;
  }
}
