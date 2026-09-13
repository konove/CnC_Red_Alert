// Fixed-point conversion helpers shared by gameplay and value tests.

#include "td/jshell.h"

#include <cstdint>

// Both helpers compute in unsigned 32-bit arithmetic so their results,
// including the wraparound for inputs outside the game's range, match the
// original code bit for bit; the simulation depends on that.
int Cardinal_To_Fixed(int base, int cardinal) {
  if (base == 0) {
    return 0xFFFF;
  }

  return static_cast<int>((static_cast<uint32_t>(cardinal) << 8) /
                          static_cast<uint32_t>(base));
}

int Fixed_To_Cardinal(int base, int fixed) {
  const uint32_t ret =
      (static_cast<uint32_t>(base) * static_cast<uint32_t>(fixed)) + 0x80;

  if (ret & 0xFF000000) {
    return 0xFFFF;
  }

  return static_cast<int>(ret >> 8);
}
