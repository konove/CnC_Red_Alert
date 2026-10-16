#include "game/sim/target_flash.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "engine/stream/archive.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
#include "gtest/gtest.h"

namespace game {

namespace {

TEST(TargetFlashTest, DoesNothingUntilStarted) {
  TargetFlash flash;
  EXPECT_FALSE(flash.Process());
  EXPECT_FALSE(flash.IsBrightened());
}

TEST(TargetFlashTest, ReportsAChangeOnAsManyFramesAsTheCount) {
  TargetFlash flash;
  flash.Start(4);
  // The call that takes the count to zero still reports a change: the object
  // has to be drawn once more, not brightened.
  for (int frame = 0; frame < 4; ++frame) {
    EXPECT_TRUE(flash.Process()) << frame;
  }
  EXPECT_FALSE(flash.Process());
  EXPECT_FALSE(flash.IsBrightened());
}

TEST(TargetFlashTest, IsBrightenedOnEveryOtherFrame) {
  TargetFlash flash;
  flash.Start(5);
  // An odd count is brightened at once, before the first Process().
  EXPECT_TRUE(flash.IsBrightened());
  constexpr std::array<bool, 5> expected{false, true, false, true, false};
  for (const bool brightened : expected) {
    flash.Process();
    EXPECT_EQ(flash.IsBrightened(), brightened);
  }
}

TEST(TargetFlashTest, StartingAgainReplacesTheCount) {
  TargetFlash flash;
  flash.Start(7);
  flash.Process();
  flash.Start(2);
  EXPECT_TRUE(flash.Process());
  EXPECT_TRUE(flash.IsBrightened());
  EXPECT_TRUE(flash.Process());
  EXPECT_FALSE(flash.Process());
}

TEST(TargetFlashTest, CutsALongCountToTheLongestFlash) {
  TargetFlash flash;
  // A count past 127 used to wrap: 129 flashed once.
  flash.Start(128 + 1);
  for (int frame = 0; frame < 127; ++frame) {
    EXPECT_TRUE(flash.Process()) << frame;
  }
  EXPECT_FALSE(flash.Process());
}

TEST(TargetFlashTest, StartingWithNothingStopsABrightenedFlash) {
  TargetFlash flash;
  flash.Start(4);
  flash.Process();
  ASSERT_TRUE(flash.IsBrightened());
  flash.Start(0);
  EXPECT_FALSE(flash.IsBrightened());
  EXPECT_FALSE(flash.Process());
  EXPECT_FALSE(flash.IsBrightened());
}

TEST(TargetFlashTest, ASavedFlashCarriesOnWhereItWas) {
  TargetFlash flash;
  flash.Start(4);
  flash.Process();
  ASSERT_TRUE(flash.IsBrightened());

  std::array<std::byte, 2> bytes{};
  SpanSink sink(bytes);
  ArchiveWriter writer(sink);
  writer(flash);
  // The count, then the brightened flag.
  EXPECT_EQ(bytes, (std::array{std::byte{3}, std::byte{1}}));

  TargetFlash loaded;
  SpanSource source(bytes);
  ArchiveReader reader(source);
  reader(loaded);
  ASSERT_TRUE(reader.ok());
  EXPECT_TRUE(loaded.IsBrightened());
  EXPECT_TRUE(loaded.Process());
  EXPECT_FALSE(loaded.IsBrightened());
  EXPECT_TRUE(loaded.Process());
  EXPECT_TRUE(loaded.Process());
  EXPECT_FALSE(loaded.Process());
}

TEST(TargetFlashTest, ALoadIgnoresAFlagSetWithNoFramesLeft) {
  // Before Start(0) cleared the flag, a count that wrapped to 0 on a
  // brightened frame left it set, and a save made then holds this pair.
  constexpr std::array<uint8_t, 2> bytes{0, 1};
  SpanSource source(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(source);
  TargetFlash flash;
  reader(flash);
  ASSERT_TRUE(reader.ok());
  EXPECT_FALSE(flash.IsBrightened());
  EXPECT_FALSE(flash.Process());
}

TEST(TargetFlashTest, ACountThatDoesNotFitFailsTheLoadAndStopsTheFlash) {
  constexpr std::array<uint8_t, 2> bytes{128, 1};
  SpanSource source(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(source);
  TargetFlash flash;
  flash.Start(4);
  reader(flash);
  EXPECT_FALSE(reader.ok());
  EXPECT_FALSE(flash.IsBrightened());
  EXPECT_FALSE(flash.Process());
}

}  // namespace

}  // namespace game
