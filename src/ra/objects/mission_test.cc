#include "ra/objects/mission.h"

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "base/installed.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
#include "game/core/game_clock.h"
#include "gtest/gtest.h"
#include "ra/core/defines.h"

namespace ra {
namespace {

// Notes which mission function ran and asks to be called again after `delay`
// frames.
class FakeHandler : public MissionHandler {
 public:
  int Mission_Sleep() override { return Called("Sleep"); }
  int Mission_Ambush() override { return Called("Ambush"); }
  int Mission_Attack() override { return Called("Attack"); }
  int Mission_Capture() override { return Called("Capture"); }
  int Mission_Guard() override { return Called("Guard"); }
  int Mission_Guard_Area() override { return Called("Guard_Area"); }
  int Mission_Harvest() override { return Called("Harvest"); }
  int Mission_Hunt() override { return Called("Hunt"); }
  int Mission_Move() override { return Called("Move"); }
  int Mission_Retreat() override { return Called("Retreat"); }
  int Mission_Return() override { return Called("Return"); }
  int Mission_Stop() override { return Called("Stop"); }
  int Mission_Unload() override { return Called("Unload"); }
  int Mission_Enter() override { return Called("Enter"); }
  int Mission_Construction() override { return Called("Construction"); }
  int Mission_Deconstruction() override { return Called("Deconstruction"); }
  int Mission_Repair() override { return Called("Repair"); }
  int Mission_Missile() override { return Called("Missile"); }

  std::vector<std::string> calls;
  int delay = 0;

 private:
  int Called(const char* name) {
    calls.emplace_back(name);
    return delay;
  }
};

class MissionTest : public testing::Test {
 protected:
  // Starts `mission` the way an object does: queue it, then commence.
  void Start(const MissionType mission) {
    mission_.Assign(mission);
    mission_.Commence();
  }

  void AdvanceFrames(const int frames) {
    clock_.set_frame(clock_.frame() + frames);
  }

  // The mission timer counts game frames.
  game::GameClock clock_;
  base::Installed<game::GameClock>::Scope clock_scope_{clock_};
  MissionClass mission_;
  FakeHandler handler_;
};

TEST_F(MissionTest, StartsWithNoMission) {
  EXPECT_EQ(mission_.current(), MISSION_NONE);
  EXPECT_EQ(mission_.queued(), MISSION_NONE);
  EXPECT_EQ(mission_.suspended(), MISSION_NONE);
  EXPECT_EQ(mission_.CurrentOrQueued(), MISSION_NONE);
  EXPECT_EQ(mission_.step(), 0);
}

TEST_F(MissionTest, AssignQueuesTheMission) {
  mission_.Assign(MISSION_ATTACK);
  EXPECT_EQ(mission_.current(), MISSION_NONE);
  EXPECT_EQ(mission_.queued(), MISSION_ATTACK);
  EXPECT_EQ(mission_.CurrentOrQueued(), MISSION_ATTACK);
}

TEST_F(MissionTest, ALaterAssignReplacesTheQueuedMission) {
  mission_.Assign(MISSION_ATTACK);
  mission_.Assign(MISSION_GUARD);
  EXPECT_EQ(mission_.queued(), MISSION_GUARD);
}

TEST_F(MissionTest, AssigningTheCurrentMissionQueuesNothing) {
  Start(MISSION_GUARD);
  mission_.Assign(MISSION_GUARD);
  EXPECT_EQ(mission_.queued(), MISSION_NONE);
}

TEST_F(MissionTest, AssigningNoMissionKeepsTheQueuedOne) {
  mission_.Assign(MISSION_ATTACK);
  mission_.Assign(MISSION_NONE);
  EXPECT_EQ(mission_.queued(), MISSION_ATTACK);
}

TEST_F(MissionTest, AQueuedMoveIsAssignedAsAMove) {
  mission_.Assign(MISSION_QMOVE);
  EXPECT_EQ(mission_.queued(), MISSION_MOVE);
}

TEST_F(MissionTest, CommenceStartsTheQueuedMissionAtItsFirstStep) {
  mission_.set_step(5);
  mission_.Assign(MISSION_MOVE);

  EXPECT_TRUE(mission_.Commence());

  EXPECT_EQ(mission_.current(), MISSION_MOVE);
  EXPECT_EQ(mission_.queued(), MISSION_NONE);
  EXPECT_EQ(mission_.CurrentOrQueued(), MISSION_MOVE);
  EXPECT_EQ(mission_.step(), 0);
}

TEST_F(MissionTest, CommenceWithNothingQueuedChangesNothing) {
  Start(MISSION_MOVE);
  mission_.set_step(3);

  EXPECT_FALSE(mission_.Commence());

  EXPECT_EQ(mission_.current(), MISSION_MOVE);
  EXPECT_EQ(mission_.step(), 3);
}

TEST_F(MissionTest, SetReplacesTheMissionAndDropsTheQueuedOne) {
  Start(MISSION_MOVE);
  mission_.set_step(3);
  mission_.Assign(MISSION_ATTACK);

  mission_.Set(MISSION_GUARD);

  EXPECT_EQ(mission_.current(), MISSION_GUARD);
  EXPECT_EQ(mission_.queued(), MISSION_NONE);
  EXPECT_EQ(mission_.step(), 3);  // The state machine is not restarted.
}

TEST_F(MissionTest, SuspendSetsAsideTheCurrentMission) {
  Start(MISSION_HARVEST);
  mission_.Suspend();
  EXPECT_EQ(mission_.suspended(), MISSION_HARVEST);
}

TEST_F(MissionTest, SuspendPrefersTheQueuedMission) {
  Start(MISSION_HARVEST);
  mission_.Assign(MISSION_MOVE);
  mission_.Suspend();
  EXPECT_EQ(mission_.suspended(), MISSION_MOVE);
}

TEST_F(MissionTest, AICallsTheFunctionOfTheCurrentMission) {
  const std::vector<std::pair<MissionType, const char*>> expected = {
      {MISSION_NONE, "Sleep"},
      {MISSION_SLEEP, "Sleep"},
      {MISSION_HARMLESS, "Sleep"},
      {MISSION_GUARD, "Guard"},
      {MISSION_STICKY, "Guard"},
      {MISSION_ENTER, "Enter"},
      {MISSION_CONSTRUCTION, "Construction"},
      {MISSION_DECONSTRUCTION, "Deconstruction"},
      {MISSION_CAPTURE, "Capture"},
      {MISSION_SABOTAGE, "Capture"},
      {MISSION_MOVE, "Move"},
      {MISSION_QMOVE, "Move"},
      {MISSION_ATTACK, "Attack"},
      {MISSION_RETREAT, "Retreat"},
      {MISSION_HARVEST, "Harvest"},
      {MISSION_GUARD_AREA, "Guard_Area"},
      {MISSION_RETURN, "Return"},
      {MISSION_STOP, "Stop"},
      {MISSION_AMBUSH, "Ambush"},
      {MISSION_HUNT, "Hunt"},
      {MISSION_RESCUE, "Hunt"},
      {MISSION_UNLOAD, "Unload"},
      {MISSION_REPAIR, "Repair"},
      {MISSION_MISSILE, "Missile"},
  };
  for (const auto& [mission, function] : expected) {
    MissionClass state;
    FakeHandler handler;
    state.set_current(mission);
    state.AI(handler);
    ASSERT_EQ(handler.calls.size(), 1U) << function;
    EXPECT_EQ(handler.calls.front(), function);
  }
}

TEST_F(MissionTest, AIWaitsTheFramesTheMissionFunctionAskedFor) {
  Start(MISSION_GUARD);
  handler_.delay = 3;
  mission_.AI(handler_);
  ASSERT_EQ(handler_.calls.size(), 1U);

  AdvanceFrames(2);
  mission_.AI(handler_);
  EXPECT_EQ(handler_.calls.size(), 1U);

  AdvanceFrames(1);
  mission_.AI(handler_);
  EXPECT_EQ(handler_.calls.size(), 2U);
}

TEST_F(MissionTest, AQueuedMissionWaitsForCommence) {
  Start(MISSION_GUARD);
  mission_.Assign(MISSION_ATTACK);
  mission_.AI(handler_);
  EXPECT_EQ(handler_.calls, std::vector<std::string>{"Guard"});
}

TEST_F(MissionTest, CommenceRunsTheNewMissionOnTheNextAI) {
  Start(MISSION_GUARD);
  handler_.delay = 100;
  mission_.AI(handler_);

  Start(MISSION_ATTACK);
  mission_.AI(handler_);

  EXPECT_EQ(handler_.calls, (std::vector<std::string>{"Guard", "Attack"}));
}

TEST_F(MissionTest, ShortenTimerRunsTheMissionOnTheNextAI) {
  Start(MISSION_GUARD);
  handler_.delay = 100;
  mission_.AI(handler_);

  mission_.ShortenTimer();
  mission_.AI(handler_);

  EXPECT_EQ(handler_.calls.size(), 2U);
}

TEST_F(MissionTest, ASavedMissionCarriesOnWhereItWas) {
  Start(MISSION_HARVEST);
  mission_.set_step(2);
  mission_.Suspend();
  mission_.Assign(MISSION_MOVE);
  handler_.delay = 5;
  mission_.AI(handler_);

  std::array<std::byte, 64> bytes{};
  SpanSink sink(bytes);
  ArchiveWriter writer(sink);
  writer(mission_);
  MissionClass loaded;
  SpanSource source(bytes);
  ArchiveReader reader(source);
  reader(loaded);
  ASSERT_TRUE(reader.ok());

  EXPECT_EQ(loaded.current(), MISSION_HARVEST);
  EXPECT_EQ(loaded.queued(), MISSION_MOVE);
  EXPECT_EQ(loaded.suspended(), MISSION_HARVEST);
  EXPECT_EQ(loaded.step(), 2);
  FakeHandler handler;
  AdvanceFrames(4);
  loaded.AI(handler);
  EXPECT_TRUE(handler.calls.empty());
  AdvanceFrames(1);
  loaded.AI(handler);
  EXPECT_EQ(handler.calls, std::vector<std::string>{"Harvest"});
}

}  // namespace
}  // namespace ra
