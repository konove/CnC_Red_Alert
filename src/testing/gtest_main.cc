// File: the main() linked into every test executable (see cmake/Testing.cmake).
// Like gtest_main, but death tests do not leave a core dump behind.

#include "gtest/gtest.h"

#ifdef __linux__
#include <linux/prctl.h>
#include <sys/prctl.h>

#include <cstdlib>
#endif

int main(int argc, char** argv) {
#ifdef __linux__
  // A death test aborts a forked child, and systemd-coredump writes a dump
  // and a stack trace for each one regardless of ulimit -c. Marking the
  // process undumpable (inherited by the fork) stops that; set
  // CNC_TEST_CORE_DUMPS=1 to get the dumps back when debugging a crash.
  if (std::getenv("CNC_TEST_CORE_DUMPS") == nullptr) {
    prctl(PR_SET_DUMPABLE, 0);
  }
#endif
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
