// Tests for the object sections of a scenario INI: what the readers build
// from an entry, the writers must write back. The writers are reached only
// from the scenario editor's Save, so nothing else exercises them.

#include <string>
#include <string_view>

#include "base/pooled.h"
#include "engine/file/ini_file.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/data/ini_io.h"
#include "td/map/world.h"
#include "td/objects/aircraft.h"
#include "td/objects/building.h"
#include "td/objects/house.h"
#include "td/objects/infantry.h"
#include "td/objects/terrain.h"
#include "td/objects/unit.h"
#include "td/startup/game_state.h"
#include "td/startup/game_test_util.h"

namespace td {
namespace {

// A running game with both sides' houses and room for a few objects of each
// kind.
class ScenarioObjectsIniTest : public GameFixture {
 protected:
  ScenarioObjectsIniTest() {
    TheObjectHeaps().house().Reset(2);
    TheObjectHeaps().unit().Reset(4);
    TheObjectHeaps().infantry().Reset(4);
    TheObjectHeaps().aircraft().Reset(4);
    TheObjectHeaps().building().Reset(4);
    TheObjectHeaps().terrain().Reset(4);
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GOOD);
    base::NewPooled<HouseClass>(HOUSE_BAD);
    // Objects are put on the map as a scenario is read, in a running game.
    TheGameState().active() = true;
    TheWorld().scenario_init() = 1;
  }

  // Returns the entry `entry` of `section` as `Writer` writes it after
  // `Reader` has built the objects of the INI text `text`.
  template <class Object>
  static std::string RoundTrip(const std::string_view text,
                               const std::string_view entry) {
    engine::file::IniFile read;
    LoadIniText(text, read);
    Object::Read_INI(read);

    engine::file::IniFile written;
    Object::Write_INI(written);
    return written.GetString(Object::INI_Name(), entry, "");
  }
};

TEST_F(ScenarioObjectsIniTest, AVehicleIsWrittenAsItWasRead) {
  EXPECT_EQ(RoundTrip<UnitClass>(
                "[UNITS]\n000=GoodGuy,MTNK,256,1320,64,Guard,None\n", "000"),
            "GoodGuy,MTNK,256,1320,64,Guard,None");
  EXPECT_EQ(TheObjectHeaps().unit().Count(), 1);
}

TEST_F(ScenarioObjectsIniTest, ASoldierIsWrittenAsItWasRead) {
  EXPECT_EQ(RoundTrip<InfantryClass>(
                "[INFANTRY]\n000=BadGuy,E1,256,1325,2,Guard,128,None\n", "000"),
            "BadGuy,E1,256,1325,2,Guard,128,None");
}

TEST_F(ScenarioObjectsIniTest, ABuildingIsWrittenAsItWasRead) {
  EXPECT_EQ(RoundTrip<BuildingClass>(
                "[STRUCTURES]\n000=GoodGuy,NUKE,256,1960,0,None\n", "000"),
            "GoodGuy,NUKE,256,1960,0,None");
}

// TODO: The mission does not survive: the reader queues it behind the
// "move" an aircraft starts with on entering the map, and the writer writes
// the one in hand. Saving a scenario in the editor turns every aircraft's
// mission into Move.
TEST_F(ScenarioObjectsIniTest, AnAircraftIsWrittenWithTheMissionItStartsOn) {
  EXPECT_EQ(RoundTrip<AircraftClass>(
                "[AIRCRAFT]\n000=GoodGuy,ORCA,256,1330,0,Guard\n", "000"),
            "GoodGuy,ORCA,256,1330,0,Move");
}

TEST_F(ScenarioObjectsIniTest, AnObjectOfAnUnknownHouseOrTypeIsNotBuilt) {
  engine::file::IniFile ini;
  LoadIniText(
      "[UNITS]\n"
      "000=Nobody,MTNK,256,1320,64,Guard,None\n"
      "001=GoodGuy,NOSUCH,256,1321,64,Guard,None\n",
      ini);

  UnitClass::Read_INI(ini);

  EXPECT_EQ(TheObjectHeaps().unit().Count(), 0);
}

// The writer replaces the section: entries of objects that no longer exist
// must not stay behind.
TEST_F(ScenarioObjectsIniTest, WritingDropsTheEntriesOfObjectsThatAreGone) {
  engine::file::IniFile ini;
  LoadIniText(
      "[UNITS]\n"
      "000=GoodGuy,MTNK,256,1320,64,Guard,None\n"
      "007=GoodGuy,MTNK,256,1400,64,Guard,None\n",
      ini);
  UnitClass::Read_INI(ini);
  ASSERT_EQ(TheObjectHeaps().unit().Count(), 2);

  UnitClass::Write_INI(ini);

  // The two vehicles are numbered by their place in the heap.
  EXPECT_EQ(ini.EntryCount("UNITS"), 2);
  EXPECT_FALSE(ini.HasEntry("UNITS", "007"));
}

// Read_INI() makes every house, so this fixture starts with none.
class ScenarioHousesIniTest : public GameFixture {
 protected:
  ScenarioHousesIniTest() {
    TheObjectHeaps().house().Reset(
        static_cast<int>(magic_enum::enum_count<HousesType>()));
  }
};

TEST_F(ScenarioHousesIniTest, AHouseIsWrittenAsItWasRead) {
  engine::file::IniFile read;
  LoadIniText(
      "[GoodGuy]\n"
      "Credits=50\n"
      "Edge=East\n"
      "MaxUnit=200\n"
      "MaxBuilding=180\n"
      "Allies=GoodGuy,Neutral\n",
      read);
  HouseClass::Read_INI(read);

  engine::file::IniFile written;
  HouseClass::Write_INI(written);

  EXPECT_EQ(written.GetInt("GoodGuy", "Credits", -1), 50);
  EXPECT_EQ(written.GetString("GoodGuy", "Edge", ""), "East");
  EXPECT_EQ(written.GetInt("GoodGuy", "MaxUnit", -1), 200);
  EXPECT_EQ(written.GetInt("GoodGuy", "MaxBuilding", -1), 180);
  EXPECT_EQ(written.GetString("GoodGuy", "Allies", ""), "GoodGuy,Neutral");
}

// A scenario may ask for fewer, but 150 of each is the least a house gets.
TEST_F(ScenarioHousesIniTest, TheLimitsAreRaisedToTheMinimum) {
  engine::file::IniFile read;
  LoadIniText("[BadGuy]\nMaxUnit=10\nMaxBuilding=20\n", read);
  HouseClass::Read_INI(read);

  engine::file::IniFile written;
  HouseClass::Write_INI(written);

  EXPECT_EQ(written.GetInt("BadGuy", "MaxUnit", -1), 150);
  EXPECT_EQ(written.GetInt("BadGuy", "MaxBuilding", -1), 150);
  EXPECT_EQ(written.GetString("BadGuy", "Edge", ""), "North");
}

}  // namespace
}  // namespace td
