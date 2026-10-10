#include "game/sim/flasher.h"

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

TEST(FlasherTest, DoesNothingUntilStarted) {
  FlasherClass flasher;
  EXPECT_FALSE(flasher.Process());
  EXPECT_FALSE(flasher.IsBrightened());
}

TEST(FlasherTest, ReportsAChangeOnAsManyFramesAsTheCount) {
  FlasherClass flasher;
  flasher.Start(4);
  // The call that takes the count to zero still reports a change: the object
  // has to be drawn once more, not brightened.
  for (int frame = 0; frame < 4; ++frame) {
    EXPECT_TRUE(flasher.Process()) << frame;
  }
  EXPECT_FALSE(flasher.Process());
  EXPECT_FALSE(flasher.IsBrightened());
}

TEST(FlasherTest, IsBrightenedOnEveryOtherFrame) {
  FlasherClass flasher;
  flasher.Start(5);
  EXPECT_FALSE(flasher.IsBrightened());  // Not before the first Process().
  const std::array<bool, 5> expected{false, true, false, true, false};
  for (const bool brightened : expected) {
    flasher.Process();
    EXPECT_EQ(flasher.IsBrightened(), brightened);
  }
}

TEST(FlasherTest, StartingAgainReplacesTheCount) {
  FlasherClass flasher;
  flasher.Start(7);
  flasher.Process();
  flasher.Start(2);
  EXPECT_TRUE(flasher.Process());
  EXPECT_TRUE(flasher.IsBrightened());
  EXPECT_TRUE(flasher.Process());
  EXPECT_FALSE(flasher.Process());
}

TEST(FlasherTest, KeepsTheLowSevenBitsOfTheCount) {
  FlasherClass flasher;
  flasher.Start(128 + 1);
  EXPECT_TRUE(flasher.Process());
  EXPECT_FALSE(flasher.Process());
}

TEST(FlasherTest, ASavedFlasherCarriesOnWhereItWas) {
  FlasherClass flasher;
  flasher.Start(4);
  flasher.Process();
  ASSERT_TRUE(flasher.IsBrightened());

  std::array<std::byte, 2> bytes{};
  SpanSink sink(bytes);
  ArchiveWriter writer(sink);
  writer(flasher);
  // The count, then the brightened flag.
  EXPECT_EQ(bytes, (std::array{std::byte{3}, std::byte{1}}));

  FlasherClass loaded;
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

TEST(FlasherTest, ACountThatDoesNotFitFailsTheLoad) {
  const std::array<uint8_t, 2> bytes{128, 0};
  SpanSource source(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(source);
  FlasherClass flasher;
  reader(flasher);
  EXPECT_FALSE(reader.ok());
}

}  // namespace

}  // namespace game
