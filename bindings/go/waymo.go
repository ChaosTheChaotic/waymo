package waymo

/*
#cgo CFLAGS: -I${SRCDIR}/../include -I${SRCDIR}
#cgo LDFLAGS: -L${SRCDIR}/../lib/static -lwaymo -Wl,-Bstatic -lwaymo -Wl,-Bdynamic -lxkbcommon -lwayland-client -lm -lpthread

#include "waymo.h"
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"runtime"
	"unsafe"
)

// EventLoop represents a Waymo event loop
type EventLoop struct {
	ptr *C.waymo_event_loop
}

// EventLoopParams represents configuration parameters for the event loop
type EventLoopParams struct {
	MaxCommands      uint
	KeyboardLayout   string
	ActionCooldownMS uint32
}

// LoopStatus represents the status of the event loop
type LoopStatus uint

const (
	StatusOK          LoopStatus = C.STATUS_OK
	StatusInitFailed  LoopStatus = C.STATUS_INIT_FAILED
	StatusKbdFailed   LoopStatus = C.STATUS_KBD_FAILED
	StatusPtrFailed   LoopStatus = C.STATUS_PTR_FAILED
)

// MouseButton represents mouse buttons
type MouseButton int

const (
	MouseButtonLeft MouseButton = C.MBTN_LEFT
	MouseButtonRight MouseButton = C.MBTN_RIGHT
	MouseButtonMiddle MouseButton = C.MBTN_MID
)

// NewEventLoop creates a new Waymo event loop
func NewEventLoop(params *EventLoopParams) (*EventLoop, error) {
	var cParams *C.eloop_params
	
	if params != nil {
		cParams = &C.eloop_params{}
		cParams.max_commands = C.uint(params.MaxCommands)
		if params.KeyboardLayout != "" {
			cParams.kbd_layout = C.CString(params.KeyboardLayout)
			defer C.free(unsafe.Pointer(cParams.kbd_layout))
		}
		cParams.action_cooldown_ms = C.uint32_t(params.ActionCooldownMS)
	}
	
	loop := &EventLoop{
		ptr: C.waymo_create_event_loop(cParams),
	}
	
	if loop.ptr == nil {
		return nil, errors.New("failed to create event loop")
	}
	
	// Set finalizer to ensure cleanup
	runtime.SetFinalizer(loop, func(l *EventLoop) {
		l.Close()
	})
	
	// Check status
	status := C.waymo_get_event_loop_status(loop.ptr)
	if status != C.STATUS_OK {
		return loop, errors.New("event loop initialization failed with status: " + LoopStatus(status).String())
	}
	
	return loop, nil
}

// Close destroys the event loop
func (e *EventLoop) Close() error {
	if e.ptr != nil {
		C.waymo_destroy_event_loop(e.ptr)
		e.ptr = nil
	}
	return nil
}

// Status returns the current status of the event loop
func (e *EventLoop) Status() LoopStatus {
	if e.ptr == nil {
		return StatusInitFailed
	}
	return LoopStatus(C.waymo_get_event_loop_status(e.ptr))
}

// MoveMouse moves the mouse cursor
func (e *EventLoop) MoveMouse(x, y uint, relative bool) {
	if e.ptr == nil {
		return
	}
	C.waymo_move_mouse(e.ptr, C.uint(x), C.uint(y), C.int(boolToInt(relative)))
}

// ClickMouse clicks a mouse button
func (e *EventLoop) ClickMouse(btn MouseButton, clicks uint, holdMs uint32) {
	if e.ptr == nil {
		return
	}
	C.waymo_click_mouse(e.ptr, C.MBTNS(btn), C.uint(clicks), C.uint32_t(holdMs))
}

// PressMouse presses or releases a mouse button
func (e *EventLoop) PressMouse(btn MouseButton, down bool) {
	if e.ptr == nil {
		return
	}
	C.waymo_press_mouse(e.ptr, C.MBTNS(btn), C.int(boolToInt(down)))
}

// PressKey presses or releases a keyboard key
func (e *EventLoop) PressKey(key byte, intervalMs *uint32, down bool) {
	if e.ptr == nil {
		return
	}
	var intervalPtr *C.uint32_t
	if intervalMs != nil {
		intervalPtr = (*C.uint32_t)(unsafe.Pointer(intervalMs))
	}
	C.waymo_press_key(e.ptr, C.char(key), intervalPtr, C.int(boolToInt(down)))
}

// HoldKey holds a key down for specified time
func (e *EventLoop) HoldKey(key byte, intervalMs *uint32, holdMs uint32) {
	if e.ptr == nil {
		return
	}
	var intervalPtr *C.uint32_t
	if intervalMs != nil {
		intervalPtr = (*C.uint32_t)(unsafe.Pointer(intervalMs))
	}
	C.waymo_hold_key(e.ptr, C.char(key), intervalPtr, C.uint32_t(holdMs))
}

// Type types a string
func (e *EventLoop) Type(text string, intervalMs *uint32) {
	if e.ptr == nil {
		return
	}
	cText := C.CString(text)
	defer C.free(unsafe.Pointer(cText))
	
	var intervalPtr *C.uint32_t
	if intervalMs != nil {
		intervalPtr = (*C.uint32_t)(unsafe.Pointer(intervalMs))
	}
	
	C.waymo_type(e.ptr, cText, intervalPtr)
}

// Helper function
func boolToInt(b bool) int {
	if b {
		return 1
	}
	return 0
}

// String returns string representation of LoopStatus
func (s LoopStatus) String() string {
	switch s {
	case StatusOK:
		return "OK"
	case StatusInitFailed:
		return "Initialization failed"
	case StatusKbdFailed:
		return "Keyboard initialization failed"
	case StatusPtrFailed:
		return "Pointer initialization failed"
	default:
		return "Unknown status"
	}
}
