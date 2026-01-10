#define _GNU_SOURCE

#include "waycon.h"
#include "wvk.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

struct fd_len {
  int fd;
  size_t len_content;
};

char *get_keymap_string(char *layout) {
  layout = layout ? layout : "us";
  struct xkb_context *xctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_rule_names names = {.layout = layout};
  struct xkb_keymap *keymap =
      xkb_keymap_new_from_names(xctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!keymap) {
    xkb_context_unref(xctx);
    return NULL;
  }
  char *keymap_str =
      xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
  xkb_keymap_unref(keymap);
  xkb_context_unref(xctx);
  return keymap_str;
}

struct fd_len keymap_fd(char *layout) {
  char *keymap = get_keymap_string(layout);
  if (!keymap)
    return (struct fd_len){.fd = -1, .len_content = 0};

  size_t keymap_len = strlen(keymap) + 1;

  int fd = memfd_create("waymo_keymap", MFD_CLOEXEC);
  if (fd == -1) {
    free(keymap);
    return (struct fd_len){.fd = -1, .len_content = 0};
  }

  if (ftruncate(fd, keymap_len) < 0 || write(fd, keymap, keymap_len) == -1) {
    close(fd);
    free(keymap);
    return (struct fd_len){.fd = -1, .len_content = 0};
  }

  lseek(fd, 0, SEEK_SET);
  free(keymap);
  return (struct fd_len){.fd = fd, .len_content = keymap_len};
}

bool waymoctx_kbd(waymoctx *ctx, char *layout) {
  if (!ctx->kman || !ctx->seat)
    return false;
  ctx->kbd = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(ctx->kman,
                                                                     ctx->seat);
  struct fd_len fd = keymap_fd(layout);
  if (fd.fd == -1) {
    fprintf(stderr, "Failed to save keymap to fd");
    zwp_virtual_keyboard_v1_destroy(ctx->kbd);
    return false;
  };
  zwp_virtual_keyboard_v1_keymap(ctx->kbd, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
                                 fd.fd, fd.len_content);
  close(fd.fd);
  return true;
}

void waymoctx_destroy_kbd(waymoctx *ctx) {
  if (ctx->kbd)
    zwp_virtual_keyboard_v1_destroy(ctx->kbd);
  ctx->kbd = NULL;
}

void ekbd_key(waymoctx *ctx, command_param *param) {
  if (unlikely(!ctx || !param || !ctx->kbd))
    return;

  struct Key key = chartokey(param->keyboard_key.key);
  if (param->keyboard_key.active_opt == DOWN) {
    enum wl_keyboard_key_state down = param->keyboard_key.keyboard_key_mod.down
                                          ? WL_KEYBOARD_KEY_STATE_PRESSED
                                          : WL_KEYBOARD_KEY_STATE_RELEASED;
    if (key.shift) {
      zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), KEY_LEFTSHIFT, down);
    }
    zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), key.keycode, down);
  } else {
    if (key.shift) {
      zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), KEY_LEFTSHIFT,
                                  WL_KEYBOARD_KEY_STATE_PRESSED);
    }
    zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), key.keycode,
                                WL_KEYBOARD_KEY_STATE_PRESSED);
    wl_display_flush(ctx->display);
    usleep(param->keyboard_key.keyboard_key_mod.hold_len);
    zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), key.keycode,
                                WL_KEYBOARD_KEY_STATE_RELEASED);
    if (key.shift) {
      zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), KEY_LEFTSHIFT,
                                  WL_KEYBOARD_KEY_STATE_RELEASED);
    }
  }
  wl_display_flush(ctx->display);
}

void ekbd_type(waymoctx *ctx, command_param *param) {
  if (unlikely(!ctx || !param || !ctx->kbd))
    return;

  unsigned long txt_len = strlen(param->kbd.txt);
  for (unsigned long i = 0; i < txt_len; i++) {
    struct Key key = chartokey(param->kbd.txt[i]);
    if (key.shift) {
      zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), KEY_LEFTSHIFT,
                                  WL_KEYBOARD_KEY_STATE_PRESSED);
    }
    zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), key.keycode,
                                WL_KEYBOARD_KEY_STATE_PRESSED);
    zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), key.keycode,
                                WL_KEYBOARD_KEY_STATE_RELEASED);
    if (key.shift) {
      zwp_virtual_keyboard_v1_key(ctx->kbd, timestamp(), KEY_LEFTSHIFT,
                                  WL_KEYBOARD_KEY_STATE_RELEASED);
    }
  }
  wl_display_flush(ctx->display);
}
