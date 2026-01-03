#include "waycon.h"
#include "wvk.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

struct fd_len {
  int fd;
  size_t len_content;
};

const char *get_keymap_string(char *layout) {
  layout = layout ? layout : "us";
  struct xkb_context *xctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_rule_names names = { .layout = layout };
  struct xkb_keymap *keymap = 
    xkb_keymap_new_from_names(xctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
  return strdup(xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1));
}

struct fd_len keymap_fd(char *layout) {
  const char *keymap = get_keymap_string(layout);
  char fname[] = "/tmp/waymokbdfd-XXXXXXXXXX";
  int fd = mkstemp(fname);
  if (fd == -1) {
    free((void*)keymap);
    return (struct fd_len){ .fd = -1, .len_content = 0 };
  }
  unlink(fname);
  size_t keymap_len = strlen(keymap) + 1;
  write(fd, keymap, keymap_len);
  lseek(fd, 0, SEEK_SET);
  free((void*)keymap);
  return (struct fd_len){ .fd = fd, .len_content = keymap_len };
}

bool waymoctx_kbd(waymoctx *ctx, char *layout) {
  ctx->kbd = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(ctx->kman, ctx->seat);
  struct fd_len fd = keymap_fd(layout);
  if (fd.fd == -1) {
    zwp_virtual_keyboard_v1_destroy(ctx->kbd);
    return false;
  };
  zwp_virtual_keyboard_v1_keymap(ctx->kbd, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, fd.fd, fd.len_content);
  close(fd.fd);
  return true;
}

void waymoctx_destroy_kbd(waymoctx *ctx) {
  zwp_virtual_keyboard_v1_destroy(ctx->kbd);
  zwp_virtual_keyboard_manager_v1_destroy(ctx->kman);
  return;
}
