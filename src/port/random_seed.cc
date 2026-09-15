#include "port/random_seed.h"

#include <cstdint>
#include <random>

namespace port {

int RandomSeed() {
  std::random_device device;
  return static_cast<int>(device() & uint32_t{0x7fffffff});
}

}  // namespace port
