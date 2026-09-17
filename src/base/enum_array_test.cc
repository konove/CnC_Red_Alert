#include "base/enum_array.h"

#include <cstdint>

#include <gtest/gtest.h>

#include "base/types.h"

namespace base {
namespace {

enum class Fruit { kApple, kPear, kPlum };
enum class Legacy : uint8_t { kA, kB };

TEST(EnumArrayTest, SubscriptsByEnumerator) {
  EnumArray<Fruit, int> counts = {1, 2, 3};
  EXPECT_EQ(counts.at(Fruit::kApple), 1);
  EXPECT_EQ(counts.at(Fruit::kPlum), 3);
  counts.at(Fruit::kPear) = 7;
  EXPECT_EQ(counts.elements[1], 7);
}

TEST(EnumArrayTest, SizeFollowsTheEnum) {
  static_assert(EnumArray<Fruit, int>::size() == 3);
  static_assert(EnumArray<Legacy, int>::size() == 2);
  static_assert(EnumArray<Fruit, int, 5>::size() == 5);
}

TEST(EnumArrayTest, IsAConstantExpressionAggregate) {
  static constexpr EnumArray<Fruit, const char*> kNames = {"apple", "pear", "plum"};
  static_assert(kNames.at(Fruit::kPear)[0] == 'p');
  static_assert(decltype(kNames)::size() == 3);
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
  EXPECT_EQ(seen, decltype(values)::size());
  EXPECT_EQ(values.end() - values.begin(), 3);
  EXPECT_EQ(values.data(), values.begin());
}

TEST(EnumArrayTest, NestsForTwoDimensionalTables) {
  constexpr EnumArray<Fruit, EnumArray<Legacy, int>> table = {{{1, 2}, {3, 4}, {5, 6}}};
  static_assert(table.at(Fruit::kPlum).at(Legacy::kB) == 6);
  EXPECT_EQ(table.at(Fruit::kPear).at(Legacy::kA), 3);
}

TEST(EnumArrayTest, ValueInitializesToZero) {
  const EnumArray<Fruit, int> zeros{};
  EXPECT_EQ(zeros.at(Fruit::kApple), 0);
  EXPECT_EQ(zeros.at(Fruit::kPlum), 0);
}

}  // namespace
}  // namespace base
