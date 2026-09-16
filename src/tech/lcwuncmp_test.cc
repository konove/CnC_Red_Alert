// Destination bounds and back-reference regressions for the legacy LCW decoder.

#include <array>
#include <span>
#include <vector>

#include "gtest/gtest.h"
#include "sdllib/iff.h"

namespace {

TEST(LcwDestinationTest, BoundsEveryCommand) {
  // Each stream emits abc followed by a command longer than the two bytes left.
  const std::vector<std::vector<unsigned char>> streams = {
      {0x83, 'a', 'b', 'c', 0x83, 'a', 'b', 'c', 0x80},
      {0x83, 'a', 'b', 'c', 0x00, 3, 0x80},
      {0x83, 'a', 'b', 'c', 0xc0, 0, 0, 0x80},
      {0x83, 'a', 'b', 'c', 0xff, 3, 0, 0, 0, 0x80},
      {0x83, 'a', 'b', 'c', 0xfe, 3, 0, 'a', 0x80},
  };
  for (const auto& stream : streams) {
    std::array<unsigned char, 10> output{};
    output.fill(0xcc);
    EXPECT_EQ(LCW_Uncompress(stream.data(), output.data() + 1, 5), 5);
    EXPECT_EQ(output[0], 0xcc);
    EXPECT_EQ(output[1], 'a');
    EXPECT_EQ(output[2], 'b');
    EXPECT_EQ(output[3], 'c');
    EXPECT_EQ(output[4], 'a');
    for (const unsigned char byte : std::span(output).subspan(6)) {
      EXPECT_EQ(byte, 0xcc);
    }
  }
}

TEST(LcwDestinationTest, RejectsReferencesOutsideDecodedPrefix) {
  const std::vector<std::vector<unsigned char>> streams = {
      {0x81, 'a', 0x00, 0, 0x80},
      {0x81, 'a', 0x00, 2, 0x80},
      {0x81, 'a', 0xc0, 1, 0, 0x80},
      {0x81, 'a', 0xc0, 0xff, 0xff, 0x80},
      {0x81, 'a', 0xff, 3, 0, 1, 0, 0x80},
      {0x81, 'a', 0xff, 3, 0, 0xff, 0xff, 0x80},
  };
  for (const auto& stream : streams) {
    std::array<unsigned char, 8> output{};
    output.fill(0xcc);
    EXPECT_EQ(LCW_Uncompress(stream.data(), output.data(), 8), 1);
    EXPECT_EQ(output[0], 'a');
    for (const unsigned char byte : std::span(output).subspan(1)) {
      EXPECT_EQ(byte, 0xcc);
    }
  }
}

TEST(LcwDestinationTest, PreservesOverlappingBackReferences) {
  const std::vector<std::vector<unsigned char>> streams = {
      {0x81, 'a', 0x20, 1, 0x80},
      {0x81, 'a', 0xc2, 0, 0, 0x80},
      {0x81, 'a', 0xff, 5, 0, 0, 0, 0x80},
  };
  for (const auto& stream : streams) {
    std::array<unsigned char, 7> output{};
    EXPECT_EQ(LCW_Uncompress(stream.data(), output.data(), 7), 6);
    for (const unsigned char byte : std::span(output).first(6)) {
      EXPECT_EQ(byte, 'a');
    }
    EXPECT_EQ(output[6], 0);
  }
}

TEST(LcwDestinationTest, AcceptsEmptyCommandsAndEndMarker) {
  constexpr std::array<unsigned char, 11> kStream = {
      0xff, 0, 0, 0xff, 0xff, 0xfe, 0, 0, 'x', 0x80, 0};
  std::array<unsigned char, 1> output = {0xcc};
  EXPECT_EQ(LCW_Uncompress(kStream.data(), output.data(), 1), 0);
  EXPECT_EQ(output[0], 0xcc);
}

TEST(LcwDestinationTest, NonpositiveCapacityDoesNotAccessBuffers) {
  EXPECT_EQ(LCW_Uncompress(nullptr, nullptr, 0), 0);
  EXPECT_EQ(LCW_Uncompress(nullptr, nullptr, -1), 0);
}

TEST(LcwDestinationTest, StopsAtCapacityWithoutReadingNextCommand) {
  constexpr std::array<unsigned char, 2> kStream = {0x81, 'a'};
  std::array<unsigned char, 1> output{};
  EXPECT_EQ(LCW_Uncompress(kStream.data(), output.data(), 1), 1);
  EXPECT_EQ(output[0], 'a');
}

TEST(LcwDestinationTest, SupportsInPlaceLiteralCopy) {
  std::array<unsigned char, 5> buffer = {0x83, 'a', 'b', 'c', 0x80};
  EXPECT_EQ(LCW_Uncompress(buffer.data(), buffer.data(), 3), 3);
  EXPECT_EQ(buffer[0], 'a');
  EXPECT_EQ(buffer[1], 'b');
  EXPECT_EQ(buffer[2], 'c');
}

}  // namespace
