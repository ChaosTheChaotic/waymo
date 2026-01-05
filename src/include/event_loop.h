#ifndef ELT_H
#define ELT_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

typedef enum {
  CMD_MOUSE_MOVE,    // Takes x, y and if movement should be relative
  CMD_MOUSE_CLICK,   // Takes the button and num clicks
  CMD_MOUSE_BTN,     // Takes button and if down
  CMD_KEYBOARD_TYPE, // Takes key and num clicks
  CMD_KEYBOARD_KEY,  // Takes key and if down
  CMD_QUIT,
} command_type;

typedef union {
  struct {
    int x, y;
    bool relative;
  } pos;
  struct {
    int button;
    unsigned int clicks;
    unsigned long long click_length;
  } mouse_click;
  struct {
    int button;
    bool down;
  } mouse_btn;
  struct {
    char key;
    bool down;
  } keyboard_key;
  struct {
    char *txt;
  } kbd;
} command_param;

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
} waymo_event_loop;

waymo_event_loop *create_event_loop(struct eloop_params *params);
void destroy_event_loop(waymo_event_loop *loop);
void send_command(waymo_event_loop *loop, command *cmd);

command *create_mouse_move_cmd(int x, int y);
command *create_mouse_click_cmd(int button, int clicks);
command *create_keyboard_type_cmd(const char *text);
command *create_quit_cmd();
void free_command(command *cmd);

#endif
