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
  EXPECT_TRUE(door.IsDoorClosed());
  ASSERT_TRUE(door.OpenDoor(1, 4));
  EXPECT_TRUE(door.IsDoorOpening());
  EXPECT_FALSE(door.OpenDoor(1, 4));  // Already opening.
  // A count of four stages shows frames 0, 1 and 2: the third step finishes
  // the opening, and the open door stays on frame 2, as the originals did.
  for (int frame = 1; frame <= 2; ++frame) {
    door.AI();
    EXPECT_EQ(door.DoorFrame(), frame);
    EXPECT_TRUE(door.IsDoorOpening());
  }
  door.AI();
  EXPECT_TRUE(door.IsDoorOpen());
  EXPECT_EQ(door.DoorFrame(), 2);
  EXPECT_TRUE(door.DoorNeedsRedraw());
  door.ClearDoorRedraw();
  door.AI();  // The rate dropped to 0 when the door finished.
  EXPECT_FALSE(door.DoorNeedsRedraw());
  EXPECT_EQ(door.DoorFrame(), 2);
}

TEST(DoorTest, AStageLastsFramesPerStageCalls) {
  Door door;
  ASSERT_TRUE(door.OpenDoor(3, 3));
  for (int stage = 1; stage <= 2; ++stage) {
    door.AI();
    door.AI();
    EXPECT_EQ(door.DoorFrame(), stage - 1);
    EXPECT_FALSE(door.DoorNeedsRedraw());
    door.AI();
    EXPECT_TRUE(door.DoorNeedsRedraw());
    door.ClearDoorRedraw();
  }
  EXPECT_TRUE(door.IsDoorOpen());
  EXPECT_EQ(door.DoorFrame(), 1);
}

TEST(DoorTest, ClosingCountsTheStagesDown) {
  Door door;
  door.OpenDoor(1, 3);
  door.AI();
  door.AI();
  ASSERT_TRUE(door.IsDoorOpen());
  ASSERT_TRUE(door.CloseDoor(1, 3));
  EXPECT_EQ(door.DoorFrame(), 1);
  door.AI();
  EXPECT_EQ(door.DoorFrame(), 0);
  EXPECT_FALSE(door.IsDoorClosed());
  door.AI();
  EXPECT_TRUE(door.IsDoorClosed());
  EXPECT_EQ(door.DoorFrame(), 0);
  EXPECT_FALSE(door.CloseDoor(1, 3));  // Already closed.
}

TEST(DoorTest, TurningRoundRestartsFromTheEndFrame) {
  Door door;
  door.OpenDoor(1, 4);
  door.AI();
  ASSERT_EQ(door.DoorFrame(), 1);
  // Closing a door that is only part-way open starts from the fully open
  // frame, not from the one it had reached.
  ASSERT_TRUE(door.CloseDoor(1, 4));
  EXPECT_FALSE(door.IsDoorOpening());
  EXPECT_EQ(door.DoorFrame(), 2);
  EXPECT_FALSE(door.CloseDoor(1, 4));  // Already closing.
  door.AI();
  EXPECT_EQ(door.DoorFrame(), 1);
  // And opening one that is part-way closed starts from the closed frame.
  ASSERT_TRUE(door.OpenDoor(1, 4));
  EXPECT_TRUE(door.IsDoorOpening());
  EXPECT_EQ(door.DoorFrame(), 0);
  for (int step = 0; step < 3; ++step) {
    door.AI();
  }
  EXPECT_TRUE(door.IsDoorOpen());
}

TEST(DoorTest, SerializeRoundTripsTheAnimation) {
  Door door;
  door.OpenDoor(2, 5);
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
  EXPECT_TRUE(loaded.IsDoorOpening());
  EXPECT_EQ(loaded.DoorFrame(), 1);
  EXPECT_TRUE(loaded.DoorNeedsRedraw());
  // The loaded door is half-way through its second stage.
  loaded.AI();
  EXPECT_EQ(loaded.DoorFrame(), 2);
}

}  // namespace

}  // namespace game
