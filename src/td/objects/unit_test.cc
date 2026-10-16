// Tests UnitClass against a real Game with no game data: a house and its
// vehicles are built by hand, and the vehicles that aim and fire stand on an
// empty map.

#include "td/objects/unit.h"

#include <cstdint>

#include "base/pooled.h"
#include "game/sim/direction.h"
#include "game/sim/enums.h"
#include "gtest/gtest.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/core/target.h"
#include "td/map/map.h"
#include "td/map/world.h"
#include "td/objects/house.h"
#include "td/startup/game_state.h"
#include "td/startup/game_test_util.h"

namespace td {
namespace {

class UnitTest : public GameFixture {
 protected:
  UnitTest() {
    TheObjectHeaps().house().Reset(1);
    TheObjectHeaps().unit().Reset(8);
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GOOD);
    // The map layer alone: the display layers above it load game data.
    TheMap().MapClass::One_Time();
    TheMap().MapClass::Set_Map_Dimensions(1, 1, MAP_CELL_W - 2, MAP_CELL_H - 2);
    // Nothing leaves limbo while the game is not active.
    TheGameState().active() = true;
  }

  // A vehicle of `type` standing at kHome with its body and its turret
  // facing `facing`.
  static UnitClass& Place(const UnitType type, const game::Direction facing) {
    return PlaceAt(kHome, type, facing);
  }

  static UnitClass& PlaceAt(const Cell cell, const UnitType type,
                            const game::Direction facing) {
    auto* unit = base::NewPooled<UnitClass>(type, HOUSE_GOOD);
    EXPECT_NE(unit, nullptr);
    // As a scenario places its vehicles: without asking the cell.
    TheWorld().scenario_init()++;
    EXPECT_TRUE(unit->Unlimbo(cell.Center(), facing));
    TheWorld().scenario_init()--;
    unit->PrimaryFacing.SnapTo(facing);
    unit->SecondaryFacing.SnapTo(facing);
    return *unit;
  }

  // The cell `columns` east and `rows` south of kHome, as a target.
  static Target CellTarget(const int columns, const int rows = 0) {
    return Target::FromCellIndex(
        Cell::FromXY(kHomeColumn + columns, kHomeRow + rows));
  }

  // Runs `unit` until its turret has stopped turning and returns the game
  // frames that took.
  static int FramesUntilTheTurretRests(UnitClass& unit) {
    int frames = 0;
    do {
      unit.AI();
      ++frames;
    } while (unit.SecondaryFacing.IsRotating() && frames < kTooManyFrames);
    return frames;
  }

  // An APC in limbo that faces `facing`.
  static UnitClass& MakeApc(const game::Direction facing) {
    auto* apc = base::NewPooled<UnitClass>(UNIT_APC, HOUSE_GOOD);
    EXPECT_NE(apc, nullptr);
    apc->PrimaryFacing.set_current(facing);
    return *apc;
  }

  // Runs the door of `apc` until `done` says it has arrived and returns the
  // game frames that took. Every frame on the way has to be one of the three
  // the door has: the draw code adds it to shape 32 (north-east) or 35
  // (north-west), and the door's shapes end at 37.
  template <class Done>
  static int FramesUntil(UnitClass& apc, const Done done) {
    int frames = 0;
    while (!done(apc.door()) && frames < kTooManyFrames) {
      apc.door().AI();
      ++frames;
      EXPECT_GE(apc.door().Frame(), 0);
      EXPECT_LT(apc.door().Frame(), kDoorFrames);
    }
    return frames;
  }

  static int FramesToOpen(UnitClass& apc) {
    return FramesUntil(apc,
                       [](const game::Door& door) { return door.IsOpen(); });
  }

  static int FramesToClose(UnitClass& apc) {
    return FramesUntil(apc,
                       [](const game::Door& door) { return door.IsClosed(); });
  }

  static constexpr int kDoorFrames = 3;
  // More than any door animation or turn lasts: one that never arrives ends
  // here.
  static constexpr int kTooManyFrames = 100;
  static constexpr int kHomeColumn = 40;
  static constexpr int kHomeRow = 40;
  static constexpr Cell kHome = Cell::FromXY(kHomeColumn, kHomeRow);
};

// The door is drawn at these two facings only, three game frames a stage.
TEST_F(UnitTest, AnApcDoorOpensInNineFramesWhereItIsDrawn) {
  for (const game::Direction facing :
       {game::Direction::kNorthEast, game::Direction::kNorthWest}) {
    UnitClass& apc = MakeApc(facing);
    apc.APC_Open_Door();
    EXPECT_TRUE(apc.door().IsOpening());
    EXPECT_EQ(FramesToOpen(apc), 9);
    EXPECT_EQ(apc.door().Frame(), kDoorFrames - 1);
  }
}

TEST_F(UnitTest, AnApcDoorOpensInThreeFramesWhereItIsNotDrawn) {
  for (const game::Direction facing :
       {game::Direction::kNorth, game::Direction::kEast,
        game::Direction::kSouthEast, game::Direction::kSouth,
        game::Direction::kSouthWest, game::Direction::kWest}) {
    UnitClass& apc = MakeApc(facing);
    apc.APC_Open_Door();
    EXPECT_EQ(FramesToOpen(apc), 3);
  }
}

TEST_F(UnitTest, AnApcDoorStaysShutWhileTheApcDrivesOrTurns) {
  UnitClass& driving = MakeApc(game::Direction::kNorthEast);
  driving.IsDriving = true;
  driving.APC_Open_Door();
  EXPECT_TRUE(driving.door().IsClosed());

  UnitClass& turning = MakeApc(game::Direction::kNorthEast);
  turning.IsRotating = true;
  turning.APC_Open_Door();
  EXPECT_TRUE(turning.door().IsClosed());
}

// Closing takes the drawn door's time at every facing.
TEST_F(UnitTest, AnApcDoorClosesInNineFrames) {
  for (const game::Direction facing :
       {game::Direction::kNorthEast, game::Direction::kSouth}) {
    UnitClass& apc = MakeApc(facing);
    apc.APC_Open_Door();
    FramesToOpen(apc);
    apc.APC_Close_Door();
    EXPECT_EQ(FramesToClose(apc), 9);
    EXPECT_EQ(apc.door().Frame(), 0);
  }
}

// The gun must point at the target: until then the tank may not fire, and
// while the turret swings it may not either. The body stays where it is.
TEST_F(UnitTest, ATurretTurnsToItsTargetBeforeTheTankMayFire) {
  UnitClass& tank = Place(UNIT_LTANK, game::Direction::kNorth);
  const Target target = CellTarget(2);
  EXPECT_EQ(tank.Can_Fire(target, 0), game::FIRE_FACING);

  tank.Assign_Target(target);
  tank.AI();
  EXPECT_EQ(tank.SecondaryFacing.desired(), game::Direction::kEast);
  tank.AI();
  EXPECT_TRUE(tank.IsRotating);
  EXPECT_NE(tank.SecondaryFacing.current(), game::Direction::kNorth);
  EXPECT_EQ(tank.Can_Fire(target, 0), game::FIRE_ROTATING);

  EXPECT_GT(FramesUntilTheTurretRests(tank), 1);
  EXPECT_EQ(tank.SecondaryFacing.current(), game::Direction::kEast);
  EXPECT_EQ(tank.PrimaryFacing.current(), game::Direction::kNorth);
  EXPECT_FALSE(tank.IsRotating);
  EXPECT_EQ(tank.Can_Fire(target, 0), game::FIRE_OK);
}

TEST_F(UnitTest, ATurretFollowsATargetThatIsOutOfRange) {
  UnitClass& tank = Place(UNIT_LTANK, game::Direction::kNorth);
  const Target target = CellTarget(12);
  // A guarding unit drops a target it cannot reach.
  tank.Assign_Mission(MISSION_SLEEP);
  tank.Assign_Target(target);
  ASSERT_EQ(tank.Can_Fire(target, 0), game::FIRE_RANGE);

  tank.AI();
  EXPECT_EQ(tank.SecondaryFacing.desired(), game::Direction::kEast);
  EXPECT_EQ(tank.PrimaryFacing.desired(), game::Direction::kNorth);
}

TEST_F(UnitTest, ATurretWithNothingToAimAtReturnsToTheBodysFacing) {
  UnitClass& tank = Place(UNIT_LTANK, game::Direction::kNorth);
  tank.SecondaryFacing.SnapTo(game::Direction::kEast);

  tank.AI();
  EXPECT_EQ(tank.SecondaryFacing.desired(), game::Direction::kNorth);
  FramesUntilTheTurretRests(tank);
  EXPECT_EQ(tank.SecondaryFacing.current(), game::Direction::kNorth);
}

// A rocket launcher drives with its launcher stowed along the body.
TEST_F(UnitTest, ALockedTurretRefusesToMoveUntilItHasSwungRound) {
  UnitClass& launcher = Place(UNIT_MSAM, game::Direction::kNorth);
  EXPECT_TRUE(launcher.Ok_To_Move(game::Direction::kNorth));

  EXPECT_FALSE(launcher.Ok_To_Move(game::Direction::kEast));
  EXPECT_EQ(launcher.SecondaryFacing.desired(), game::Direction::kEast);

  launcher.AI();
  ASSERT_TRUE(launcher.IsRotating);
  EXPECT_FALSE(launcher.Ok_To_Move(game::Direction::kEast));
  // Asking for another facing meanwhile does not redirect the turret.
  EXPECT_FALSE(launcher.Ok_To_Move(game::Direction::kWest));
  EXPECT_EQ(launcher.SecondaryFacing.desired(), game::Direction::kEast);

  int frames = 0;
  while (launcher.IsRotating && frames < kTooManyFrames) {
    launcher.AI();
    ++frames;
  }
  EXPECT_EQ(launcher.SecondaryFacing.current(), game::Direction::kEast);
  EXPECT_TRUE(launcher.Ok_To_Move(game::Direction::kEast));
}

TEST_F(UnitTest, AFreeTurretNeverHoldsTheTankBack) {
  UnitClass& tank = Place(UNIT_LTANK, game::Direction::kNorth);
  EXPECT_TRUE(tank.Ok_To_Move(game::Direction::kEast));
  EXPECT_EQ(tank.SecondaryFacing.desired(), game::Direction::kNorth);
}

// Only a turret that turns on its own lets a vehicle shoot on the way.
TEST_F(UnitTest, AUnitWithoutAFreeTurretCannotFireWhileItHasADestination) {
  const Target target = CellTarget(2);
  const Target destination = CellTarget(0, 5);

  UnitClass& apc = Place(UNIT_APC, game::Direction::kEast);
  EXPECT_EQ(apc.Can_Fire(target, 0), game::FIRE_OK);
  apc.NavCom = destination;
  EXPECT_EQ(apc.Can_Fire(target, 0), game::FIRE_MOVING);
  apc.NavCom = Target{};
  apc.Limbo();

  UnitClass& launcher = Place(UNIT_MSAM, game::Direction::kEast);
  EXPECT_EQ(launcher.Can_Fire(target, 0), game::FIRE_OK);
  launcher.NavCom = destination;
  EXPECT_EQ(launcher.Can_Fire(target, 0), game::FIRE_MOVING);
  launcher.NavCom = Target{};
  launcher.Limbo();

  UnitClass& tank = Place(UNIT_LTANK, game::Direction::kEast);
  tank.NavCom = destination;
  EXPECT_EQ(tank.Can_Fire(target, 0), game::FIRE_OK);
  tank.NavCom = Target{};
}

// The body is the gun of a vehicle without a turret.
TEST_F(UnitTest, AUnitWithoutATurretAimsWithItsBody) {
  UnitClass& apc = Place(UNIT_APC, game::Direction::kNorth);
  const Target target = CellTarget(2);
  EXPECT_EQ(apc.Can_Fire(target, 0), game::FIRE_FACING);

  apc.Assign_Target(target);
  apc.AI();
  EXPECT_EQ(apc.PrimaryFacing.desired(), game::Direction::kEast);
}

// A flame tank fires along one of the eight facings its body is drawn at.
TEST_F(UnitTest, AFlameTankTurnsItsBodyToTheNearestFacing) {
  // Three cells east and one south is a little south of east.
  const Target target = CellTarget(3, 1);

  UnitClass& apc = Place(UNIT_APC, game::Direction::kNorth);
  apc.Assign_Target(target);
  apc.AI();
  EXPECT_EQ(apc.PrimaryFacing.desired(), apc.DirectionTo(target));
  EXPECT_NE(apc.PrimaryFacing.desired(), game::Direction::kEast);
  apc.Limbo();

  UnitClass& flame_tank = Place(UNIT_FTANK, game::Direction::kNorth);
  // The flame does not reach that far, and a guarding unit drops a target it
  // cannot reach.
  flame_tank.Assign_Mission(MISSION_SLEEP);
  flame_tank.Assign_Target(target);
  flame_tank.AI();
  EXPECT_EQ(flame_tank.PrimaryFacing.desired(), game::Direction::kEast);
}

// Stowed for the drive, the turret ignores what the vehicle aims at.
TEST_F(UnitTest, ATurretLockedDownStaysAlongTheBodyDespiteATarget) {
  UnitClass& tank = Place(UNIT_LTANK, game::Direction::kNorth);
  tank.SecondaryFacing.SnapTo(game::Direction::kWest);
  tank.IsTurretLockedDown = true;
  tank.Assign_Target(CellTarget(2));

  tank.AI();
  EXPECT_EQ(tank.SecondaryFacing.desired(), game::Direction::kNorth);
  for (int frame = 0; frame < kTooManyFrames; ++frame) {
    tank.AI();
  }
  EXPECT_EQ(tank.SecondaryFacing.current(), game::Direction::kNorth);
}

TEST_F(UnitTest, AnIdleTurretLooksWhereTheTankIsGoing) {
  UnitClass& tank = Place(UNIT_LTANK, game::Direction::kNorth);
  tank.NavCom = CellTarget(3);

  tank.AI();
  EXPECT_EQ(tank.SecondaryFacing.desired(), game::Direction::kEast);
  tank.NavCom = Target{};
}

// A launcher whose turret is fixed while it drives turns body and turret
// together.
TEST_F(UnitTest, ALockedTurretTurnsWithTheBodyTowardsTheTarget) {
  UnitClass& launcher = Place(UNIT_MLRS, game::Direction::kEast);
  // Behind it: homing rockets forgive rough aim, not half a turn.
  const Target target = CellTarget(-3);
  ASSERT_EQ(launcher.Can_Fire(target, 0), game::FIRE_FACING);
  // A turret on its way somewhere else is redirected as well.
  launcher.SecondaryFacing.set_desired(game::Direction::kNorth);

  launcher.Assign_Target(target);
  launcher.AI();
  EXPECT_EQ(launcher.PrimaryFacing.desired(), game::Direction::kWest);
  EXPECT_EQ(launcher.SecondaryFacing.desired(), game::Direction::kWest);
}

// A shell needs the gun within 8/256 of a turn of the target; a homing
// rocket finds its way from four times as far off, and from a turret that
// is still turning.
TEST_F(UnitTest, AHomingWeaponNeedsOnlyRoughAim) {
  const Target target = CellTarget(3);
  const game::Direction rough = game::Direction::kEast + game::Direction(20);

  UnitClass& tank = Place(UNIT_LTANK, game::Direction::kEast);
  tank.SecondaryFacing.SnapTo(rough);
  EXPECT_EQ(tank.Can_Fire(target, 0), game::FIRE_FACING);
  tank.Limbo();

  UnitClass& launcher = Place(UNIT_MLRS, game::Direction::kEast);
  launcher.SecondaryFacing.SnapTo(rough);
  EXPECT_EQ(launcher.Can_Fire(target, 0), game::FIRE_OK);
  launcher.IsRotating = true;
  EXPECT_EQ(launcher.Can_Fire(target, 0), game::FIRE_OK);
  launcher.SecondaryFacing.SnapTo(game::Direction::kEast + game::Direction(32));
  EXPECT_EQ(launcher.Can_Fire(target, 0), game::FIRE_FACING);
}

// The flame fans out, so the body only has to be drawn towards the target.
TEST_F(UnitTest, AFlameTankFiresAtWhateverItsBodyFacingCovers) {
  // An object a cell east whose centre is south of the row's: 15/256 of a
  // turn off east, which is still the east facing.
  UnitClass& victim = PlaceAt(Cell::FromXY(kHomeColumn + 1, kHomeRow),
                              UNIT_JEEP, game::Direction::kNorth);
  victim.Coord =
      Coordinate::FromLeptons(victim.Coord.x(), victim.Coord.y() + 0x60);
  const Target target = victim.As_Target();

  UnitClass& apc = Place(UNIT_APC, game::Direction::kEast);
  EXPECT_EQ(apc.Can_Fire(target, 0), game::FIRE_FACING);
  apc.Limbo();

  UnitClass& flame_tank = Place(UNIT_FTANK, game::Direction::kEast);
  EXPECT_EQ(flame_tank.Can_Fire(target, 0), game::FIRE_OK);
  flame_tank.PrimaryFacing.SnapTo(game::Direction::kSouthEast);
  EXPECT_EQ(flame_tank.Can_Fire(target, 0), game::FIRE_FACING);

  // Then it turns to the facing that covers the target, not to the target.
  flame_tank.Assign_Mission(MISSION_SLEEP);
  flame_tank.Assign_Target(target);
  flame_tank.AI();
  EXPECT_EQ(flame_tank.PrimaryFacing.desired(), game::Direction::kEast);
  EXPECT_EQ(flame_tank.SecondaryFacing.desired(), game::Direction::kEast);
}

TEST_F(UnitTest, ATankThatFacesItsTargetFiresAndHasToRearm) {
  TheObjectHeaps().bullet().Reset(4);
  UnitClass& tank = Place(UNIT_LTANK, game::Direction::kEast);
  const Target target = CellTarget(2);
  tank.Assign_Target(target);

  tank.AI();
  EXPECT_EQ(TheObjectHeaps().bullet().Count(), 1);
  EXPECT_EQ(tank.Can_Fire(target, 0), game::FIRE_REARM);
}

// Its two rockets come back one at a time, half a minute apart.
TEST_F(UnitTest, ARocketLauncherStartsToReloadWhenItFires) {
  TheObjectHeaps().bullet().Reset(4);
  UnitClass& launcher = Place(UNIT_MSAM, game::Direction::kEast);
  const int full = launcher.Ammo;
  launcher.Assign_Target(CellTarget(4));

  launcher.AI();
  EXPECT_EQ(TheObjectHeaps().bullet().Count(), 1);
  EXPECT_EQ(launcher.Ammo, full - 1);
  EXPECT_EQ(launcher.Reload.Remaining(), int64_t{kTicksPerSecond} * 30);
}

// The mammoth tank's cannon wants exact aim; its rockets do not.
TEST_F(UnitTest, TheSecondWeaponFiresWhenTheFirstCannot) {
  TheObjectHeaps().bullet().Reset(4);
  UnitClass& mammoth = Place(UNIT_HTANK, game::Direction::kEast);
  mammoth.SecondaryFacing.SnapTo(game::Direction::kEast + game::Direction(20));
  const Target target = CellTarget(3);
  ASSERT_EQ(mammoth.Can_Fire(target, 0), game::FIRE_FACING);
  ASSERT_EQ(mammoth.Can_Fire(target, 1), game::FIRE_OK);
  mammoth.Assign_Target(target);

  mammoth.AI();
  ASSERT_EQ(TheObjectHeaps().bullet().Count(), 1);
  EXPECT_TRUE(TheObjectHeaps().bullet().ActiveAt(0)->Class->IsHoming);
}

TEST_F(UnitTest, ARadarDishKeepsTurning) {
  UnitClass& radar = Place(UNIT_MHQ, game::Direction::kNorth);
  radar.AI();
  EXPECT_EQ(radar.SecondaryFacing.current(), game::Direction(8));
  radar.AI();
  EXPECT_EQ(radar.SecondaryFacing.current(), game::Direction(16));
  EXPECT_EQ(radar.PrimaryFacing.current(), game::Direction::kNorth);
  EXPECT_FALSE(radar.IsRotating);
}

// The bite lands at the fourth stage of the attack animation, which the
// first attempt to fire starts.
TEST_F(UnitTest, ADinosaurStartsItsAttackAnimationAndBitesAtStageFour) {
  UnitClass& rex = Place(UNIT_TREX, game::Direction::kEast);
  const Target target = CellTarget(1);
  // Asking alone starts nothing.
  EXPECT_EQ(rex.Can_Fire(target, 0), game::FIRE_BUSY);
  EXPECT_FALSE(rex.IsFiring);

  rex.animation().set_stage(7);
  rex.Assign_Target(target);
  rex.AI();
  EXPECT_TRUE(rex.IsFiring);
  EXPECT_EQ(rex.animation().stage(), 0);
  EXPECT_NE(rex.animation().frames_per_stage(), 0);
  EXPECT_EQ(rex.Can_Fire(target, 0), game::FIRE_BUSY);

  rex.animation().set_stage(3);
  EXPECT_EQ(rex.Can_Fire(target, 0), game::FIRE_BUSY);
  rex.animation().set_stage(4);
  EXPECT_EQ(rex.Can_Fire(target, 0), game::FIRE_OK);
}

// Out of range there is no animation to start.
TEST_F(UnitTest, ADinosaurOutOfRangeDoesNotStartItsAttack) {
  UnitClass& rex = Place(UNIT_TREX, game::Direction::kEast);
  const Target target = CellTarget(5);
  rex.Assign_Target(target);
  rex.AI();
  EXPECT_FALSE(rex.IsFiring);
  EXPECT_EQ(rex.Can_Fire(target, 0), game::FIRE_RANGE);
}

TEST_F(UnitTest, ACloakedTankUncloaksToFire) {
  UnitClass& stealth_tank = Place(UNIT_STANK, game::Direction::kEast);
  const Target target = CellTarget(2);
  stealth_tank.Cloak = game::CLOAKED;
  EXPECT_EQ(stealth_tank.Can_Fire(target, 0), game::FIRE_CLOAKED);

  stealth_tank.Assign_Target(target);
  stealth_tank.AI();
  EXPECT_NE(stealth_tank.Cloak, game::CLOAKED);
}

}  // namespace
}  // namespace td
