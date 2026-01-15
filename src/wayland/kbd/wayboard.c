#include "events/pendings.h"
#include "utils.h"
#include "wayland/waycon.h"
#include "wvk.h"
#include <stdlib.h>
#include <string.h>

uint32_t waymoctx_get_keycode(waymoctx *ctx, wchar_t ch) {
  // Search the existing keymap
  for (uint32_t i = 0; i < ctx->keymap_len; i++) {
    if (ctx->keymap[i].wchr == ch) {
      return i;
    }
  }

  // Dynamic upload for missing characters
  ctx->keymap =
      realloc(ctx->keymap, sizeof(struct keymap_entry) * (ctx->keymap_len + 1));
  ctx->keymap[ctx->keymap_len].wchr = ch;
  ctx->keymap[ctx->keymap_len].xkb = xkb_utf32_to_keysym(ch);
  ctx->keymap_len++;

  waymoctx_upload_keymap(ctx);

  return (uint32_t)(ctx->keymap_len + 7); // Last added keycode
}

void waymoctx_upload_keymap(waymoctx *ctx) {
  char filename[] = "/tmp/waymo-XXXXXX";
  int fd = mkstemp(filename);
  if (fd < 0)
    return;
  unlink(filename);
  FILE *f = fdopen(fd, "w");

  fprintf(f, "xkb_keymap {\n");
  // Min is 8. Max must be at least 8 + keymap_len
  fprintf(f, "xkb_keycodes \"(unnamed)\" {\n  minimum = 8;\n  maximum = %zu;\n",
          ctx->keymap_len + 8);

  for (size_t i = 0; i < ctx->keymap_len; i++) {
    // Map key name <K#> to keycode # + 8
    fprintf(f, "  <K%zu> = %zu;\n", i, i + 8);
  }
  fprintf(f, "};\n");

  fprintf(f, "xkb_types \"(unnamed)\" { include \"complete\" };\n");
  fprintf(f, "xkb_compatibility \"(unnamed)\" { include \"complete\" };\n");

  fprintf(f, "xkb_symbols \"(unnamed)\" {\n");
  for (size_t i = 0; i < ctx->keymap_len; i++) {
    char name[64];
    xkb_keysym_get_name(ctx->keymap[i].xkb, name, sizeof(name));

    // Check if the name is valid or use the hex value
    if (strcmp(name, "NoSymbol") == 0) {
      fprintf(f, "  key <K%zu> {[ U%04X ]};\n", i,
              (unsigned int)ctx->keymap[i].wchr);
    } else {
      fprintf(f, "  key <K%zu> {[ %s ]};\n", i, name);
    }
  }
  fprintf(f, "};\n};\n");

  // Get size for the Wayland call
  long size = ftell(f);
  rewind(f);

  zwp_virtual_keyboard_v1_keymap(ctx->kbd, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
                                 fileno(f), (uint32_t)size);
  wl_display_roundtrip(ctx->display);
  fclose(f);
}

bool waymoctx_kbd(waymoctx *ctx, char *layout) {
  if (!ctx->kman || !ctx->seat)
    return false;

  ctx->kbd = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(ctx->kman,
                                                                     ctx->seat);

  ctx->keymap = NULL;
  ctx->keymap_len = 0;

  // Add Special Control Keys
  struct {
    wchar_t wc;
    xkb_keysym_t ks;
  } specials[] = {{L'\n', XKB_KEY_Return},
                  {L'\t', XKB_KEY_Tab},
                  {L'\b', XKB_KEY_BackSpace},
                  {L' ', XKB_KEY_space}};

  for (size_t i = 0; i < sizeof(specials) / sizeof(specials[0]); i++) {
    ctx->keymap = realloc(ctx->keymap,
                          sizeof(struct keymap_entry) * (ctx->keymap_len + 1));
    ctx->keymap[ctx->keymap_len].wchr = specials[i].wc;
    ctx->keymap[ctx->keymap_len].xkb = specials[i].ks;
    ctx->keymap_len++;
  }

  // Add standard printable ASCII (33-126)
  for (wchar_t wc = 33; wc <= 126; wc++) {
    ctx->keymap = realloc(ctx->keymap,
                          sizeof(struct keymap_entry) * (ctx->keymap_len + 1));
    ctx->keymap[ctx->keymap_len].wchr = wc;
    ctx->keymap[ctx->keymap_len].xkb = xkb_utf32_to_keysym(wc);
    ctx->keymap_len++;
  }

  waymoctx_upload_keymap(ctx);
  return true;
}

void waymoctx_destroy_kbd(waymoctx *ctx) {
  if (ctx->kbd)
    zwp_virtual_keyboard_v1_destroy(ctx->kbd);
  ctx->kbd = NULL;
  if (ctx->keymap) {
    free(ctx->keymap);
    ctx->keymap = NULL;
  }
  ctx->keymap_len = 0;
}

void ekbd_key(waymo_event_loop *loop, waymoctx *ctx, command_param *param,
              int fd) {
  if (!ctx->kbd)
    return;

  // Convert char to wchar for broader support
  wchar_t wc;
  mbtowc(&wc, &param->keyboard_key.key, 1);
  uint32_t keycode = waymoctx_get_keycode(ctx, wc);

  if (param->keyboard_key.active_opt == DOWN) {
    uint32_t state = param->keyboard_key.keyboard_key_mod.down
                         ? WL_KEYBOARD_KEY_STATE_PRESSED
                         : WL_KEYBOARD_KEY_STATE_RELEASED;

    zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), keycode, state);
    wl_display_flush(ctx->display);
    signal_done(fd, loop->action_cooldown_ms);
  } else {
    uint32_t hold_ms = param->keyboard_key.keyboard_key_mod.hold_ms;
    uint32_t repeat_interval_ms =
        param->keyboard_key.interval_ms ? *param->keyboard_key.interval_ms : 10;

    zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), keycode,
                                WL_KEYBOARD_KEY_STATE_PRESSED);
    zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), keycode,
                                WL_KEYBOARD_KEY_STATE_RELEASED);
    wl_display_flush(ctx->display);

    if (hold_ms > repeat_interval_ms) {
      struct pending_action *act = malloc(sizeof(struct pending_action));
      act->expiry_ms = timestamp() + repeat_interval_ms;
      act->type = ACTION_KEY_REPEAT;
      act->data.key_repeat.keycode = keycode;
      act->data.key_repeat.repeat_interval_ms = repeat_interval_ms;
      act->data.key_repeat.total_hold_ms = hold_ms;
      act->data.key_repeat.elapsed_ms = repeat_interval_ms;
      act->done_fd = fd;
      schedule_action(loop, act);
    } else {
      // If hold time is less than repeat interval, just signal done
      signal_done(fd, loop->action_cooldown_ms);
    }
  }
}

void ekbd_type(waymo_event_loop *loop, waymoctx *ctx, command_param *param,
               int fd) {
  if (unlikely(!ctx || !param || !ctx->kbd || !param->kbd.txt))
    return;

  struct pending_action *act = malloc(sizeof(struct pending_action));
  if (!act)
    return;

  act->type = ACTION_TYPE_STEP;
  act->expiry_ms = timestamp(); // Start immediately
  // Command might be freed after execute_command so dupe string
  act->data.type_txt.txt = strdup(param->kbd.txt);
  if (!act->data.type_txt.txt) {
    free(act);
    return;
  }
  act->data.type_txt.index = 0;
  act->data.type_txt.inteval_ms =
      param->kbd.interval_ms ? *param->kbd.interval_ms : 10;
  act->next = NULL;

  act->done_fd = fd;

  schedule_action(loop, act);
  wl_display_flush(ctx->display);
}
