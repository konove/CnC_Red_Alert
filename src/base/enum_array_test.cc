#include "base/enum_array.h"

#include <cstdint>

#include <gtest/gtest.h>

#include "base/types.h"

namespace base {
namespace {

enum class Fruit { kApple, kPear, kPlum };
enum Legacy : uint8_t { LEGACY_A, LEGACY_B };

TEST(EnumArrayTest, SubscriptsByEnumerator) {
  EnumArray<Fruit, int> counts = {1, 2, 3};
  EXPECT_EQ(counts[Fruit::kApple], 1);
  EXPECT_EQ(counts[Fruit::kPlum], 3);
  counts[Fruit::kPear] = 7;
  EXPECT_EQ(counts.elements[1], 7);
}

TEST(EnumArrayTest, SizeFollowsTheEnum) {
  static_assert(EnumArray<Fruit, int>::size() == 3);
  static_assert(EnumArray<Legacy, int>::size() == 2);
  static_assert(EnumArray<Fruit, int, 5>::size() == 5);
}

TEST(EnumArrayTest, IsAConstantExpressionAggregate) {
  static constexpr EnumArray<Fruit, const char*> kNames = {"apple", "pear", "plum"};
  static_assert(kNames[Fruit::kPear][0] == 'p');
  static_assert(kNames.size() == 3);
}

TEST(EnumArrayTest, IteratesInEnumeratorOrder) {
  const EnumArray<Fruit, int> values = {10, 20, 30};
  int sum = 0;
  ssize seen = 0;
  for (const int v : values) {
    sum += v;
    ++seen;
  }
  EXPECT_EQ(sum, 60);
  EXPECT_EQ(seen, values.size());
  EXPECT_EQ(values.end() - values.begin(), 3);
  EXPECT_EQ(values.data(), values.begin());
}

TEST(EnumArrayTest, NestsForTwoDimensionalTables) {
  constexpr EnumArray<Fruit, EnumArray<Legacy, int>> table = {{{1, 2}, {3, 4}, {5, 6}}};
  static_assert(table[Fruit::kPlum][LEGACY_B] == 6);
  EXPECT_EQ(table[Fruit::kPear][LEGACY_A], 3);
}

TEST(EnumArrayTest, ValueInitializesToZero) {
  const EnumArray<Fruit, int> zeros{};
  EXPECT_EQ(zeros[Fruit::kApple], 0);
  EXPECT_EQ(zeros[Fruit::kPlum], 0);
}

}  // namespace
}  // namespace base
