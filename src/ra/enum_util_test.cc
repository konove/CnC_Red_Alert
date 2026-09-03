#include "ra/enum_util.h"

#include <gtest/gtest.h>

#include "ra/defines.h"

// The kXxxCount constants in defines.h are plain numbers so that headers pay
// nothing for them; this is where they are checked against the enums.
static_assert(kEnumCount<DiffType> == kDiffCount);

TEST(EnumUtil, EnumValuesListsTheIndexValuesInOrder) {
  constexpr auto values = EnumValues<DiffType>();
  ASSERT_EQ(values.size(), 3u);
  EXPECT_EQ(values[0], DIFF_EASY);
  EXPECT_EQ(values[1], DIFF_NORMAL);
  EXPECT_EQ(values[2], DIFF_HARD);
}
