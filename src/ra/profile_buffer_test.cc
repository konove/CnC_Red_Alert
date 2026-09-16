#include "ra/profile.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/buffer.h"
#include "gtest/gtest.h"

namespace {

TEST(ProfileBinaryTest, NumberUsesTheExactBufferAndRejectsInvalidSeeks) {
  char storage[4]{};
  const uint32_t value = 0x12345678;
  ASSERT_TRUE(Write_Bin_Init(storage));
  EXPECT_EQ(Write_Bin_PosSet(-1, storage), -1);
  EXPECT_EQ(Write_Bin_PosSet(5, storage), -1);
  ASSERT_TRUE(Write_Bin_Num(base::ObjectBytes(value), 4, storage));
  EXPECT_EQ(Write_Bin_Length(storage), 4);
  EXPECT_FALSE(Write_Bin_Num(base::ObjectBytes(value), 1, storage));
  ASSERT_TRUE(Read_Bin_Init(storage));
  EXPECT_EQ(Read_Bin_PosSet(-1, storage), -1);
  EXPECT_EQ(Read_Bin_PosSet(5, storage), -1);
  uint32_t copy = 0;
  ASSERT_TRUE(Read_Bin_Num(base::ObjectBytes(copy), 4, storage));
  EXPECT_EQ(copy, value);
  EXPECT_FALSE(Read_Bin_Num(base::ObjectBytes(copy), 1, storage));
}

TEST(ProfileBinaryTest, ShortStringOutputDoesNotAdvanceTheCursor) {
  char storage[5]{};
  ASSERT_TRUE(Write_Bin_Init(storage));
  ASSERT_TRUE(Write_Bin_String("abc", storage));
  EXPECT_EQ(Write_Bin_Length(storage), 5);
  ASSERT_TRUE(Read_Bin_Init(storage));
  char small[3]{};
  EXPECT_FALSE(Read_Bin_String(small, storage));
  EXPECT_EQ(Read_Bin_Pos(storage), 0);
  char output[4]{};
  ASSERT_TRUE(Read_Bin_String(output, storage));
  EXPECT_STREQ(output, "abc");
  EXPECT_EQ(Read_Bin_Pos(storage), 5);
}

TEST(ProfileBinaryTest, UnterminatedEncodedStringIsRejected) {
  char storage[] = {3, 'a', 'b', 'c', 'x'};
  ASSERT_TRUE(Read_Bin_Init(storage));
  char output[4]{};
  EXPECT_FALSE(Read_Bin_String(output, storage));
  EXPECT_EQ(Read_Bin_Pos(storage), 0);
}

TEST(ProfileBinaryTest, InputLengthCannotExceedItsSpan) {
  char storage[4]{};
  const uint8_t value = 7;
  ASSERT_TRUE(Write_Bin_Init(storage));
  EXPECT_FALSE(Write_Bin_Num(base::ObjectBytes(value), 4, storage));
  EXPECT_EQ(Write_Bin_Pos(storage), 0);
  ASSERT_TRUE(Read_Bin_Init(storage));
  std::byte output{};
  EXPECT_FALSE(Read_Bin_Num(base::ObjectBytes(output), 4, storage));
  EXPECT_EQ(Read_Bin_Pos(storage), 0);
}

}  // namespace
