#include "winvq/vqa32/movie_clock.h"

#include "gtest/gtest.h"

namespace {

TEST(MovieClockTest, ReadsWhatItWasSetTo) {
  MovieClock clock;
  clock.Set(600, nullptr);

  // The system clock runs on, but not by a second within the test.
  EXPECT_GE(clock.Now(), 600);
  EXPECT_LT(clock.Now(), 660);
}

TEST(MovieClockTest, EveryMovieHasItsOwnClock) {
  MovieClock first;
  MovieClock second;
  first.Set(0, nullptr);
  second.Set(6000, nullptr);

  EXPECT_LT(first.Now(), 60);
  EXPECT_GE(second.Now(), 6000);
}

}  // namespace
