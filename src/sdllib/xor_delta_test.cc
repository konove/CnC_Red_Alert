#include "sdllib/xor_delta.h"

#include <array>
#include <cstdint>
#include <span>

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
