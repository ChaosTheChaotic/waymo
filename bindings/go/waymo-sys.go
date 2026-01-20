package waymo

/*
#cgo pkg-config: wayland-client xkbcommon
#cgo LDFLAGS: -l:libwaymo.a -lstdc++
#cgo CFLAGS: -I${SRCDIR}/../../include

#include "waymo/actions.h"
#include "waymo/events.h"
#include "waymo/actions_internal.h"

static void move_mouse_wrapped(waymo_event_loop *loop, unsigned int x, unsigned int y, bool relative) {
    move_mouse(loop, x, y, relative);
}

static void click_mouse_wrapped(waymo_event_loop *loop, MBTNS btn, unsigned int clicks, uint32_t hold_ms) {
    click_mouse(loop, btn, clicks, hold_ms);
}

static void press_mouse_wrapped(waymo_event_loop *loop, MBTNS btn, bool down) {
    press_mouse(loop, btn, down);
}

static void press_key_wrapped(waymo_event_loop *loop, char key, uint32_t *interval_ms, bool down) {
    press_key(loop, key, interval_ms, down);
}

static void hold_key_wrapped(waymo_event_loop *loop, char key, uint32_t *interval_ms, uint32_t hold_ms) {
    hold_key(loop, key, interval_ms, hold_ms);
}

static void type_wrapped(waymo_event_loop *loop, const char *text, uint32_t *interval_ms) {
    type(loop, text, interval_ms);
}
*/
import "C"
