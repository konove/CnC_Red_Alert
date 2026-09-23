// Tests for the ByteStream helpers (typed reads, ReadBytes, ReadString),
// exercised through a MemoryStream.

#include "tech/byte_stream.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string>
#include <string_view>

#include "base/array.h"
#include "base/buffer.h"
#include "gtest/gtest.h"
#include "tech/memory_stream.h"

namespace {

std::span<const std::byte> Bytes(const std::string_view text) {
  return std::as_bytes(std::span(text));
}

TEST(ByteStreamTest, ReadObjectFailsOnShortRead) {
  const std::array<std::byte, 3> bytes{};
  MemoryStream memory(bytes);
  ByteStream& stream = memory;
  int32_t value = 0;
  EXPECT_FALSE(stream.ReadObject(value));
}

TEST(ByteStreamTest, ArraysAreObjectsToo) {
  const int16_t written[3] = {1, 2, 3};
  MemoryStream memory(std::as_bytes(std::span(written)));
  ByteStream& stream = memory;
  int16_t read[3] = {};
  EXPECT_TRUE(stream.ReadObject(read));
  EXPECT_EQ(base::At(read, 2), 3);
}

TEST(ByteStreamTest, ReadBytesAndReadStringStopAtEndOfStream) {
  const std::string_view text = "abcde";
  MemoryStream memory(std::as_bytes(std::span(text)));
  ByteStream& stream = memory;
  EXPECT_EQ(stream.ReadString(3), "abc");
  EXPECT_EQ(std::ssize(stream.ReadBytes(10)), 2);
}

TEST(ByteStreamTest, SpanAndRawPointerReadsAgree) {
  MemoryStream memory(Bytes("xyz"));
  ByteStream& stream = memory;
  std::array<std::byte, 2> first{};
  EXPECT_EQ(stream.Read(std::span(first)), 2);
  EXPECT_EQ(static_cast<char>(first.at(1)), 'y');
  char last = 0;
  EXPECT_EQ(stream.Read(base::ObjectBytes(last)), 1);
  EXPECT_EQ(last, 'z');
}

TEST(ByteStreamTest, TypedViewCountIsBytesRatherThanElements) {
  const std::array<uint16_t, 2> words{0x0102, 0x0304};
  MemoryStream memory(std::as_bytes(std::span(words)));
  ByteStream& stream = memory;
  std::array<uint16_t, 2> read{};
  EXPECT_EQ(stream.Read(std::span(read), 2), 2);  // 2 bytes = 1 element
  EXPECT_EQ(read.at(0), words.at(0));
  EXPECT_EQ(read.at(1), 0);
}

TEST(ByteStreamTest, TypedViewCountStopsMidElement) {
  MemoryStream memory(Bytes("abcd"));
  ByteStream& stream = memory;
  std::array<uint16_t, 2> words{};
  EXPECT_EQ(stream.Read(std::span(words), 3), 3);  // 3 bytes: 1 full, 1 half
  const auto bytes = std::as_bytes(std::span(words));
  EXPECT_EQ(base::At(bytes, 0), std::byte{'a'});
  EXPECT_EQ(base::At(bytes, 1), std::byte{'b'});
  EXPECT_EQ(base::At(bytes, 2), std::byte{'c'});
  EXPECT_EQ(base::At(bytes, 3), std::byte{0});
}

}  // namespace
