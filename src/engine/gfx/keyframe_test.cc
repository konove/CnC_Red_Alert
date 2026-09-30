#include "engine/gfx/keyframe.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/unaligned.h"
#include "gtest/gtest.h"

namespace engine::gfx {
namespace {
std::vector<std::byte> FrameFile() {
  // Three 2x2 frames: an LCW key frame, a key delta, and a chained delta.
  std::vector<std::byte> data(64);
  base::WriteUnaligned<uint16_t>(data, 3);
  base::WriteUnaligned<uint16_t>(std::span(data).subspan(6), 2);
  base::WriteUnaligned<uint16_t>(std::span(data).subspan(8), 2);
  base::WriteUnaligned<uint32_t>(std::span(data).subspan(14), 0x80000028U);
  base::WriteUnaligned<uint32_t>(std::span(data).subspan(22), 0x4000002eU);
  base::WriteUnaligned<uint32_t>(std::span(data).subspan(26), 40);
  base::WriteUnaligned<uint32_t>(std::span(data).subspan(30), 0x20000036U);
  base::WriteUnaligned<uint32_t>(std::span(data).subspan(34), 1);
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
  EXPECT_EQ(BuildFrame(data, 0), (std::vector<uint8_t>{1, 2, 3, 4}));
  EXPECT_EQ(BuildFrame(data, 1), (std::vector<uint8_t>{0, 3, 2, 5}));
  EXPECT_EQ(BuildFrame(data, 2), (std::vector<uint8_t>{2, 1, 0, 7}));
}

TEST(KeyFrameBoundsTest, RejectsInvalidFramesAndOffsets) {
  auto data = FrameFile();
  EXPECT_TRUE(BuildFrame(data, -1).empty());
  EXPECT_TRUE(BuildFrame(data, 3).empty());
  EXPECT_TRUE(BuildFrame(std::span(data).first(15), 0).empty());
  base::WriteUnaligned<uint32_t>(std::span(data).subspan(14), 0x80ffffffU);
  EXPECT_TRUE(BuildFrame(data, 0).empty());
}

TEST(KeyFrameBoundsTest, LeavesPixelsShortDataMissesZero) {
  auto data = FrameFile();
  // The key frame's LCW literal now carries two of the four pixels.
  data.at(40) = std::byte{0x82};
  data.at(43) = std::byte{0x80};
  EXPECT_EQ(BuildFrame(data, 0), (std::vector<uint8_t>{1, 2, 0, 0}));
}

TEST(KeyFrameBoundsTest, RejectsFramesLargerThanTheCap) {
  auto data = FrameFile();
  static_assert(256 * 254 > kMaxFramePixels);
  base::WriteUnaligned<uint16_t>(std::span(data).subspan(6), 256);
  base::WriteUnaligned<uint16_t>(std::span(data).subspan(8), 254);
  EXPECT_TRUE(BuildFrame(data, 0).empty());
}

}  // namespace
}  // namespace engine::gfx
