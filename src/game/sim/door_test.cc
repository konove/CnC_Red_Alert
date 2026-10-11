#include "game/sim/door.h"

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

// Loads a door from the five numbers a saved game holds for one.
Door LoadDoor(int32_t stage, int32_t countdown, int32_t frames_per_stage,
              int32_t frame_count, int32_t state) {
  std::array<std::byte, 64> bytes{};
  SpanSink sink(bytes);
  ArchiveWriter writer(sink);
  writer(stage, countdown, frames_per_stage, frame_count, state);
  EXPECT_TRUE(writer.ok());
  Door door;
  SpanSource source{std::span<const std::byte>(bytes)};
  ArchiveReader reader(source);
  door.Serialize(reader);
  EXPECT_TRUE(reader.ok());
  return door;
}

constexpr int32_t kClosed = 0;
constexpr int32_t kOpening = 1;
constexpr int32_t kOpen = 2;
constexpr int32_t kClosing = 3;

void ExpectClosedAndStill(Door door) {
  EXPECT_TRUE(door.IsClosed());
  EXPECT_EQ(door.Frame(), 0);
  EXPECT_FALSE(door.AI());
  EXPECT_TRUE(door.IsClosed());
  // And it opens like a new door.
  ASSERT_TRUE(door.Open(1, 2));
  EXPECT_TRUE(door.AI());
  EXPECT_EQ(door.Frame(), 1);
}

TEST(DoorTest, LoadsTheStatesADoorCanHave) {
  Door closing = LoadDoor(1, 2, 2, 3, kClosing);
  EXPECT_EQ(closing.Frame(), 1);
  EXPECT_FALSE(closing.AI());
  EXPECT_TRUE(closing.AI());
  EXPECT_EQ(closing.Frame(), 0);

  // What the last animation left in an open or closed door does not move it.
  Door open = LoadDoor(3, 2, 2, 3, kOpen);
  EXPECT_TRUE(open.IsOpen());
  EXPECT_EQ(open.Frame(), 2);
  EXPECT_FALSE(open.AI());
  EXPECT_FALSE(open.AI());
  EXPECT_TRUE(open.IsOpen());
  ExpectClosedAndStill(LoadDoor(3, 1, 1, 3, kClosed));
}

TEST(DoorTest, ADoorLoadedWithAnUnknownStateIsClosed) {
  ExpectClosedAndStill(LoadDoor(1, 2, 2, 3, 4));
  ExpectClosedAndStill(LoadDoor(1, 2, 2, 3, -1));
}

TEST(DoorTest, AnOpenDoorLoadedWithoutFramesIsClosed) {
  ExpectClosedAndStill(LoadDoor(0, 0, 0, 0, kOpen));
}

TEST(DoorTest, AMovingDoorLoadedWithImpossibleCountersIsClosed) {
  ExpectClosedAndStill(LoadDoor(0, 1, 1, 0, kOpening));   // No frames.
  ExpectClosedAndStill(LoadDoor(-1, 1, 1, 3, kOpening));  // Before the first.
  ExpectClosedAndStill(LoadDoor(3, 1, 1, 3, kClosing));   // Past the last.
  ExpectClosedAndStill(LoadDoor(1, 1, 0, 3, kOpening));   // Would never end.
  ExpectClosedAndStill(LoadDoor(1, 0, 2, 3, kClosing));   // Stage overdue.
  ExpectClosedAndStill(LoadDoor(1, 3, 2, 3, kOpening));   // Stage too long.
}

}  // namespace

}  // namespace game
