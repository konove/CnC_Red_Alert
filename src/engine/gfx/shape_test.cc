// Tests for CountShapeFrames and ExtractShapeFrame: reading a shape file's
// frame table, and rejecting tables and headers that point outside the file.

#include "engine/gfx/shape.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <vector>

#include "base/numeric.h"
#include "gtest/gtest.h"

namespace engine::gfx {
namespace {

// Shape file images, built little-endian.
using Bytes = std::vector<std::byte>;

// LLVM 23 mistakes element invalidation for invalidating the vector reference;
// no element reference or iterator is retained across these appends.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void AppendU16(Bytes& file, const uint16_t value) {
  file.push_back(static_cast<std::byte>(value & 0xFFU));
  file.push_back(static_cast<std::byte>(value >> 8U));
}

// LLVM 23 mistakes element invalidation for invalidating the vector reference;
// no element reference or iterator is retained across these appends.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void AppendU32(Bytes& file, const uint32_t value) {
  for (uint32_t shift = 0; shift < 32; shift += 8) {
    file.push_back(static_cast<std::byte>((value >> shift) & 0xFFU));
  }
}

void AppendPadding(Bytes& file, const int count) {
  file.insert(file.end(), base::ToSize(count), std::byte{0});
}

// Appends a kShapeHeaderSize-byte header whose frame_bytes field says
// `frame_bytes`.
void AppendHeader(Bytes& file, const uint16_t frame_bytes) {
  AppendU16(file, 0);  // format
  AppendPadding(file, 1);
  AppendU16(file, 4);  // width
  AppendPadding(file, 1);
  AppendU16(file, frame_bytes);
  AppendU16(file, 0);  // decompressed_bytes
}

// A file of `count` frames whose offset table holds `offsets`.
Bytes FileWithTable(const uint16_t count,
                    const std::vector<uint32_t>& offsets) {
  Bytes file;
  AppendU16(file, count);
  for (const uint32_t offset : offsets) {
    AppendU32(file, offset);
  }
  return file;
}

TEST(CountShapeFramesTest, ReadsTheCount) {
  EXPECT_EQ(CountShapeFrames(FileWithTable(3, {})), 3);
}

TEST(CountShapeFramesTest, TooShortForTheCountIsEmpty) {
  EXPECT_EQ(CountShapeFrames(Bytes{std::byte{5}}), 0);
  EXPECT_EQ(CountShapeFrames({}), 0);
}

// Offsets count from the end of the count: the table is 8 bytes, so frame 0
// starts at 8 and frame 1 (12 bytes long) right after frame 0's 10-byte
// header.
TEST(ExtractShapeFrameTest, ReturnsEachFrameWithItsHeader) {
  Bytes file = FileWithTable(2, {8, 18});
  AppendHeader(file, 10);
  AppendHeader(file, 12);
  AppendPadding(file, 2);
  const std::span<const std::byte> whole = file;

  const auto frame0 = ExtractShapeFrame(whole, 0);
  EXPECT_EQ(frame0.data(), whole.subspan(10).data());
  EXPECT_EQ(std::ssize(frame0), 10);

  const auto frame1 = ExtractShapeFrame(whole, 1);
  EXPECT_EQ(frame1.data(), whole.subspan(20).data());
  EXPECT_EQ(std::ssize(frame1), 12);
}

TEST(ExtractShapeFrameTest, FrameNumberOutOfRangeIsEmpty) {
  Bytes file = FileWithTable(1, {4});
  AppendHeader(file, 10);
  EXPECT_TRUE(ExtractShapeFrame(file, -1).empty());
  EXPECT_TRUE(ExtractShapeFrame(file, 1).empty());
}

TEST(ExtractShapeFrameTest, TableShorterThanTheCountIsEmpty) {
  EXPECT_TRUE(ExtractShapeFrame(FileWithTable(2, {4}), 1).empty());
}

TEST(ExtractShapeFrameTest, OffsetPastTheEndIsEmpty) {
  Bytes file = FileWithTable(1, {0xFFFFFFFFU});
  AppendHeader(file, 10);
  EXPECT_TRUE(ExtractShapeFrame(file, 0).empty());
}

TEST(ExtractShapeFrameTest, FrameShorterThanItsHeaderIsEmpty) {
  Bytes file = FileWithTable(1, {4});
  AppendPadding(file, 9);
  EXPECT_TRUE(ExtractShapeFrame(file, 0).empty());
}

TEST(ExtractShapeFrameTest, SizeSmallerThanTheHeaderIsEmpty) {
  Bytes file = FileWithTable(1, {4});
  AppendHeader(file, 9);
  EXPECT_TRUE(ExtractShapeFrame(file, 0).empty());
}

TEST(ExtractShapeFrameTest, SizePastTheEndIsEmpty) {
  Bytes file = FileWithTable(1, {4});
  AppendHeader(file, 11);
  EXPECT_TRUE(ExtractShapeFrame(file, 0).empty());
}

}  // namespace
}  // namespace engine::gfx
