/**
 * @file actions.h
 * @brief Input APIs for Waymo event loop
 */

#ifndef ACTIONS_H
#define ACTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "waymo/actions_internal.h"
#include <sys/eventfd.h>
#include <unistd.h>

/**
 * @brief Moves the mouse around the screen
 * @param[in] loop     Pointer to the event loop
 * @param[in] x	      X coordinate on the screen
 * @param[in] y	      Y coordinate on the screen
 * @param[in] relative True if the coordinates should be relative to current
 * coordinates
 */
static inline void move_mouse(waymo_event_loop *loop, unsigned int x,
                              unsigned int y, bool relative) {
  WAIT_COMPLETE(_send_command, loop, _create_mouse_move_cmd(x, y, relative));
}

/**
 * @brief Clicks a button on the mouse
 * @param[in] loop    Pointer to the event loop
 * @param[in] btn     Button to click from MBTNS enum (include btns.h)
 * @param[in] clicks  The number of times to click
 * @param[in] hold_ms The time in ms to hold the button down per click
 */
static inline void click_mouse(waymo_event_loop *loop, MBTNS btn,
                               unsigned int clicks, uint32_t hold_ms) {
  WAIT_COMPLETE(_send_command, loop,
                _create_mouse_click_cmd(btn, clicks, hold_ms));
}

/**
 * @brief Change a mouse button to be down or up
 * @param[in] loop  Pointer to the event loop
 * @param[in] btn   Button to click from the MBTNS enum (include btns.h)
 * @param[in] down  If the button should be down or up
 */
static inline void press_mouse(waymo_event_loop *loop, MBTNS btn, bool down) {
  WAIT_COMPLETE(_send_command, loop, _create_mouse_button_cmd(btn, down));
}

/**
 * @brief Press a key down or up
 * Pressing keys with a dynamic keymap on Wayland does not work well due to
 * refreshing of that keymap This means that I must constantly press that key to
 * emulate holding
 * @param[in] loop	    Pointer to the event loop
 * @param[in] key  	    The key to press
 * @param[in] interval_ms  A pointer to a uint32_t to represent how long in
 * between each press (pass NULL for default)
 * @param[in] down	    If the key to be pressed should be down or not
 */
static inline void press_key(waymo_event_loop *loop, char key,
                             uint32_t *interval_ms, bool down) {
#ifdef __cplusplus
  _send_command(loop, _create_keyboard_key_cmd_b(key, interval_ms, down), -1);
#else
  _send_command(loop, _create_keyboard_key_cmd(key, interval_ms, down), -1);
#endif
}

/**
 * @brief Holds a key down for some amount of time
 * Pressing keys with a dynamic keymap on Wayland does not work well due to
 * refreshing of the dynamic keymap This means that the keys must be held down
 * to emulate holding
 * @param[in] loop	    Pointer to the event loop
 * @param[in] key	    The key to press
 * @param[in] interval_ms  A pointer to a uint32_t to represent how long should
 * be left in between each press (NULL for default)
 * @param[in] hold_ms	    How long the key should be held for in ms
 */
static inline void hold_key(waymo_event_loop *loop, char key,
                            uint32_t *interval_ms, uint32_t hold_ms) {
#ifdef __cplusplus
  _send_command(loop, _create_keyboard_key_cmd_uintt(key, interval_ms, hold_ms),
                -1);
#else
  _send_command(loop, _create_keyboard_key_cmd(key, interval_ms, hold_ms), -1);
#endif
}

/**
 * @brief Types a string
 * @param[in] loop	 Pointer to the event loop
 * @param[in] text 	 String to type out
 * @param[in] interval_ms A pointer to the ms between each key being clicked
 * (NULL for default)
 */
static inline void type(waymo_event_loop *loop, const char *text,
                        uint32_t *interval_ms) {
  WAIT_COMPLETE(_send_command, loop,
                _create_keyboard_type_cmd(text, interval_ms));
}

#ifdef __cplusplus
}
#endif

#endif
