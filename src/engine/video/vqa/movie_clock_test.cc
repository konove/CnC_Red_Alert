#include "engine/video/vqa/movie_clock.h"

#include "engine/platform/manual_clock.h"
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

// Without sound the clock is the game's, so it stands still with a manual
// clock and moves when that does: 60 movie ticks a second.
TEST(MovieClockTest, ASilentMovieRunsOnTheGamesClock) {
  engine::platform::ManualClock game_clock;
  MovieClock clock;
  clock.Set(100, nullptr);

  EXPECT_EQ(clock.Now(), 100);
  game_clock.Advance(30);
  EXPECT_EQ(clock.Now(), 130);
}

}  // namespace
