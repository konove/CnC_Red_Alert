// Holds the Rust XOR delta decoder in src/rust to the C++ one it replaced:
// the same target bytes, including the ones left alone, for valid, damaged
// and random deltas.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/lcg_test_util.h"
#include "base/numeric.h"
#include "engine/codec/xor_delta.h"
#include "engine/codec/xor_delta_reference_test_util.h"
#include "gtest/gtest.h"

namespace {

void Append(std::vector<std::byte>& delta, const uint32_t value) {
  delta.push_back(static_cast<std::byte>(value));
}

// Appends one well-formed command that covers 1 to `max_count` pixels.
void AppendCommand(base::Lcg& lcg, std::vector<std::byte>& delta,
                   const int max_count) {
  const int kind = lcg.Below(6);
  const bool is_long = kind >= 3;
  const auto count = static_cast<uint32_t>(
      1 + lcg.Below(std::min(max_count, is_long ? 600 : 127)));
  switch (kind) {
    case 0:  // Short run.
      Append(delta, 0);
      Append(delta, count);
      Append(delta, lcg.Byte());
      break;
    case 1:  // Short literal.
      Append(delta, count);
      for (uint32_t i = 0; i < count; ++i) {
        Append(delta, lcg.Byte());
      }
      break;
    case 2:  // Short skip.
      Append(delta, 0x80U | count);
      break;
    case 3:  // Long skip.
      Append(delta, 0x80);
      Append(delta, count & 0xffU);
      Append(delta, count >> 8U);
      break;
    case 4:  // Long literal.
      Append(delta, 0x80);
      Append(delta, count & 0xffU);
      Append(delta, 0x80U | (count >> 8U));
      for (uint32_t i = 0; i < count; ++i) {
        Append(delta, lcg.Byte());
      }
      break;
    default:  // Long run.
      Append(delta, 0x80);
      Append(delta, count & 0xffU);
      Append(delta, 0xc0U | (count >> 8U));
      Append(delta, lcg.Byte());
      break;
  }
}

// A delta of well-formed commands, which may still run past `pixels`.
std::vector<std::byte> ValidDelta(base::Lcg& lcg, const int pixels) {
  std::vector<std::byte> delta;
  for (int commands = lcg.Below(40); commands > 0; --commands) {
    AppendCommand(lcg, delta, std::max(1, pixels));
  }
  if (lcg.Below(2) == 0) {
    delta.insert(delta.end(), {std::byte{0x80}, std::byte{0}, std::byte{0}});
  }
  return delta;
}

// `delta` with up to three bytes changed and, one time in four, its tail cut
// off.
std::vector<std::byte> Damage(base::Lcg& lcg, std::vector<std::byte> delta) {
  for (int flips = lcg.Below(4); flips > 0 && !delta.empty(); --flips) {
    const auto at = base::ToSize(lcg.Below(static_cast<int>(delta.size())));
    delta.at(at) ^= static_cast<std::byte>(1 + lcg.Below(255));
  }
  if (lcg.Below(4) == 0 && !delta.empty()) {
    delta.resize(base::ToSize(lcg.Below(static_cast<int>(delta.size()))));
  }
  return delta;
}

// Applies `delta` with both decoders to copies of one random target, as a
// contiguous run and as rows of every kind of geometry.
void ExpectMatch(base::Lcg& lcg, const std::vector<std::byte>& delta,
                 const int size) {
  std::vector<uint8_t> original(base::ToSize(size));
  for (uint8_t& pixel : original) {
    pixel = lcg.Byte();
  }
  {
    auto want = original;
    auto got = original;
    xor_delta_reference::ApplyXorDelta(want, delta);
    ApplyXorDelta(got, delta);
    EXPECT_TRUE(got == want) << "contiguous, " << size << " bytes";
  }
  // Mostly sensible rows; sometimes a width or stride that must do nothing.
  const int width = lcg.Below(8) == 0 ? lcg.Below(5) - 2 : 1 + lcg.Below(40);
  const int stride =
      lcg.Below(8) == 0 ? lcg.Below(width + 3) - 1 : width + lcg.Below(9);
  for (const bool copy : {false, true}) {
    auto want = original;
    auto got = original;
    xor_delta_reference::ApplyXorDeltaToView(want, delta, width, stride, copy);
    ApplyXorDeltaToView(got, delta, width, stride, copy);
    EXPECT_TRUE(got == want) << "width " << width << ", stride " << stride
                             << ", copy " << copy << ", " << size << " bytes";
  }
}

TEST(XorDeltaDifferentialTest, ValidDeltasMatchReference) {
  base::Lcg lcg(1);
  for (int round = 0; round < 3000; ++round) {
    const int size = lcg.Below(2000);
    ExpectMatch(lcg, ValidDelta(lcg, size / 8), size);
  }
}

TEST(XorDeltaDifferentialTest, DamagedDeltasMatchReference) {
  base::Lcg lcg(2);
  for (int round = 0; round < 6000; ++round) {
    const int size = lcg.Below(2000);
    ExpectMatch(lcg, Damage(lcg, ValidDelta(lcg, size / 8)), size);
  }
}

TEST(XorDeltaDifferentialTest, RandomBytesMatchReference) {
  base::Lcg lcg(3);
  for (int round = 0; round < 6000; ++round) {
    const auto delta = lcg.Bytes(lcg.Below(200));
    ExpectMatch(lcg, delta, lcg.Below(4000));
  }
}

TEST(XorDeltaDifferentialTest, EmptySpansMatchReference) {
  base::Lcg lcg(4);
  ExpectMatch(lcg, {}, 0);
  ExpectMatch(lcg, {}, 16);
  ExpectMatch(lcg, ValidDelta(lcg, 16), 0);
}

TEST(XorDeltaDeathTest, RejectsADeltaInsideItsTarget) {
  std::vector<uint8_t> pixels(16);
  const auto delta = std::as_bytes(std::span(pixels)).subspan(8);
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(ApplyXorDelta(pixels, delta), "overlaps its target");
}

}  // namespace
