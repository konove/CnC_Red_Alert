// Tests for RangeStream, the read-only window onto another stream.

#include "engine/stream/range_stream.h"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "engine/stream/byte_stream.h"
#include "engine/stream/memory_stream.h"
#include "engine/stream/seek_origin.h"
#include "gtest/gtest.h"

namespace {

std::span<const std::byte> Bytes(const std::string_view text) {
  return std::as_bytes(std::span(text));
}

std::string ReadAll(ByteStream& stream, const int count) {
  std::string out(static_cast<size_t>(count), '\0');
  out.resize(
      static_cast<size_t>(stream.Read(std::as_writable_bytes(std::span(out)))));
  return out;
}

TEST(RangeStreamTest, SeesOnlyItsWindow) {
  RangeStream range(std::make_unique<MemoryStream>(Bytes("xabcdy")), 1, 4);
  EXPECT_EQ(range.Size(), 4);
  EXPECT_EQ(ReadAll(range, 2), "ab");
  EXPECT_EQ(range.Tell(), 2);
  EXPECT_EQ(ReadAll(range, 10), "cd");
  EXPECT_EQ(range.Seek(-1, SeekOrigin::kEnd), 3);
  EXPECT_EQ(ReadAll(range, 10), "d");
  EXPECT_EQ(range.Seek(-10, SeekOrigin::kCurrent), 0);
  EXPECT_EQ(range.Seek(10, SeekOrigin::kBegin), 4);
  EXPECT_EQ(range.Write(Bytes("z")), 0);
}

TEST(RangeStreamTest, WindowIsClippedToTheInnerStream) {
  RangeStream range(std::make_unique<MemoryStream>(Bytes("xabcd")), 3, 10);
  EXPECT_EQ(range.Size(), 2);
  EXPECT_EQ(ReadAll(range, 10), "cd");
  RangeStream beyond(std::make_unique<MemoryStream>(Bytes("xabcd")), 9, 1);
  EXPECT_EQ(beyond.Size(), 0);
}

TEST(RangeStreamTest, NestsInsideAnotherRange) {
  // "xabcd" sits at offset 2 of the outer data; "bc" at offset 2 of that.
  auto outer = std::make_unique<RangeStream>(
      std::make_unique<MemoryStream>(Bytes("--xabcd--")), 2, 5);
  RangeStream inner(std::move(outer), 2, 2);
  EXPECT_EQ(inner.Size(), 2);
  EXPECT_EQ(ReadAll(inner, 10), "bc");
  EXPECT_EQ(inner.Seek(1, SeekOrigin::kBegin), 1);
  EXPECT_EQ(ReadAll(inner, 10), "c");
}

}  // namespace
