#include "engine/base/bytes_of.h"

#include <array>
#include <cstdint>
#include <type_traits>

#include "engine/base/buffer.h"
#include "gtest/gtest.h"

namespace {

struct Record {
  uint16_t word;
  uint8_t byte;
};

TEST(BytesOf, ScalarAddressesTheObject) {
  uint32_t value = 0x01020304;
  unsigned char* bytes = base::BytesOf(value);
  EXPECT_EQ(static_cast<void*>(bytes), static_cast<void*>(&value));

  uint32_t copy = 0;
  base::CopyBytes(base::ObjectBytes(copy), base::ObjectBytes(value), sizeof(copy));
  EXPECT_EQ(copy, value);
}

TEST(BytesOf, WritesReachTheObject) {
  Record record{};
  std::array<unsigned char, sizeof(Record)> pattern{};
  pattern.fill(0x5A);
  base::CopyBytes(base::ObjectBytes(record), base::ObjectBytes(pattern), pattern.size());
  EXPECT_EQ(record.word, 0x5A5A);
  EXPECT_EQ(record.byte, 0x5A);
}

TEST(BytesOf, ConstObjectGivesConstBytes) {
  const Record record{.word = 1, .byte = 2};
  static_assert(
      std::is_same_v<decltype(base::BytesOf(record)), const unsigned char*>);
  EXPECT_EQ(static_cast<const void*>(base::BytesOf(record)),
            static_cast<const void*>(&record));
}

TEST(BytesOf, ArrayAddressesFirstElement) {
  const std::array<uint16_t, 3> words = {7, 8, 9};
  EXPECT_EQ(static_cast<const void*>(base::BytesOf(words)),
            static_cast<const void*>(words.data()));
}

}  // namespace
