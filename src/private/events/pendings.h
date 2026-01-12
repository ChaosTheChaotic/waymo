#ifndef PENDINGS_H
#define PENDINGS_H

#include <stdint.h>
#include <stdbool.h>
#include "event_loop.h"
#include "wayland/waycon.h"

// For non blocking delays
enum action_type { ACTION_KEY_RELEASE, ACTION_MOUSE_RELEASE, ACTION_CLICK_STEP, ACTION_TYPE_STEP };

struct pending_action {
    uint64_t expiry_ms;
    enum action_type type;
    int done_fd;
    union {
        struct { uint32_t keycode; bool shift; } key;
        struct { uint32_t button; } mouse;
        struct { uint32_t button; uint32_t ms; unsigned int remaining; bool is_down; } click;
	struct { char *txt; unsigned int index; } type_txt;
	    } data;
    struct pending_action *next;
};

void update_timer(waymo_event_loop *loop);
void schedule_action(waymo_event_loop *loop, struct pending_action *action);
void clear_pending_actions(waymo_event_loop *loop);
void handle_timer_expiry(waymo_event_loop *loop, waymoctx *ctx);

#endif
