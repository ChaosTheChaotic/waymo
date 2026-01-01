#include "internal/winit.h"

atomic_ushort istate = NINIT;

void ensure_init() {
  unsigned short exp = NINIT;
  if (atomic_compare_exchange_strong(&istate, &exp, IINIT)) {
    // TODO: Impl init
    atomic_store(&istate, DINIT);
  }
}
