#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

static inline uint64_t timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static inline void signal_done(int fd) {
  if (fd < 0) return;
  uint64_t sig = 1;
  while (write(fd, &sig, sizeof(sig)) < 0) {
    if (errno == EINTR) continue;
    break;
  }
}

#endif
