// Tests the helpers in ra/core/inline.h against a real Game with no game data.

#include "ra/core/inline.h"

#include "gtest/gtest.h"
#include "ra/map/world.h"
#include "ra/startup/game_test_util.h"

namespace ra {
namespace {

using InlineTest = GameFixture;

TEST_F(InlineTest, TextStringServesEveryNameOverrideSlot) {
  auto& overrides = TheWorld().name_override();
  overrides.front() = "First";
  overrides.back() = "Last";

  EXPECT_STREQ(TextString(-1), "First");
  EXPECT_STREQ(TextString(-static_cast<int>(overrides.size())), "Last");
}

}  // namespace
}  // namespace ra
