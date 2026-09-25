#include "engine/platform/random_seed.h"

#include <cstdint>
#include <random>

namespace platform {

int RandomSeed() {
  std::random_device device;
  return static_cast<int>(device() & uint32_t{0x7fffffff});
}

}  // namespace platform
