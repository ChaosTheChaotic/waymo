#ifndef ATOMIC_COMPAT_H
#define ATOMIC_COMPAT_H

#ifdef __cplusplus
#include <atomic>
#define WAYMO_ATOMIC(t) std::atomic<t>
#define WAYMO_ATOMIC_BOOL std::atomic<bool>
#else
#include <stdatomic.h>
#define WAYMO_ATOMIC(t) _Atomic t
#define WAYMO_ATOMIC_BOOL atomic_bool
#endif

#endif
