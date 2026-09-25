// Tests for MemoryStream, the read-only ByteStream over borrowed bytes.

#include "engine/stream/memory_stream.h"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include "engine/stream/byte_stream.h"
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

TEST(MemoryStreamTest, ReadsWithinTheViewAndClampsSeeks) {
  MemoryStream stream(Bytes("xabcd"));
  EXPECT_EQ(stream.Size(), 5);
  EXPECT_EQ(ReadAll(stream, 3), "xab");
  EXPECT_EQ(stream.Seek(10, SeekOrigin::kCurrent), 5);
  EXPECT_EQ(ReadAll(stream, 3), "");
  EXPECT_EQ(stream.Seek(-100, SeekOrigin::kEnd), 0);
  EXPECT_EQ(stream.Seek(4, SeekOrigin::kBegin), 4);
  EXPECT_EQ(ReadAll(stream, 3), "d");
  EXPECT_EQ(stream.Write(Bytes("z")), 0);
}

}  // namespace
