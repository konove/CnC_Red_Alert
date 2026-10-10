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

// A stage that advances one step on every Graphic_Logic() call while its
// rate is nonzero, like the games' StageClass with a rate of 1.
class FakeStage {
 public:
  bool Graphic_Logic() {
    if (rate_ == 0) {
      return false;
    }
    ++stage_;
    return true;
  }
  [[nodiscard]] int Fetch_Stage() const { return stage_; }
  void Set_Stage(int stage) { stage_ = stage; }
  void Set_Rate(int rate) { rate_ = rate; }

  template <class Archive>
  void Serialize(Archive& ar) {
    ar(stage_, rate_);
  }

 private:
  int32_t stage_ = 0;
  int32_t rate_ = 0;
};

using TestDoor = Door<FakeStage>;

TEST(DoorTest, OpensThroughEveryStageThenStops) {
  TestDoor door;
  EXPECT_TRUE(door.IsDoorClosed());
  ASSERT_TRUE(door.OpenDoor(1, 4));
  EXPECT_TRUE(door.IsDoorOpening());
  EXPECT_FALSE(door.OpenDoor(1, 4));  // Already opening.
  // Four stages are shown as 0 to 3 while opening; the last frame of the
  // animation finishes it, and an open door shows its last stage less one,
  // as the originals did.
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

TEST(DoorTest, ClosingCountsTheStagesDown) {
  TestDoor door;
  door.OpenDoor(1, 3);
  door.AI();
  door.AI();
  ASSERT_TRUE(door.IsDoorOpen());
  ASSERT_TRUE(door.CloseDoor(1, 3));
  EXPECT_EQ(door.DoorFrame(), 1);
  door.AI();
  EXPECT_EQ(door.DoorFrame(), 0);
  EXPECT_TRUE(door.IsDoorClosing());
  door.AI();
  EXPECT_TRUE(door.IsDoorClosed());
  EXPECT_EQ(door.DoorFrame(), 0);
  EXPECT_FALSE(door.CloseDoor(1, 3));  // Already closed.
}

TEST(DoorTest, SerializeRoundTripsTheAnimation) {
  TestDoor door;
  door.OpenDoor(1, 5);
  door.AI();
  std::array<std::byte, 64> bytes{};
  SpanSink sink(bytes);
  ArchiveWriter writer(sink);
  door.Serialize(writer);
  ASSERT_TRUE(writer.ok());

  TestDoor loaded;
  SpanSource source{std::span<const std::byte>(bytes)};
  ArchiveReader reader(source);
  loaded.Serialize(reader);
  ASSERT_TRUE(reader.ok());
  EXPECT_TRUE(loaded.IsDoorOpening());
  EXPECT_EQ(loaded.DoorFrame(), 1);
  EXPECT_TRUE(loaded.DoorNeedsRedraw());
}

}  // namespace

}  // namespace game
