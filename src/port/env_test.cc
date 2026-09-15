#include "port/env.h"

#include <cstdlib>

#include "gtest/gtest.h"

namespace {

// setenv and unsetenv are POSIX additions that include-cleaner does not map
// to <cstdlib>.
void SetVar(const char* name, const char* value) {
  ASSERT_EQ(setenv(name, value, 1), 0);  // NOLINT(misc-include-cleaner)
}

void UnsetVar(const char* name) {
  ASSERT_EQ(unsetenv(name), 0);  // NOLINT(misc-include-cleaner)
}

TEST(EnvTest, ReturnsValueEmptyValueAndUnsetDistinctly) {
  SetVar("CNC_ENV_TEST_VALUE", "abc");
  SetVar("CNC_ENV_TEST_EMPTY", "");
  UnsetVar("CNC_ENV_TEST_UNSET");
  EXPECT_EQ(port::GetEnv("CNC_ENV_TEST_VALUE"), "abc");
  EXPECT_EQ(port::GetEnv("CNC_ENV_TEST_EMPTY"), "");
  EXPECT_FALSE(port::GetEnv("CNC_ENV_TEST_UNSET").has_value());
}

TEST(EnvTest, CopyOutlivesLaterChanges) {
  SetVar("CNC_ENV_TEST_COPY", "first");
  const auto copy = port::GetEnv("CNC_ENV_TEST_COPY");
  SetVar("CNC_ENV_TEST_COPY", "second");
  EXPECT_EQ(copy, "first");
}

}  // namespace
