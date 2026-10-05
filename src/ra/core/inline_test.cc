// Tests the text table lookup against a real Game with no game data.

#include "ra/core/inline.h"

#include "gtest/gtest.h"
#include "ra/data/text_ids.h"
#include "ra/map/world.h"
#include "ra/startup/game_test_util.h"

namespace ra {
namespace {

using TextStringTest = GameFixture;

TEST_F(TextStringTest, ServesEveryNameOverrideSlot) {
  auto& overrides = TheWorld().name_override();
  overrides.front() = "First";
  overrides.back() = "Last";

  EXPECT_STREQ(TextString(-1), "First");
  EXPECT_STREQ(TextString(-static_cast<int>(overrides.size())), "Last");
}

TEST_F(TextStringTest, FindsNothingBelowTheOverrideSlots) {
  const int slots = static_cast<int>(TheWorld().name_override().size());

  EXPECT_EQ(TextString(-slots - 1), nullptr);
}

// No archive is mounted, so both text tables are empty.
TEST_F(TextStringTest, FindsNothingInAnEmptyTable) {
  EXPECT_EQ(TextString(TXT_NONE), nullptr);
  EXPECT_EQ(TextString(999), nullptr);
  EXPECT_EQ(TextString(1000), nullptr);
}

}  // namespace
}  // namespace ra
