// Fixed-point conversion helpers shared by gameplay and value tests.

#include "td/jshell.h"

unsigned int Cardinal_To_Fixed(unsigned base, unsigned cardinal) {
  if (!base) {
    return 0xFFFF;
  }

  return (cardinal << 8) / base;
}

unsigned int Fixed_To_Cardinal(unsigned base, unsigned fixed) {
  unsigned ret = base * fixed + 0x80;

  if (ret & 0xFF000000) {
    return 0xFFFF;
  }

  return ret >> 8;
}
