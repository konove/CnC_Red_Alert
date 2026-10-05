// Output bounds and back-reference regressions for the lenient LCW decoder.

#include "engine/codec/lcw_uncompress.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "base/types.h"
#include "gtest/gtest.h"

namespace {

// LcwUncompress() over the byte view of `stream`.
std::vector<uint8_t> Decode(const std::span<const unsigned char> stream,
                            const base::ssize max_size) {
  return LcwUncompress(std::as_bytes(stream), max_size);
}

TEST(LcwUncompressTest, BoundsEveryCommand) {
  // Each stream emits abc followed by a command longer than the two bytes left.
  const std::vector<std::vector<unsigned char>> streams = {
      {0x83, 'a', 'b', 'c', 0x83, 'a', 'b', 'c', 0x80},
      {0x83, 'a', 'b', 'c', 0x00, 3, 0x80},
      {0x83, 'a', 'b', 'c', 0xc0, 0, 0, 0x80},
      {0x83, 'a', 'b', 'c', 0xff, 3, 0, 0, 0, 0x80},
      {0x83, 'a', 'b', 'c', 0xfe, 3, 0, 'a', 0x80},
  };
  for (const auto& stream : streams) {
    const auto output = Decode(stream, 5);
    ASSERT_EQ(output.size(), 5U);
    EXPECT_EQ(output.at(0), 'a');
    EXPECT_EQ(output.at(1), 'b');
    EXPECT_EQ(output.at(2), 'c');
    EXPECT_EQ(output.at(3), 'a');
  }
}

TEST(LcwUncompressTest, RejectsReferencesOutsideDecodedPrefix) {
  const std::vector<std::vector<unsigned char>> streams = {
      {0x81, 'a', 0x00, 0, 0x80},
      {0x81, 'a', 0x00, 2, 0x80},
      {0x81, 'a', 0xc0, 1, 0, 0x80},
      {0x81, 'a', 0xc0, 0xff, 0xff, 0x80},
      {0x81, 'a', 0xff, 3, 0, 1, 0, 0x80},
      {0x81, 'a', 0xff, 3, 0, 0xff, 0xff, 0x80},
  };
  for (const auto& stream : streams) {
    EXPECT_EQ(Decode(stream, 8), std::vector<uint8_t>{'a'});
  }
}

TEST(LcwUncompressTest, PreservesOverlappingBackReferences) {
  const std::vector<std::vector<unsigned char>> streams = {
      {0x81, 'a', 0x20, 1, 0x80},
      {0x81, 'a', 0xc2, 0, 0, 0x80},
      {0x81, 'a', 0xff, 5, 0, 0, 0, 0x80},
  };
  for (const auto& stream : streams) {
    EXPECT_EQ(Decode(stream, 7), std::vector<uint8_t>(6, 'a'));
  }
}

TEST(LcwUncompressTest, AcceptsEmptyCommandsAndEndMarker) {
  constexpr std::array<unsigned char, 11> kStream = {
      0xff, 0, 0, 0xff, 0xff, 0xfe, 0, 0, 'x', 0x80, 0};
  EXPECT_TRUE(Decode(kStream, 1).empty());
}

TEST(LcwUncompressTest, EmptyStreamDecodesToNothing) {
  EXPECT_TRUE(Decode({}, 0).empty());
  EXPECT_TRUE(Decode({}, 8).empty());
}

TEST(LcwUncompressTest, StopsAtMaxSizeWithoutReadingNextCommand) {
  constexpr std::array<unsigned char, 2> kStream = {0x81, 'a'};
  EXPECT_EQ(Decode(kStream, 1), std::vector<uint8_t>{'a'});
}

TEST(LcwUncompressTest, TruncatedCommandsKeepDecodedPrefix) {
  const std::vector<std::vector<unsigned char>> streams = {
      {0x81, 'a', 0x00},
      {0x81, 'a', 0xfe, 3},
      {0x81, 'a', 0xff, 3, 0},
      {0x81, 'a', 0xc0, 0},
  };
  for (const auto& stream : streams) {
    EXPECT_EQ(Decode(stream, 8), std::vector<uint8_t>{'a'});
  }
}

TEST(LcwUncompressInPlaceTest, SupportsInPlaceLiteralCopy) {
  std::array<unsigned char, 5> buffer = {0x83, 'a', 'b', 'c', 0x80};
  EXPECT_EQ(LcwUncompressInPlace(buffer, std::span(buffer).first(3)), 3);
  EXPECT_EQ(buffer.at(0), 'a');
  EXPECT_EQ(buffer.at(1), 'b');
  EXPECT_EQ(buffer.at(2), 'c');
}

TEST(LcwUncompressInPlaceTest, CopiesInAStreamStoredElsewhere) {
  constexpr std::array<unsigned char, 4> kStream = {0x82, 'a', 'b', 0x80};
  std::array<unsigned char, 3> output = {0xcc, 0xcc, 0xcc};
  EXPECT_EQ(LcwUncompressInPlace(kStream, output), 2);
  EXPECT_EQ(output, (std::array<unsigned char, 3>{'a', 'b', 0xcc}));
}

// The WSA and VQA layout: the stream at the tail of the buffer, the output
// growing from the front and into the bytes already read.
TEST(LcwUncompressInPlaceTest, DecodesAStreamAtTheTailOfItsDestination) {
  std::array<unsigned char, 9> buffer = {0xcc, 0xcc, 0xcc, 0x84, 'a',
                                         'b',  'c',  'd',  0x80};
  EXPECT_EQ(
      LcwUncompressInPlace(std::span<const unsigned char>(buffer).subspan(3),
                           std::span(buffer).first(4)),
      4);
  EXPECT_EQ(buffer, (std::array<unsigned char, 9>{'a', 'b', 'c', 'd', 'a', 'b',
                                                  'c', 'd', 0x80}));
}

TEST(LcwUncompressInPlaceDeathTest, RejectsAStreamOverlappingTheFront) {
  std::array<unsigned char, 16> buffer = {};
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(
      LcwUncompressInPlace(std::span<const unsigned char>(buffer).first(8),
                           std::span(buffer).subspan(4, 8)),
      "overlaps the front");
}

TEST(LcwUncompressInPlaceTest, EmptyBuffersDoNotAccessStorage) {
  EXPECT_EQ(LcwUncompressInPlace(std::span<const unsigned char>{},
                                 std::span<unsigned char>{}),
            0);
}

}  // namespace
