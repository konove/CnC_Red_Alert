// Tests for the trigger events in a real Game with no game data: how an event
// reads its scenario INI fields, when it counts as having happened, and what
// it can be attached to.

#include "ra/ai/tevent.h"

#include <cstdint>
#include <string_view>

#include "base/array.h"
#include "base/enum_array.h"
#include "base/flags.h"
#include "base/pooled.h"
#include "base/strings/tokenizer.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/team.h"
#include "ra/ai/team_type.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/infantry.h"
#include "ra/startup/game_test_util.h"

namespace ra {

namespace {

// A Game with one team type, in slot 0 of a four-slot heap.
class TEventTest : public GameFixture {
 protected:
  void SetUp() override {
    TheObjectHeaps().team().Reset(4);
    TheObjectHeaps().team_type().Reset(4);
    team_type_ = base::NewPooled<TeamType>();
    team_type_->Set_Name("tanks");
    owner_ = base::NewPooled<HouseClass>(HOUSE_USSR);
    named_ = base::NewPooled<HouseClass>(HOUSE_GREECE);
  }

  // Makes an event of `type` with `value` as its number, or, when `value` is
  // 0, naming the Greek house; and readies the trigger's state for it.
  TriggerEvent Make(const TriggerEventType type, const int32_t value = 0) {
    TriggerEvent event(type);
    if (value != 0) {
      event.set_value(value);
    } else {
      event.set_house(HOUSE_GREECE);
    }
    event.Reset(state_);
    return event;
  }

  // Checks `event` for a Soviet trigger, as `cause` with `object`.
  bool Check(const TriggerEvent& event,
             const TriggerEventType cause = TriggerEventType::kAny,
             const ObjectClass* object = nullptr, const bool forced = false) {
    return event.HasHappened(state_, cause, HOUSE_USSR, object, forced);
  }

  // Reads an event from `entry` in INI format `format`.
  static TriggerEvent Read(const int format, const std::string_view entry) {
    TheWorld().new_ini_format() = format;
    base::Tokenizer tokens(entry);
    TriggerEvent event;
    event.ReadIni(tokens);
    return event;
  }

  TeamType* team_type_ = nullptr;
  HouseClass* owner_ = nullptr;  // The trigger's house.
  HouseClass* named_ = nullptr;  // The house the events name.
  TriggerEventState state_;
};

// A number past the last event, in any format, reads as no event rather than
// as one that indexes past the name table and always happens.
TEST_F(TEventTest, ANumberThatIsNoEventReadsAsNone) {
  EXPECT_EQ(Read(2, "33,-1,5").Event, TriggerEventType::kNone);
  EXPECT_EQ(Read(2, "-2,-1,5").Event, TriggerEventType::kNone);
  EXPECT_EQ(Read(1, "99,5").Event, TriggerEventType::kNone);
  EXPECT_EQ(Read(0, "40,0,0,None,5,0").Event, TriggerEventType::kNone);
}

// Format 1 gives a team event's team as its heap slot.
TEST_F(TEventTest, FormatOneReadsTheTeamSlot) {
  const TriggerEvent event = Read(1, "23,0");
  EXPECT_EQ(event.Event, TriggerEventType::kLeavesMap);
  EXPECT_EQ(event.Team, team_type_);
}

// A slot outside the heap reads as no team instead of failing a check.
TEST_F(TEventTest, FormatOneReadsASlotOutsideTheHeapAsNoTeam) {
  EXPECT_EQ(Read(1, "23,9").Team, nullptr);
  EXPECT_EQ(Read(1, "23,-1").Team, nullptr);
}

TEST_F(TEventTest, AForcedEventHasHappenedUntilReset) {
  const TriggerEvent event = Make(TriggerEventType::kCredits, 1000);
  EXPECT_FALSE(Check(event));

  EXPECT_TRUE(Check(event, TriggerEventType::kAny, nullptr, true));
  EXPECT_TRUE(state_.is_tripped);
  EXPECT_TRUE(Check(event));

  event.Reset(state_);
  EXPECT_FALSE(Check(event));
}

TEST_F(TEventTest, NoEventNeverHappensUnlessForced) {
  const TriggerEvent event = Make(TriggerEventType::kNone);
  EXPECT_FALSE(Check(event, TriggerEventType::kNone));
  EXPECT_FALSE(Check(event, TriggerEventType::kAny));
  EXPECT_TRUE(Check(event, TriggerEventType::kAny, nullptr, true));
}

TEST_F(TEventTest, AnyEventAlwaysHappens) {
  EXPECT_TRUE(Check(Make(TriggerEventType::kAny), TriggerEventType::kTime));
}

TEST_F(TEventTest, AGlobalEventFollowsItsGlobal) {
  const TriggerEvent set = Make(TriggerEventType::kGlobalSet, 3);
  const TriggerEvent clear = Make(TriggerEventType::kGlobalClear, 3);
  base::At(TheScenario().GlobalFlags, 3) = true;
  EXPECT_TRUE(Check(set));
  EXPECT_FALSE(Check(clear));

  base::At(TheScenario().GlobalFlags, 3) = false;
  EXPECT_FALSE(Check(set));
  EXPECT_TRUE(Check(clear));
  EXPECT_FALSE(state_.is_tripped);
}

TEST_F(TEventTest, TheMissionTimerEventNeedsARunningTimerAtZero) {
  const TriggerEvent event = Make(TriggerEventType::kMissionTimerExpired);
  TheScenario().MissionTimer.Set(0);
  EXPECT_TRUE(Check(event));

  TheScenario().MissionTimer.Stop();
  EXPECT_FALSE(Check(event));

  TheScenario().MissionTimer.Set(100);
  EXPECT_FALSE(Check(event));
}

TEST_F(TEventTest, ATimeEventWaitsForItsCountdown) {
  EXPECT_FALSE(Check(Make(TriggerEventType::kTime, 10)));
  EXPECT_EQ(state_.timer.Remaining(), 10 * (kTicksPerMinute / 10));
  const TriggerEvent now(TriggerEventType::kTime);
  now.Reset(state_);
  EXPECT_TRUE(Check(now));
}

// The moment events count when the check is made for them, or for any
// event, and are not remembered.
TEST_F(TEventTest, AMomentEventHappensWhenCheckedForIt) {
  for (const TriggerEventType type :
       {TriggerEventType::kAttacked, TriggerEventType::kDestroyed,
        TriggerEventType::kDiscovered, TriggerEventType::kSpied}) {
    SCOPED_TRACE(magic_enum::enum_name(type));
    const TriggerEvent event = Make(type);
    EXPECT_TRUE(Check(event, type));
    EXPECT_TRUE(Check(event, TriggerEventType::kAny));
    EXPECT_FALSE(Check(event, TriggerEventType::kTime));
    EXPECT_FALSE(state_.is_tripped);
  }
}

// The entering events also need an object of the house the event names, and
// stay tripped once it has entered.
TEST_F(TEventTest, AnEnteringEventNeedsAnObjectOfItsHouse) {
  InitInfantryTypes();
  TheObjectHeaps().infantry().Reset(2);
  const auto* greek = base::NewPooled<InfantryClass>(INFANTRY_E1, HOUSE_GREECE);
  const auto* soviet = base::NewPooled<InfantryClass>(INFANTRY_E1, HOUSE_USSR);
  ASSERT_NE(greek, nullptr);
  ASSERT_NE(soviet, nullptr);
  for (const TriggerEventType type :
       {TriggerEventType::kPlayerEntered, TriggerEventType::kEntersZone,
        TriggerEventType::kCrossHorizontal, TriggerEventType::kCrossVertical}) {
    SCOPED_TRACE(magic_enum::enum_name(type));
    const TriggerEvent event = Make(type);
    EXPECT_FALSE(Check(event, type));
    EXPECT_FALSE(Check(event, type, soviet));
    EXPECT_FALSE(Check(event, TriggerEventType::kTime, greek));
    EXPECT_FALSE(state_.is_tripped);

    EXPECT_TRUE(Check(event, type, greek));
    EXPECT_TRUE(state_.is_tripped);
    EXPECT_TRUE(Check(event, TriggerEventType::kTime));
  }
}

TEST_F(TEventTest, TheBridgeEventStaysTrippedOnceTheLastBridgeFalls) {
  const TriggerEvent event = Make(TriggerEventType::kAllBridgesDestroyed);
  TheScenario().BridgeCount = 1;
  EXPECT_FALSE(Check(event));

  TheScenario().BridgeCount = 0;
  EXPECT_TRUE(Check(event));
  TheScenario().BridgeCount = 1;
  EXPECT_TRUE(Check(event));
}

TEST_F(TEventTest, TheLeavesMapEventWaitsForAnEmptyTeamOfItsTypeToLeave) {
  TriggerEvent event = Make(TriggerEventType::kLeavesMap);
  event.Team = team_type_;
  EXPECT_FALSE(Check(event));

  auto* other_type = base::NewPooled<TeamType>();
  auto* other = base::NewPooled<TeamClass>(other_type);
  other->IsLeaveMap = true;
  EXPECT_FALSE(Check(event));

  auto* team = base::NewPooled<TeamClass>(team_type_);
  EXPECT_FALSE(Check(event));
  team->IsLeaveMap = true;
  EXPECT_TRUE(Check(event));
  EXPECT_TRUE(state_.is_tripped);
}

// The trigger's house: what it has, has built and has lost.
TEST_F(TEventTest, TheTriggersHouseConditionsAreReadFromItsState) {
  const TriggerEvent credits = Make(TriggerEventType::kCredits, 500);
  owner_->Credits = 499;
  EXPECT_FALSE(Check(credits));
  owner_->Credits = 500;
  EXPECT_TRUE(Check(credits));

  const TriggerEvent factories = Make(TriggerEventType::kNoFactories);
  owner_->BScan = kStructFlagWeap;
  EXPECT_FALSE(Check(factories));
  owner_->BScan = kStructFlagPower;
  EXPECT_TRUE(Check(factories));

  const TriggerEvent fakes = Make(TriggerEventType::kFakesDestroyed);
  owner_->BScan = kStructFlagFakeConst;
  EXPECT_FALSE(Check(fakes));
  owner_->BScan = kStructFlagFakeWeap;
  EXPECT_FALSE(Check(fakes));
  owner_->BScan = 0;
  EXPECT_TRUE(Check(fakes));

  const TriggerEvent evacuated = Make(TriggerEventType::kCivilianEvacuated);
  EXPECT_FALSE(Check(evacuated));
  owner_->IsCivEvacuated = true;
  EXPECT_TRUE(Check(evacuated));

  TriggerEvent exists = Make(TriggerEventType::kBuildingExists);
  exists.set_structure(STRUCT_WEAP);
  EXPECT_FALSE(Check(exists));
  owner_->ActiveBScan = kStructFlagWeap;
  EXPECT_TRUE(Check(exists));

  const TriggerEvent buildings_lost = Make(TriggerEventType::kBuildingsLost, 2);
  owner_->BuildingsLost = 1;
  EXPECT_FALSE(Check(buildings_lost));
  owner_->BuildingsLost = 2;
  EXPECT_TRUE(Check(buildings_lost));

  const TriggerEvent units_lost = Make(TriggerEventType::kUnitsLost, 2);
  owner_->UnitsLost = 1;
  EXPECT_FALSE(Check(units_lost));
  owner_->UnitsLost = 3;
  EXPECT_TRUE(Check(units_lost));
  EXPECT_FALSE(state_.is_tripped);
}

// What the house has just built changes with the next thing it builds, so the
// building events stay tripped.
TEST_F(TEventTest, ABuildEventStaysTrippedOnceTheTypeIsBuilt) {
  TriggerEvent building = Make(TriggerEventType::kBuildBuilding);
  building.set_structure(STRUCT_WEAP);
  EXPECT_FALSE(Check(building));
  owner_->JustBuiltStructure = STRUCT_WEAP;
  EXPECT_TRUE(Check(building));
  owner_->JustBuiltStructure = STRUCT_POWER;
  EXPECT_TRUE(Check(building));

  TriggerEvent unit = Make(TriggerEventType::kBuildUnit);
  unit.set_unit(UNIT_JEEP);
  EXPECT_FALSE(Check(unit));
  owner_->JustBuiltUnit = UNIT_JEEP;
  EXPECT_TRUE(Check(unit));

  TriggerEvent infantry = Make(TriggerEventType::kBuildInfantry);
  infantry.set_infantry(INFANTRY_E1);
  EXPECT_FALSE(Check(infantry));
  owner_->JustBuiltInfantry = INFANTRY_E1;
  EXPECT_TRUE(Check(infantry));

  TriggerEvent aircraft = Make(TriggerEventType::kBuildAircraft);
  aircraft.set_aircraft(AIRCRAFT_HIND);
  EXPECT_FALSE(Check(aircraft));
  owner_->JustBuiltAircraft = AIRCRAFT_HIND;
  EXPECT_TRUE(Check(aircraft));
}

// The house the event names: its power, what was stolen from it, whether it
// has been seen and what it has left.
TEST_F(TEventTest, TheNamedHousesConditionsAreReadFromItsState) {
  const TriggerEvent power = Make(TriggerEventType::kLowPower);
  named_->Power = 100;
  named_->Drain = 100;
  EXPECT_FALSE(Check(power));
  named_->Drain = 101;
  EXPECT_TRUE(Check(power));

  const TriggerEvent thieved = Make(TriggerEventType::kThieved);
  EXPECT_FALSE(Check(thieved));
  named_->IsThieved = true;
  EXPECT_TRUE(Check(thieved));

  const TriggerEvent discovered = Make(TriggerEventType::kHouseDiscovered);
  EXPECT_FALSE(Check(discovered));
  named_->IsDiscovered = true;
  EXPECT_TRUE(Check(discovered));

  const TriggerEvent buildings = Make(TriggerEventType::kBuildingsDestroyed);
  const TriggerEvent units = Make(TriggerEventType::kUnitsDestroyed);
  const TriggerEvent all = Make(TriggerEventType::kAllDestroyed);
  named_->ActiveBScan = 1;
  EXPECT_FALSE(Check(buildings));
  EXPECT_TRUE(Check(units));
  EXPECT_FALSE(Check(all));

  named_->ActiveBScan = 0;
  named_->ActiveIScan = 1;
  EXPECT_TRUE(Check(buildings));
  EXPECT_FALSE(Check(units));
  EXPECT_FALSE(Check(all));

  // Vessels count only for everything destroyed, aircraft for nothing.
  named_->ActiveIScan = 0;
  named_->ActiveVScan = 1;
  EXPECT_TRUE(Check(units));
  EXPECT_FALSE(Check(all));
  named_->ActiveVScan = 0;
  named_->ActiveAScan = 1;
  EXPECT_TRUE(Check(units));
  EXPECT_TRUE(Check(all));
}

// A condition whose house is not in the game is not checked and counts as
// having happened.
TEST_F(TEventTest, AConditionOnAHouseNotInTheGameHasHappened) {
  const TriggerEvent credits = Make(TriggerEventType::kCredits, 500);
  EXPECT_TRUE(credits.HasHappened(state_, TriggerEventType::kAny, HOUSE_ENGLAND,
                                  nullptr, false));

  TriggerEvent thieved = Make(TriggerEventType::kThieved);
  thieved.set_house(HOUSE_ENGLAND);
  EXPECT_TRUE(Check(thieved));
}

// What each event can be attached to, as the scenario's trigger lists and
// the editor read it.
TEST(EventAttachmentTest, EachEventAttachesToWhatItWatches) {
  constexpr TriggerAttachment kCell = TriggerAttachment::kCell;
  constexpr TriggerAttachment kObject = TriggerAttachment::kObject;
  constexpr TriggerAttachment kMap = TriggerAttachment::kMap;
  constexpr TriggerAttachment kHouse = TriggerAttachment::kHouse;
  constexpr TriggerAttachment kGeneral = TriggerAttachment::kGeneral;
  const base::EnumArray<TriggerEventType, TriggerAttachment> expected = {
      kCell | kObject,                             // kNone
      kCell | kObject,                             // kPlayerEntered
      kObject,                                     // kSpied
      kHouse,                                      // kThieved
      kCell | kObject,                             // kDiscovered
      kHouse,                                      // kHouseDiscovered
      kObject,                                     // kAttacked
      kObject,                                     // kDestroyed
      kCell | kObject | kMap | kHouse | kGeneral,  // kAny
      kHouse,                                      // kUnitsDestroyed
      kHouse,                                      // kBuildingsDestroyed
      kHouse,                                      // kAllDestroyed
      kHouse,                                      // kCredits
      kGeneral,                                    // kTime
      kGeneral,                                    // kMissionTimerExpired
      kHouse,                                      // kBuildingsLost
      kHouse,                                      // kUnitsLost
      kHouse,                                      // kNoFactories
      kHouse,                                      // kCivilianEvacuated
      kHouse,                                      // kBuildBuilding
      kHouse,                                      // kBuildUnit
      kHouse,                                      // kBuildInfantry
      kHouse,                                      // kBuildAircraft
      kGeneral,                                    // kLeavesMap
      kCell | kMap,                                // kEntersZone
      kCell,                                       // kCrossHorizontal
      kCell,                                       // kCrossVertical
      kGeneral,                                    // kGlobalSet
      kGeneral,                                    // kGlobalClear
      kHouse,                                      // kFakesDestroyed
      kHouse,                                      // kLowPower
      kGeneral,                                    // kAllBridgesDestroyed
      kHouse,                                      // kBuildingExists
  };
  for (const TriggerEventType event :
       magic_enum::enum_values<TriggerEventType>()) {
    SCOPED_TRACE(magic_enum::enum_name(event));
    EXPECT_EQ(EventAttachment(event), expected.at(event));
  }
}

}  // namespace

}  // namespace ra
