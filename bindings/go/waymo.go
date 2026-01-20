package waymo

/*
#include <stdlib.h>
#include "waymo/actions.h"
#include "waymo/events.h"
*/
import "C"

import (
	"runtime"
	"unsafe"
)

type EventLoop struct {
	ptr *C.waymo_event_loop
}

type MouseButton int

const (
	ButtonLeft   MouseButton = C.MBTN_LEFT
	ButtonRight  MouseButton = C.MBTN_RIGHT
	ButtonMiddle MouseButton = C.MBTN_MID
)

func NewEventLoop(maxCmds uint, kbdLayout string, cooldownMs uint32) (*EventLoop, error) {
	layout := C.CString(kbdLayout)
	defer C.free(unsafe.Pointer(layout))

	params := C.eloop_params{
		max_commands:       C.uint(maxCmds),
		kbd_layout:         layout,
		action_cooldown_ms: C.uint32_t(cooldownMs),
	}

	loopPtr := C.create_event_loop(&params)
	status := C.get_event_loop_status(loopPtr)
	if status != C.STATUS_OK {
		return nil, uint32(status)
	}

	loop := &EventLoop{ptr: loopPtr}
	
	// Ensure C memory is freed when Go object is GC'd
	runtime.SetFinalizer(loop, func(l *EventLoop) {
		C.destroy_event_loop(l.ptr)
	})

	return loop, nil
}

func (l *EventLoop) MoveMouse(x, y uint, relative bool) {
	C.move_mouse_wrapped(l.ptr, C.uint(x), C.uint(y), C.bool(relative))
}

func (l *EventLoop) ClickMouse(btn MouseButton, clicks uint, holdMs uint32) {
	C.click_mouse_wrapped(l.ptr, C.MBTNS(btn), C.uint(clicks), C.uint32_t(holdMs))
}

func (l *EventLoop) PressMouse(btn MouseButton, down bool) {
	C.press_mouse_wrapped(l.ptr, C.MBTNS(btn), C.bool(down))
}

func (l *EventLoop) PressKey(key byte, intervalMs *uint32, down bool) {
	var cInterval *C.uint32_t
	if intervalMs != nil {
		cInterval = (*C.uint32_t)(unsafe.Pointer(intervalMs))
	}
	C.press_key_blocking(l.ptr, C.char(key), cInterval, C.bool(down))
}

func (l *EventLoop) HoldKey(key byte, intervalMs *uint32, holdMs uint32) {
	var cInterval *C.uint32_t
	if intervalMs != nil {
		cInterval = (*C.uint32_t)(unsafe.Pointer(intervalMs))
	}
	C.hold_key_blocking(l.ptr, C.char(key), cInterval, C.uint32_t(holdMs))
}

func (l *EventLoop) Type(text string, intervalMs *uint32) {
	cStr := C.CString(text)
	defer C.free(unsafe.Pointer(cStr))

	var cInterval *C.uint32_t
	if intervalMs != nil {
		cInterval = (*C.uint32_t)(unsafe.Pointer(intervalMs))
	}
	C.type_wrapped(l.ptr, cStr, cInterval)
}
