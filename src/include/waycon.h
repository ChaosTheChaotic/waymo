#ifndef WAYCON_H
#define WAYCON_H

#include "event_loop.h"
#include "wvk.h"
#include "wvp.h"
#include <stdbool.h>
#include <stdint.h>
#include <linux/input-event-codes.h>

typedef struct {
  struct wl_display *display;
  uint32_t screen_width;
  uint32_t screen_height;
  int32_t scale_factor;
  struct wl_registry *registry;
  struct wl_seat *seat;
  struct zwp_virtual_keyboard_manager_v1 *kman;
  struct zwp_virtual_keyboard_v1 *kbd;
  struct zwlr_virtual_pointer_manager_v1 *pman;
  struct zwlr_virtual_pointer_v1 *ptr;
} waymoctx;

waymoctx *init_waymoctx(char *layout, _Atomic loop_status *status);
void destroy_waymoctx(waymoctx *ctx);

bool waymoctx_connect(waymoctx *ctx, _Atomic loop_status *status);
void waymoctx_destroy_connect(waymoctx *ctx);

bool waymoctx_kbd(waymoctx *ctx, char *layout);
void waymoctx_destroy_kbd(waymoctx *ctx);

bool waymoctx_pointer(waymoctx *ctx);
void waymoctx_destroy_pointer(waymoctx *ctx);

void emouse_move(waymoctx *ctx, command_param *param);
void emouse_click(waymo_event_loop *loop, waymoctx *ctx, command_param *param);
void emouse_btn(waymoctx *ctx, command_param *param);

void ekbd_type(waymoctx *ctx, command_param *param);
void ekbd_key(waymo_event_loop *loop, waymoctx *ctx, command_param *param);

static inline uint32_t mbtnstoliec(MBTNS btn) {
  switch (btn) {
    case MBTN_LEFT:
      return BTN_LEFT;
    case MBTN_RIGHT:
      return BTN_RIGHT;
    case MBTN_MID:
      return BTN_MIDDLE;
    default: return 0;
  }
}

struct Key {
  uint32_t keycode;
  bool shift;
};

static inline struct Key chartokey(char c) {
    // We want one table at compile time
    static const struct Key map[128] = {
        // Lowercase
        ['a'] = {KEY_A, false}, ['b'] = {KEY_B, false}, ['c'] = {KEY_C, false},
        ['d'] = {KEY_D, false}, ['e'] = {KEY_E, false}, ['f'] = {KEY_F, false},
        ['g'] = {KEY_G, false}, ['h'] = {KEY_H, false}, ['i'] = {KEY_I, false},
        ['j'] = {KEY_J, false}, ['k'] = {KEY_K, false}, ['l'] = {KEY_L, false},
        ['m'] = {KEY_M, false}, ['n'] = {KEY_N, false}, ['o'] = {KEY_O, false},
        ['p'] = {KEY_P, false}, ['q'] = {KEY_Q, false}, ['r'] = {KEY_R, false},
        ['s'] = {KEY_S, false}, ['t'] = {KEY_T, false}, ['u'] = {KEY_U, false},
        ['v'] = {KEY_V, false}, ['w'] = {KEY_W, false}, ['x'] = {KEY_X, false},
        ['y'] = {KEY_Y, false}, ['z'] = {KEY_Z, false},

        // Uppercase (Same keycode, Shift = true)
        ['A'] = {KEY_A, true},  ['B'] = {KEY_B, true},  ['C'] = {KEY_C, true},
        ['D'] = {KEY_D, true},  ['E'] = {KEY_E, true},  ['F'] = {KEY_F, true},
        ['G'] = {KEY_G, true},  ['H'] = {KEY_H, true},  ['I'] = {KEY_I, true},
        ['J'] = {KEY_J, true},  ['K'] = {KEY_K, true},  ['L'] = {KEY_L, true},
        ['M'] = {KEY_M, true},  ['N'] = {KEY_N, true},  ['O'] = {KEY_O, true},
        ['P'] = {KEY_P, true},  ['Q'] = {KEY_Q, true},  ['R'] = {KEY_R, true},
        ['S'] = {KEY_S, true},  ['T'] = {KEY_T, true},  ['U'] = {KEY_U, true},
        ['V'] = {KEY_V, true},  ['W'] = {KEY_W, true},  ['X'] = {KEY_X, true},
        ['Y'] = {KEY_Y, true},  ['Z'] = {KEY_Z, true},

        // Numbers & Symbols (US Layout)
        ['1'] = {KEY_1, false}, ['!'] = {KEY_1, true},
        ['2'] = {KEY_2, false}, ['@'] = {KEY_2, true},
        ['3'] = {KEY_3, false}, ['#'] = {KEY_3, true},
        ['4'] = {KEY_4, false}, ['$'] = {KEY_4, true},
        ['5'] = {KEY_5, false}, ['%'] = {KEY_5, true},
        ['6'] = {KEY_6, false}, ['^'] = {KEY_6, true},
        ['7'] = {KEY_7, false}, ['&'] = {KEY_7, true},
        ['8'] = {KEY_8, false}, ['*'] = {KEY_8, true},
        ['9'] = {KEY_9, false}, ['('] = {KEY_9, true},
        ['0'] = {KEY_0, false}, [')'] = {KEY_0, true},

        // Special Characters
        [' ']  = {KEY_SPACE, false},  ['\n'] = {KEY_ENTER, false},
        ['\t'] = {KEY_TAB, false},    ['\b'] = {KEY_BACKSPACE, false},
        ['.']  = {KEY_DOT, false},    ['>']  = {KEY_DOT, true},
        [',']  = {KEY_COMMA, false},  ['<']  = {KEY_COMMA, true},
        ['/']  = {KEY_SLASH, false},  ['?']  = {KEY_SLASH, true},
        [';']  = {KEY_SEMICOLON, false}, [':'] = {KEY_SEMICOLON, true},
        ['\''] = {KEY_APOSTROPHE, false},['\"'] = {KEY_APOSTROPHE, true},
        ['[']  = {KEY_LEFTBRACE, false}, ['{']  = {KEY_LEFTBRACE, true},
        [']']  = {KEY_RIGHTBRACE, false},['}']  = {KEY_RIGHTBRACE, true},
        ['\\'] = {KEY_BACKSLASH, false}, ['|']  = {KEY_BACKSLASH, true},
        ['-']  = {KEY_MINUS, false},     ['_']  = {KEY_MINUS, true},
        ['=']  = {KEY_EQUAL, false},     ['+']  = {KEY_EQUAL, true},
        ['`']  = {KEY_GRAVE, false},     ['~']  = {KEY_GRAVE, true},
    };

    // Bounds check to prevent out-of-bounds access with extended ASCII
    if ((unsigned char)c >= 128) {
        return (struct Key){0, false};
    }

    return map[(unsigned char)c];
}

#endif
