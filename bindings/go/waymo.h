#ifndef WAYMO_GO_H
#define WAYMO_GO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "waymo/events.h"
#include "waymo/btns.h"

typedef struct waymo_event_loop waymo_event_loop;
typedef struct eloop_params eloop_params;

waymo_event_loop* waymo_create_event_loop(const eloop_params* params);
void waymo_destroy_event_loop(waymo_event_loop* loop);
loop_status waymo_get_event_loop_status(waymo_event_loop* loop);

void waymo_move_mouse(waymo_event_loop* loop, unsigned int x, unsigned int y, int relative);
void waymo_click_mouse(waymo_event_loop* loop, MBTNS btn, unsigned int clicks, uint32_t hold_ms);
void waymo_press_mouse(waymo_event_loop* loop, MBTNS btn, int down);

void waymo_press_key(waymo_event_loop* loop, char key, uint32_t* interval_ms, int down);
void waymo_hold_key(waymo_event_loop* loop, char key, uint32_t* interval_ms, uint32_t hold_ms);
void waymo_type(waymo_event_loop* loop, const char* text, uint32_t* interval_ms);

#ifdef __cplusplus
}
#endif

#endif
