/**
 * @file events.h
 * @brief APIs for the event loop management
 */

#ifndef PEVENTS_H
#define PEVENTS_H

#include <pthread.h>
#include <stdint.h>

/**
 * @brief The params that can be passed to the create_event_loop
 */
typedef struct eloop_params {
  unsigned int max_commands;   /**< The max commands in the queue */
  char *kbd_layout;            /**< The layout of the keyboard */
  uint32_t action_cooldown_ms; /**< Cooldown between each action */
} eloop_params;

typedef enum {
  STATUS_OK = 0,
  STATUS_INIT_FAILED = 1 << 0, // This error is fatal
  STATUS_KBD_FAILED = 1 << 1,
  STATUS_PTR_FAILED = 1 << 2,
} loop_status;

typedef struct waymo_event_loop waymo_event_loop;

/**
 * @brief The function which creates an event loop
 * This function creates an event loop that can be used to send inputs
 * @param[in] params A pointer to eloop_params struct allowing for configuration
 * (NULL for default)
 */
waymo_event_loop *create_event_loop(const eloop_params *params);

/**
 * @brief Destroys the created event loop
 * @param[in] loop A pointer to the loop to be freed
 */
void destroy_event_loop(waymo_event_loop *loop);

/**
 * @brief Gets the status of the event loop
 * This function should be checked just after the event loop is created
 * This ensures that all components work fine and are initialized properly
 * @param[in] loop A pointer to the loop to be checked
 */
loop_status get_event_loop_status(waymo_event_loop *loop);

#endif
