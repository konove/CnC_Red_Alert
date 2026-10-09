#include "engine/gfx/pixel_surface_test_util.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "engine/gfx/pixel_surface.h"

FakeSurface::FakeSurface(const int pitch, const int height)
    : pixels_(static_cast<size_t>(pitch) * static_cast<size_t>(height)),
      pitch_(pitch) {}

FakeSurface::FakeSurface(const int pitch, std::vector<uint8_t> pixels)
    : pixels_(std::move(pixels)), pitch_(pitch) {}

std::optional<PixelSurface::Pixels> FakeSurface::Lock() {
  ++lock_calls_;
  if (refuse_lock_) {
    return std::nullopt;
  }
  return Pixels{.bytes = pixels_, .pitch = pitch_};
}

void FakeSurface::Unlock() { ++unlock_calls_; }
