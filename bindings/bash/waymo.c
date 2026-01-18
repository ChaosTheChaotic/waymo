#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <waymo/actions.h>
#include <waymo/btns.h>
#include <waymo/events.h>

bool parse_bool(const char *str) {
  if (!str)
    return false;
  return (strcasecmp(str, "true") == 0 || strcmp(str, "1") == 0);
}

bool parse_mouse_btn(const char *str, MBTNS *out_btn) {
  if (!str)
    return false;

  if (strcasecmp(str, "left") == 0) {
    *out_btn = MBTN_LEFT;
    return true;
  }
  if (strcasecmp(str, "right") == 0) {
    *out_btn = MBTN_RIGHT;
    return true;
  }
  if (strcasecmp(str, "middle") == 0) {
    *out_btn = MBTN_MID;
    return true;
  }

  return false;
}

void print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s [options] <action> [args]\n", prog);
  fprintf(
      stderr,
      "Options:\n  -l, --layout <lang>  Set keyboard layout (default: us)\n\n");
  fprintf(stderr, "Actions:\n");
  fprintf(stderr, "  move <x> <y> [is_relative]\n");
  fprintf(stderr, "  click <btn> [clicks] [hold_ms]\n");
  fprintf(stderr, "  press_mouse <btn> <is_down>\n");
  fprintf(stderr, "  type <text> [interval_ms]\n");
  fprintf(stderr, "  press_key <char> <is_down> [interval_ms]\n");
  fprintf(stderr, "  hold_key <char> <hold_ms> [interval_ms]\n");
}

int main(int argc, char **argv) {
  char *layout = "us";
  int opt;

  static struct option long_options[] = {{"layout", required_argument, 0, 'l'},
                                         {"help", no_argument, 0, 'h'},
                                         {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "l:h", long_options, NULL)) != -1) {
    switch (opt) {
    case 'l':
      layout = optarg;
      break;
    case 'h':
      print_usage(argv[0]);
      return 0;
    default:
      print_usage(argv[0]);
      return 1;
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Error: Missing 'action' argument.\n");
    print_usage(argv[0]);
    return 1;
  }

  const eloop_params params = {
      .action_cooldown_ms = 0, .kbd_layout = layout, .max_commands = 1};

  waymo_event_loop *loop = create_event_loop(&params);
  if (!loop) {
    fprintf(stderr, "Failed to create event loop.\n");
    return 1;
  }

  char *action = argv[optind];
  int args_left = argc - (optind + 1);
  char **args = &argv[optind + 1];
  int ret = 0;

  if (strcmp(action, "move") == 0) {
    if (args_left < 2) {
      fprintf(stderr, "Usage: move <x> <y> [relative]\n");
      ret = 1;
      goto cleanup;
    }
    unsigned int x = strtoul(args[0], NULL, 10);
    unsigned int y = strtoul(args[1], NULL, 10);
    bool rel = (args_left >= 3) ? parse_bool(args[2]) : false;

    move_mouse(loop, x, y, rel);

  } else if (strcmp(action, "click") == 0) {
    if (args_left < 1) {
      fprintf(stderr, "Usage: click <btn> [clicks] [hold_ms]\n");
      ret = 1;
      goto cleanup;
    }

    MBTNS btn;
    if (!parse_mouse_btn(args[0], &btn)) {
      fprintf(stderr,
              "Error: Invalid mouse button '%s' (Use: left, right, middle)\n",
              args[0]);
      ret = 1;
      goto cleanup;
    }

    unsigned int clicks = (args_left >= 2) ? strtoul(args[1], NULL, 10) : 1;
    uint32_t hold = (args_left >= 3) ? strtoul(args[2], NULL, 10) : 0;

    click_mouse(loop, btn, clicks, hold);

  } else if (strcmp(action, "press_mouse") == 0) {
    if (args_left < 2) {
      fprintf(stderr, "Usage: press_mouse <btn> <down>\n");
      ret = 1;
      goto cleanup;
    }

    MBTNS btn;
    if (!parse_mouse_btn(args[0], &btn)) {
      fprintf(stderr,
              "Error: Invalid mouse button '%s' (Use: left, right, middle)\n",
              args[0]);
      ret = 1;
      goto cleanup;
    }

    bool down = parse_bool(args[1]);
    press_mouse(loop, btn, down);

  } else if (strcmp(action, "type") == 0) {
    if (args_left < 1) {
      fprintf(stderr, "Usage: type <text> [interval_ms]\n");
      ret = 1;
      goto cleanup;
    }
    const char *text = args[0];
    uint32_t interval_val;
    uint32_t *interval_ptr = NULL;

    if (args_left >= 2) {
      interval_val = strtoul(args[1], NULL, 10);
      interval_ptr = &interval_val;
    }

    type(loop, text, interval_ptr);

  } else if (strcmp(action, "press_key") == 0) {
    if (args_left < 2) {
      fprintf(stderr, "Usage: press_key <char> <down> [interval_ms]\n");
      ret = 1;
      goto cleanup;
    }
    char key = args[0][0];
    bool down = parse_bool(args[1]);

    uint32_t interval_val;
    uint32_t *interval_ptr = NULL;
    if (args_left >= 3) {
      interval_val = strtoul(args[2], NULL, 10);
      interval_ptr = &interval_val;
    }

    press_key(loop, key, interval_ptr, down);

  } else if (strcmp(action, "hold_key") == 0) {
    if (args_left < 2) {
      fprintf(stderr, "Usage: hold_key <char> <hold_ms> [interval_ms]\n");
      ret = 1;
      goto cleanup;
    }
    char key = args[0][0];
    uint32_t hold_ms = strtoul(args[1], NULL, 10);

    uint32_t interval_val;
    uint32_t *interval_ptr = NULL;
    if (args_left >= 3) {
      interval_val = strtoul(args[2], NULL, 10);
      interval_ptr = &interval_val;
    }

    hold_key(loop, key, interval_ptr, hold_ms);

  } else {
    fprintf(stderr, "Unknown action: %s\n", action);
    print_usage(argv[0]);
    ret = 1;
  }

cleanup:
  destroy_event_loop(loop);
  return ret;
}
