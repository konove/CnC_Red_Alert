#include "engine/codec/xor_delta.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

TEST(XorDeltaTest, BoundsRunsSkipsAndTruncatedCommands) {
  std::array<uint8_t, 6> output{1, 2, 3, 4, 5, 6};
  const std::array<uint8_t, 6> run{0, 3, 0x10, 0x80, 0, 0};
  ApplyXorDelta(std::span(output).first(3), std::as_bytes(std::span(run)));
  EXPECT_EQ(output, (std::array<uint8_t, 6>{0x11, 0x12, 0x13, 4, 5, 6}));
  const auto before = output;
  const std::array<uint8_t, 3> excessive_run{0, 7, 0xff};
  ApplyXorDelta(output, std::as_bytes(std::span(excessive_run)));
  EXPECT_EQ(output, before);
  const std::array<uint8_t, 2> truncated{0x80, 0};
  ApplyXorDelta(output, std::as_bytes(std::span(truncated)));
  EXPECT_EQ(output, before);
  const std::array<uint8_t, 4> skip{0x86, 1, 0xff, 0};
  ApplyXorDelta(output, std::as_bytes(std::span(skip)));
  EXPECT_EQ(output, before);
}

TEST(XorDeltaTest, CopiesRowsWithoutTouchingPadding) {
  std::array<uint8_t, 8> output{};
  output.fill(0xa5);
  const std::array<uint8_t, 8> literal{4, 1, 2, 3, 4, 0x80, 0, 0};
  ApplyXorDeltaToView(output, std::as_bytes(std::span(literal)), 2, 4,
                      /*copy=*/true);
  EXPECT_EQ(output,
            (std::array<uint8_t, 8>{1, 2, 0xa5, 0xa5, 3, 4, 0xa5, 0xa5}));
}

TEST(XorDeltaTest, XorsRowsWithoutTouchingPadding) {
  std::array<uint8_t, 8> output{};
  output.fill(0xa5);
  const std::array<uint8_t, 8> literal{4, 1, 2, 3, 4, 0x80, 0, 0};
  ApplyXorDeltaToView(output, std::as_bytes(std::span(literal)), 2, 4,
                      /*copy=*/false);
  EXPECT_EQ(output, (std::array<uint8_t, 8>{0xa4, 0xa7, 0xa5, 0xa5, 0xa6, 0xa1,
                                            0xa5, 0xa5}));
}

// Counts above 255 take the long commands' high byte.
TEST(XorDeltaTest, LongCommandsUseTheirHighByte) {
  std::vector<uint8_t> output(600);
  // Skip 300, run 260 of 0x5a, end.
  const std::array<uint8_t, 10> skip_and_run{0x80, 0x2c, 0x01, 0x80, 0x04,
                                             0xc1, 0x5a, 0x80, 0,    0};
  ApplyXorDelta(output, std::as_bytes(std::span(skip_and_run)));
  for (std::size_t i = 0; i < output.size(); ++i) {
    ASSERT_EQ(output.at(i), i >= 300 && i < 560 ? 0x5a : 0) << "at " << i;
  }

  // A literal of 257 bytes.
  std::vector<uint8_t> literal{0x80, 0x01, 0x81};
  for (int i = 0; i < 257; ++i) {
    literal.push_back(static_cast<uint8_t>(i));
  }
  std::ranges::fill(output, 0);
  ApplyXorDelta(output, std::as_bytes(std::span(literal)));
  for (std::size_t i = 0; i < output.size(); ++i) {
    ASSERT_EQ(output.at(i), i < 257 ? static_cast<uint8_t>(i) : 0)
        << "at " << i;
  }
}

TEST(XorDeltaTest, IgnoresRowsOfNoWidthOrTooShortAStride) {
  std::array<uint8_t, 8> output{};
  output.fill(0xa5);
  const std::array<uint8_t, 3> run{0, 8, 0xff};
  for (const auto& [width, stride] :
       {std::pair(0, 4), std::pair(-1, 4), std::pair(4, 0), std::pair(4, 3)}) {
    ApplyXorDeltaToView(output, std::as_bytes(std::span(run)), width, stride,
                        /*copy=*/true);
    EXPECT_EQ(output, (std::array<uint8_t, 8>{0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
                                              0xa5, 0xa5, 0xa5}))
        << "width " << width << ", stride " << stride;
  }
}

TEST(XorDeltaDeathTest, RejectsADeltaInsideItsTarget) {
  std::vector<uint8_t> pixels(16);
  const auto delta = std::as_bytes(std::span(pixels)).subspan(8);
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(ApplyXorDelta(pixels, delta), "overlaps its target");
}
