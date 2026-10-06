// Tests for the accessor of the installed special flags.

#include "ra/core/special_flags.h"

#include "base/installed.h"
#include "gtest/gtest.h"

namespace ra {

namespace {

TEST(SpecialFlagsTest, TheSpecialFlagsIsTheInstalledInstance) {
  SpecialFlags flags{};
  const base::Installed<SpecialFlags>::Scope scope(flags);

  TheSpecialFlags().inert_weapons = true;

  EXPECT_EQ(&TheSpecialFlags(), &flags);
  EXPECT_TRUE(flags.inert_weapons);
}

}  // namespace

}  // namespace ra
