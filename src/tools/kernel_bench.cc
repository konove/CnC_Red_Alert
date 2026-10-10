// Times the engine kernels that moved to Rust (src/rust) against the C++
// references they replaced. Run a RelWithDebInfo build; see CLAUDE.md on Debug.
//
// Usage: kernel_bench

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/lcg_test_util.h"
#include "engine/codec/xor_delta.h"
#include "engine/codec/xor_delta_reference_test_util.h"

namespace {

// Returns how many megabytes a second `function` gets through if one call
// handles `bytes`: the best of five timings of enough calls to fill 50 ms.
template <typename Function>
double MegabytesPerSecond(const std::size_t bytes, const Function& function) {
  using Clock = std::chrono::steady_clock;
  double best = 0;
  for (int attempt = 0; attempt < 5; ++attempt) {
    int calls = 0;
    const auto start = Clock::now();
    std::chrono::duration<double> elapsed{};
    do {
      function();
      ++calls;
      elapsed = Clock::now() - start;
    } while (elapsed.count() < 0.05);
    const double megabytes =
        static_cast<double>(bytes) * calls / (1024.0 * 1024.0);
    best = std::max(best, megabytes / elapsed.count());
  }
  return best;
}

void Report(const char* const what, const double rust, const double cpp) {
  absl::PrintF("%-26s Rust %8.1f MB/s   C++ %8.1f MB/s   %+.0f%%\n", what,
               rust, cpp, ((rust / cpp) - 1.0) * 100.0);
}

// A delta over `pixels` pixels shaped like an animation frame's: short and
// long runs, literals and skips in turn.
std::vector<std::byte> FrameDelta(const int pixels) {
  base::Lcg lcg(1);
  std::vector<std::byte> delta;
  const auto append = [&delta](const uint32_t value) {
    delta.push_back(static_cast<std::byte>(value));
  };
  for (int left = pixels; left > 0;) {
    const int kind = lcg.Below(5);
    const int pixels_covered =
        std::min(left, 1 + lcg.Below(kind == 4 ? 400 : 127));
    left -= pixels_covered;
    const auto count = static_cast<uint32_t>(pixels_covered);
    switch (kind) {
      case 0:
        append(0);
        append(count);
        append(lcg.Byte());
        break;
      case 1:
      case 2:
        append(count);
        for (uint32_t i = 0; i < count; ++i) {
          append(lcg.Byte());
        }
        break;
      case 3:
        append(0x80U | count);
        break;
      default:
        append(0x80);
        append(count & 0xffU);
        append(0x80U | (count >> 8U));
        for (uint32_t i = 0; i < count; ++i) {
          append(lcg.Byte());
        }
        break;
    }
  }
  delta.insert(delta.end(), {std::byte{0x80}, std::byte{0}, std::byte{0}});
  return delta;
}

void BenchXorDelta() {
  constexpr int kWidth = 320;
  constexpr int kHeight = 200;
  constexpr int kStride = 640;
  const auto delta = FrameDelta(kWidth * kHeight);
  std::vector<uint8_t> flat(std::size_t{kWidth} * kHeight);
  std::vector<uint8_t> view(std::size_t{kStride} * kHeight);
  Report(
      "xor delta, contiguous",
      MegabytesPerSecond(flat.size(), [&] { ApplyXorDelta(flat, delta); }),
      MegabytesPerSecond(flat.size(), [&] {
        xor_delta_reference::ApplyXorDelta(flat, delta);
      }));
  Report("xor delta, rows", MegabytesPerSecond(flat.size(), [&] {
           ApplyXorDeltaToView(view, delta, kWidth, kStride, /*copy=*/false);
         }),
         MegabytesPerSecond(flat.size(), [&] {
           xor_delta_reference::ApplyXorDeltaToView(view, delta, kWidth,
                                                    kStride, /*copy=*/false);
         }));
}

}  // namespace

int main() {
  BenchXorDelta();
  return 0;
}
