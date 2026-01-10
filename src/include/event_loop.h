#ifndef ELT_H
#define ELT_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

typedef enum {
  CMD_MOUSE_MOVE,    // Takes x, y and if movement should be relative
  CMD_MOUSE_CLICK,   // Takes the button and num clicks
  CMD_MOUSE_BTN,     // Takes button and if down
  CMD_KEYBOARD_TYPE, // Takes key and num clicks
  CMD_KEYBOARD_KEY,  // Takes key and if down
  CMD_QUIT,
} command_type;

typedef enum {
  MBTN_LEFT,
  MBTN_RIGHT,
  MBTN_MID,
}MBTNS;

enum KMODOPT {
  DOWN,
  HOLD,
};

typedef union {
  struct {
    int x, y;
    bool relative;
  } pos;
  struct {
    MBTNS button;
    unsigned int clicks;
    uint32_t click_ms;
  } mouse_click;
  struct {
    MBTNS button;
    bool down;
  } mouse_btn;
  struct {
    char key;
    enum KMODOPT active_opt;
    union {
      bool down;
      uint32_t hold_ms;
    } keyboard_key_mod;
  } keyboard_key;
  struct {
    char *txt;
  } kbd;
} command_param;

// For non blocking delays
enum action_type { ACTION_KEY_RELEASE, ACTION_MOUSE_RELEASE, ACTION_CLICK_STEP };

struct pending_action {
    uint64_t expiry_ms;
    enum action_type type;
    union {
        struct { uint32_t keycode; bool shift; } key;
        struct { uint32_t button; } mouse;
        struct { uint32_t button; uint32_t ms; unsigned int remaining; bool is_down; } click;
    } data;
    struct pending_action *next;
};

typedef struct {
  command_type type;
  command_param param;
} command;

typedef struct {
  command **commands;
  unsigned int num_commands;
  unsigned int front;
  unsigned int back;
  unsigned int max_capacity;
  pthread_mutex_t mutex;
  int fd;
  bool shutdown;
} command_queue;

struct eloop_params {
  unsigned int max_commands;
  char *layout;
};

typedef enum {
  STATUS_OK = 0,
  STATUS_INIT_FAILED = 1 << 0, // This error is fatal
  STATUS_KBD_FAILED = 1 << 1,
  STATUS_PTR_FAILED = 1 << 2,
} loop_status;

typedef struct {
  pthread_t thread;
  command_queue *queue;
  char *layout;
  _Atomic loop_status status;
  sem_t ready_sem;
  int timer_fd;
  struct pending_action *pending_head;
} waymo_event_loop;

waymo_event_loop *create_event_loop(struct eloop_params *params);
void destroy_event_loop(waymo_event_loop *loop);
void send_command(waymo_event_loop *loop, command *cmd);

command* create_mouse_move_cmd(int x, int y, bool relative);
command* create_mouse_click_cmd(int button, int clicks, uint32_t click_ms);
command* create_mouse_button_cmd(int button, bool down);

command* create_keyboard_key_cmd_b(char key, bool down);
command* create_keyboard_key_cmd_uintt(char key, uint32_t hold_ms);

#define create_keyboard_key_cmd(A, B)                                          \
  _Generic((B),                                                                \
      bool: create_keyboard_key_cmd_b,                                         \
      uint32_t: create_keyboard_key_cmd_uintt, )(A)

command* create_keyboard_type_cmd(const char *text);
command* create_quit_cmd();
void free_command(command *cmd);

static inline uint32_t timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void schedule_action(waymo_event_loop *loop, struct pending_action *action);

#endif
