#ifndef WINIT_H
#define WINIT_H

#include <stdatomic.h>

#define NINIT 0 // Not initialized
#define IINIT 1 // Initializing
#define DINIT 2 // Done Initializing

extern atomic_ushort istate;

void ensure_init();

#endif
