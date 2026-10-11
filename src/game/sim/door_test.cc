#include "game/sim/door.h"

#include <array>
#include <cstddef>
#include <span>

#include "engine/stream/archive.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
#include "gtest/gtest.h"

namespace game {

namespace {

TEST(DoorTest, OpensThroughEveryStageThenStops) {
  Door door;
  EXPECT_TRUE(door.IsClosed());
  ASSERT_TRUE(door.Open(1, 3));
  EXPECT_TRUE(door.IsOpening());
  EXPECT_FALSE(door.Open(1, 3));  // Already opening.
  // The third step finishes the opening, and the open door stays on frame 2.
  for (int frame = 1; frame <= 2; ++frame) {
    door.AI();
    EXPECT_EQ(door.Frame(), frame);
    EXPECT_TRUE(door.IsOpening());
  }
  EXPECT_TRUE(door.AI());
  EXPECT_TRUE(door.IsOpen());
  EXPECT_EQ(door.Frame(), 2);
  EXPECT_FALSE(door.AI());  // An open door has stopped.
  EXPECT_EQ(door.Frame(), 2);
}

TEST(DoorTest, AStageLastsFramesPerStageCalls) {
  Door door;
  ASSERT_TRUE(door.Open(3, 2));
  for (int stage = 1; stage <= 2; ++stage) {
    EXPECT_FALSE(door.AI());
    EXPECT_FALSE(door.AI());
    EXPECT_EQ(door.Frame(), stage - 1);
    EXPECT_TRUE(door.AI());
  }
  EXPECT_TRUE(door.IsOpen());
  EXPECT_EQ(door.Frame(), 1);
}

TEST(DoorTest, ClosingCountsTheStagesDown) {
  Door door;
  door.Open(1, 2);
  door.AI();
  door.AI();
  ASSERT_TRUE(door.IsOpen());
  ASSERT_TRUE(door.Close(1, 2));
  EXPECT_EQ(door.Frame(), 1);
  door.AI();
  EXPECT_EQ(door.Frame(), 0);
  EXPECT_FALSE(door.IsClosed());
  door.AI();
  EXPECT_TRUE(door.IsClosed());
  EXPECT_EQ(door.Frame(), 0);
  EXPECT_FALSE(door.Close(1, 2));  // Already closed.
}

TEST(DoorTest, TurningRoundCarriesOnFromTheFrameItShows) {
  Door door;
  door.Open(1, 4);
  door.AI();
  ASSERT_EQ(door.Frame(), 1);
  ASSERT_TRUE(door.Close(1, 4));
  EXPECT_FALSE(door.IsOpening());
  EXPECT_EQ(door.Frame(), 1);
  EXPECT_FALSE(door.Close(1, 4));  // Already closing.
  door.AI();
  EXPECT_EQ(door.Frame(), 0);
  ASSERT_TRUE(door.Open(1, 4));
  EXPECT_TRUE(door.IsOpening());
  EXPECT_EQ(door.Frame(), 0);
  door.AI();
  door.AI();
  ASSERT_EQ(door.Frame(), 2);
  // Frames 2, 1 and 0 are left to close: a stage each, where a whole
  // animation takes four.
  ASSERT_TRUE(door.Close(1, 4));
  for (int frame = 1; frame >= 0; --frame) {
    door.AI();
    EXPECT_EQ(door.Frame(), frame);
    EXPECT_FALSE(door.IsClosed());
  }
  door.AI();
  EXPECT_TRUE(door.IsClosed());
}

TEST(DoorTest, TurningRoundWithFewerFramesStaysWithinThem) {
  Door door;
  door.Open(1, 5);
  for (int step = 0; step < 5; ++step) {
    door.AI();
  }
  ASSERT_TRUE(door.IsOpen());
  ASSERT_EQ(door.Frame(), 4);
  ASSERT_TRUE(door.Close(1, 2));
  EXPECT_EQ(door.Frame(), 1);
  door.AI();
  ASSERT_EQ(door.Frame(), 0);
  // And with more frames it carries on from the one it shows.
  ASSERT_TRUE(door.Open(1, 5));
  EXPECT_EQ(door.Frame(), 0);
}

TEST(DoorTest, SerializeRoundTripsTheAnimation) {
  Door door;
  door.Open(2, 4);
  for (int call = 0; call < 3; ++call) {
    door.AI();
  }
  std::array<std::byte, 64> bytes{};
  SpanSink sink(bytes);
  ArchiveWriter writer(sink);
  door.Serialize(writer);
  ASSERT_TRUE(writer.ok());

  Door loaded;
  SpanSource source{std::span<const std::byte>(bytes)};
  ArchiveReader reader(source);
  loaded.Serialize(reader);
  ASSERT_TRUE(reader.ok());
  EXPECT_TRUE(loaded.IsOpening());
  EXPECT_EQ(loaded.Frame(), 1);
  // The loaded door is half-way through its second stage.
  EXPECT_TRUE(loaded.AI());
  EXPECT_EQ(loaded.Frame(), 2);
}

}  // namespace

}  // namespace game
