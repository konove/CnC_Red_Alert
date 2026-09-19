#include "port/env.h"

#include <cstdlib>

#include "gtest/gtest.h"

namespace {

// setenv and unsetenv are POSIX additions that include-cleaner does not map
// to <cstdlib>, and they are the environment writes GetEnv's copy guards
// against; a single-threaded test may call them. Windows has _putenv_s
// instead, which removes a variable set to "".
void SetVar(const char* name, const char* value) {
#ifdef _WIN32
  ASSERT_EQ(_putenv_s(name, value), 0);
#else
  // NOLINTNEXTLINE(misc-include-cleaner,concurrency-mt-unsafe)
  ASSERT_EQ(setenv(name, value, 1), 0);
#endif
}

void UnsetVar(const char* name) {
#ifdef _WIN32
  ASSERT_EQ(_putenv_s(name, ""), 0);
#else
  // NOLINTNEXTLINE(misc-include-cleaner,concurrency-mt-unsafe)
  ASSERT_EQ(unsetenv(name), 0);
#endif
}

TEST(EnvTest, ReturnsValueEmptyValueAndUnsetDistinctly) {
  SetVar("CNC_ENV_TEST_VALUE", "abc");
  UnsetVar("CNC_ENV_TEST_UNSET");
  EXPECT_EQ(port::GetEnv("CNC_ENV_TEST_VALUE"), "abc");
#ifndef _WIN32
  SetVar("CNC_ENV_TEST_EMPTY", "");
  EXPECT_EQ(port::GetEnv("CNC_ENV_TEST_EMPTY"), "");
#endif
  EXPECT_FALSE(port::GetEnv("CNC_ENV_TEST_UNSET").has_value());
}

TEST(EnvTest, CopyOutlivesLaterChanges) {
  SetVar("CNC_ENV_TEST_COPY", "first");
  const auto copy = port::GetEnv("CNC_ENV_TEST_COPY");
  SetVar("CNC_ENV_TEST_COPY", "second");
  EXPECT_EQ(copy, "first");
}

}  // namespace
