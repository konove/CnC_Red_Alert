// Tests installing, fetching and restoring instances with base::Installed.

#include "engine/base/installed.h"

#include "gtest/gtest.h"

namespace {

struct Widget {
  int value = 0;
};

// A second type, so the tests can show each T has its own slot.
struct Gadget {
  int value = 0;
};

TEST(InstalledTest, NothingIsInstalledByDefault) {
  EXPECT_FALSE(base::Installed<Widget>::IsInstalled());
}

TEST(InstalledTest, ScopeInstallsForItsLifetime) {
  Widget widget{.value = 7};
  {
    const base::Installed<Widget>::Scope scope(widget);
    EXPECT_TRUE(base::Installed<Widget>::IsInstalled());
    EXPECT_EQ(&base::Installed<Widget>::Get(), &widget);
    EXPECT_EQ(base::Installed<Widget>::Get().value, 7);
  }
  EXPECT_FALSE(base::Installed<Widget>::IsInstalled());
}

TEST(InstalledTest, NestedScopeRestoresThePreviousInstance) {
  Widget outer{.value = 1};
  Widget inner{.value = 2};
  const base::Installed<Widget>::Scope outer_scope(outer);
  {
    const base::Installed<Widget>::Scope inner_scope(inner);
    EXPECT_EQ(&base::Installed<Widget>::Get(), &inner);
  }
  EXPECT_EQ(&base::Installed<Widget>::Get(), &outer);
}

TEST(InstalledTest, EachTypeHasItsOwnInstance) {
  Widget widget;
  const base::Installed<Widget>::Scope scope(widget);
  EXPECT_TRUE(base::Installed<Widget>::IsInstalled());
  EXPECT_FALSE(base::Installed<Gadget>::IsInstalled());
}

TEST(InstalledTest, GetWritesThroughToTheInstance) {
  Widget widget;
  const base::Installed<Widget>::Scope scope(widget);
  base::Installed<Widget>::Get().value = 42;
  EXPECT_EQ(widget.value, 42);
}

// GoogleTest's death-test macro formats subprocess diagnostics with libc.
// NOLINTBEGIN(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
TEST(InstalledDeathTest, GetWithNothingInstalledDies) {
  EXPECT_DEATH(base::Installed<Widget>::Get(), "nothing is installed");
}

TEST(InstalledDeathTest, EndingScopesOutOfOrderDies) {
  EXPECT_DEATH(
      {
        Widget first;
        Widget second;
        auto* const outer = new base::Installed<Widget>::Scope(first);
        const base::Installed<Widget>::Scope inner(second);
        delete outer;
      },
      "out of order");
}
// NOLINTEND(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)

}  // namespace
