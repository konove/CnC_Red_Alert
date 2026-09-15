#include "port/env.h"

#include <cstdlib>
#include <optional>
#include <string>

namespace port {

std::optional<std::string> GetEnv(const char* name) {
  // The value is copied before anything else can touch the environment; the
  // game never sets variables from another thread. The NOLINT covers only
  // that check.
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const char* const value = std::getenv(name);
  if (value == nullptr) {
    return std::nullopt;
  }
  return std::string(value);
}

}  // namespace port
