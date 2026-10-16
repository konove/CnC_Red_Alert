#include "game/sim/stage_counter.h"

#include <array>
#include <cstddef>
#include <span>

#include "base/installed.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
#include "game/core/game_clock.h"
#include "gtest/gtest.h"

namespace game {
namespace {

class StageCounterTest : public testing::Test {
 protected:
  // Moves the game on `frames` frames without calling AI().
  void Skip(const int frames) { clock_.set_frame(clock_.frame() + frames); }

  // Moves the game on a frame and runs the counter's frame logic.
  bool Tick(StageCounter& counter) {
    clock_.Advance();
    return counter.AI();
  }

 private:
  GameClock clock_;
  base::Installed<GameClock>::Scope clock_scope_{clock_};
};

TEST_F(StageCounterTest, ANewCounterStandsStill) {
  StageCounter counter;
  EXPECT_EQ(counter.stage(), 0);
  EXPECT_EQ(counter.frames_per_stage(), 0);
  for (int frame = 0; frame < 5; ++frame) {
    EXPECT_FALSE(Tick(counter));
  }
  EXPECT_EQ(counter.stage(), 0);
}

TEST_F(StageCounterTest, MovesOnAStageEveryFramesPerStage) {
  StageCounter counter;
  counter.set_stage(7);
  counter.SetFramesPerStage(3);
  EXPECT_EQ(counter.frames_per_stage(), 3);
  for (int stage = 8; stage <= 9; ++stage) {
    EXPECT_FALSE(Tick(counter));
    EXPECT_FALSE(Tick(counter));
    EXPECT_TRUE(Tick(counter));
    EXPECT_EQ(counter.stage(), stage);
  }
}

TEST_F(StageCounterTest, KeepsTimeThroughFramesWithoutItsLogic) {
  StageCounter counter;
  counter.SetFramesPerStage(4);
  Skip(10);
  // However late, the stage moves on by one and the countdown starts anew.
  EXPECT_TRUE(counter.AI());
  EXPECT_EQ(counter.stage(), 1);
  EXPECT_FALSE(counter.AI());
  Skip(3);
  EXPECT_FALSE(counter.AI());
  EXPECT_TRUE(Tick(counter));
  EXPECT_EQ(counter.stage(), 2);
}

TEST_F(StageCounterTest, SettingFramesPerStageRestartsTheCountdown) {
  StageCounter counter;
  counter.SetFramesPerStage(2);
  EXPECT_FALSE(Tick(counter));
  counter.SetFramesPerStage(2);
  EXPECT_FALSE(Tick(counter));
  EXPECT_TRUE(Tick(counter));
  counter.SetFramesPerStage(0);
  Skip(100);
  EXPECT_FALSE(counter.AI());
  EXPECT_EQ(counter.stage(), 1);
}

TEST_F(StageCounterTest, ASavedCounterCarriesOnWhereItWas) {
  StageCounter counter;
  counter.set_stage(12);
  counter.SetFramesPerStage(3);
  EXPECT_FALSE(Tick(counter));
  std::array<std::byte, 64> bytes{};
  SpanSink sink(bytes);
  ArchiveWriter writer(sink);
  counter.Serialize(writer);
  ASSERT_TRUE(writer.ok());

  // The game that loads it is at another frame.
  Skip(1000);
  StageCounter loaded;
  SpanSource source{std::span<const std::byte>(bytes)};
  ArchiveReader reader(source);
  loaded.Serialize(reader);
  ASSERT_TRUE(reader.ok());
  EXPECT_EQ(loaded.stage(), 12);
  EXPECT_EQ(loaded.frames_per_stage(), 3);
  EXPECT_FALSE(Tick(loaded));
  EXPECT_TRUE(Tick(loaded));
  EXPECT_EQ(loaded.stage(), 13);
}

}  // namespace
}  // namespace game
