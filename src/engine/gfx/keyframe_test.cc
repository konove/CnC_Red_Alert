#include "ra/keyframe.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "engine/base/unaligned.h"
#include "gtest/gtest.h"

namespace {
std::vector<std::byte> FrameFile() {
  // Three 2x2 frames: an LCW key frame, a key delta, and a chained delta.
  std::vector<std::byte> data(64);
  port::WriteUnaligned<uint16_t>(data, 3);
  port::WriteUnaligned<uint16_t>(std::span(data).subspan(6), 2);
  port::WriteUnaligned<uint16_t>(std::span(data).subspan(8), 2);
  port::WriteUnaligned<uint32_t>(std::span(data).subspan(14), 0x80000028U);
  port::WriteUnaligned<uint32_t>(std::span(data).subspan(22), 0x4000002eU);
  port::WriteUnaligned<uint32_t>(std::span(data).subspan(26), 40);
  port::WriteUnaligned<uint32_t>(std::span(data).subspan(30), 0x20000036U);
  port::WriteUnaligned<uint32_t>(std::span(data).subspan(34), 1);
  const std::array<uint8_t, 22> stream{
      0x84, 1, 2, 3, 4, 0x80,         // LCW literal and stop.
      4,    1, 1, 1, 1, 0x80, 0, 0,   // XOR key delta.
      4,    2, 2, 2, 2, 0x80, 0, 0};  // XOR chained delta.
  for (size_t i = 0; i < stream.size(); ++i) {
    data.at(40 + i) = std::byte{stream.at(i)};
  }
  return data;
}

TEST(KeyFrameBoundsTest, BuildsKeyAndChainedDeltaFrames) {
  const auto data = FrameFile();
  std::array<uint8_t, 6> output{};
  output.fill(0xa5);
  ASSERT_EQ(Build_Frame(data, 0, output).size(), 4);
  EXPECT_EQ(output, (std::array<uint8_t, 6>{1, 2, 3, 4, 0xa5, 0xa5}));
  ASSERT_EQ(Build_Frame(data, 1, output).size(), 4);
  EXPECT_EQ(output, (std::array<uint8_t, 6>{0, 3, 2, 5, 0xa5, 0xa5}));
  ASSERT_EQ(Build_Frame(data, 2, output).size(), 4);
  EXPECT_EQ(output, (std::array<uint8_t, 6>{2, 1, 0, 7, 0xa5, 0xa5}));
}

TEST(KeyFrameBoundsTest, RejectsSmallDestinationsAndInvalidOffsets) {
  auto data = FrameFile();
  std::array<uint8_t, 3> output{0xa5, 0xa5, 0xa5};
  EXPECT_TRUE(Build_Frame(data, 0, output).empty());
  EXPECT_EQ(output.front(), 0xa5);
  std::array<uint8_t, 4> full{};
  EXPECT_TRUE(Build_Frame(std::span(data).first(15), 0, full).empty());
  port::WriteUnaligned<uint32_t>(std::span(data).subspan(14), 0x80ffffffU);
  EXPECT_TRUE(Build_Frame(data, 0, full).empty());
}
}  // namespace
