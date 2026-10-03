// Tests that the scenario writers put the same entries into an IniFile that
// their readers take out of one, so a scenario the editor saves reads back as
// it was.

#include <algorithm>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/pooled.h"
#include "engine/file/ini_file.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "game/core/object_heap.h"
#include "gtest/gtest.h"
#include "td/ai/prebuilt_base.h"
#include "td/ai/teamtype.h"
#include "td/ai/trigger.h"
#include "td/core/const.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/map/world.h"
#include "td/objects/cell.h"
#include "td/objects/overlay.h"
#include "td/objects/smudge.h"
#include "td/objects/template.h"
#include "td/objects/type.h"

namespace td {

namespace {

using engine::file::IniFile;

constexpr std::string_view kScenarioText =
    "[Basic]\r\n"
    "Name=Test\r\n"
    "Player=GoodGuy\r\n"
    "Intro=x\r\n"
    "[TeamTypes]\r\n"
    "alpha=None,1,0,1,0,1,5,3,2,4,1,E1:3,2,Move:3,Attack Base:0,1,0\r\n"
    "[Waypoints]\r\n"
    "0=100\r\n"
    "1=-1\r\n"
    "[UNITS]\r\n"
    "000=GoodGuy,MTNK,256,1234,64,Guard,None\r\n";

// Loads `text` into a fresh database, as the scenario reader does.
void Parse(std::string_view text, IniFile& ini) {
  SpanSource source(std::as_bytes(std::span(text)));
  ASSERT_TRUE(ini.Load(source));
}

// Returns the text `ini` saves.
std::string SaveToString(const IniFile& ini) {
  std::vector<std::byte> bytes;
  VectorSink sink(bytes);
  EXPECT_TRUE(ini.Save(sink));
  std::string text;
  for (const std::byte byte : bytes) {
    text.push_back(static_cast<char>(byte));
  }
  return text;
}

// Expects both databases to hold the same sections, entries and values in the
// same order.
void ExpectSameContents(const IniFile& expected, const IniFile& actual) {
  ASSERT_EQ(actual.SectionCount(), expected.SectionCount());
  for (int s = 0; s < expected.SectionCount(); ++s) {
    const std::string section = expected.SectionName(s);
    ASSERT_STREQ(actual.SectionName(s), section.c_str());
    ASSERT_EQ(actual.EntryCount(section), expected.EntryCount(section));
    for (int e = 0; e < expected.EntryCount(section); ++e) {
      const std::string entry = expected.EntryName(section, e);
      EXPECT_STREQ(actual.EntryName(section, e), entry.c_str());
      EXPECT_EQ(actual.GetString(section, entry, "?"),
                expected.GetString(section, entry, "!"))
          << "[" << section << "] " << entry;
    }
  }
}

TEST(ScenarioIniTest, SavedScenarioReadsBackIdentically) {
  IniFile first;
  Parse(kScenarioText, first);
  const std::string saved = SaveToString(first);

  IniFile second;
  Parse(saved, second);
  ExpectSameContents(first, second);
}

// The writers work on real object heaps, which the tests of this suite make
// and empty.
class ScenarioWriterTest : public testing::Test {
 protected:
  void SetUp() override {
    team_type_capacity_ = TheObjectHeaps().team_type().Capacity();
    trigger_capacity_ = TheObjectHeaps().trigger().Capacity();
    team_capacity_ = TheObjectHeaps().team().Capacity();
    TheObjectHeaps().team_type().Reset(8);
    TheObjectHeaps().trigger().Reset(8);
    TheObjectHeaps().team().Reset(8);
  }
  // Runs even when an ASSERT returned early, so nothing leaks into the next
  // test or suite.
  void TearDown() override {
    while (TheObjectHeaps().trigger().Count() != 0) {
      base::DeletePooled(TheObjectHeaps().trigger().ActiveAt(0));
    }
    while (TheObjectHeaps().team_type().Count() != 0) {
      base::DeletePooled(TheObjectHeaps().team_type().ActiveAt(0));
    }
    TheObjectHeaps().team_type().Reset(team_type_capacity_);
    TheObjectHeaps().trigger().Reset(trigger_capacity_);
    TheObjectHeaps().team().Reset(team_capacity_);
  }

 private:
  int team_type_capacity_ = 0;
  int trigger_capacity_ = 0;
  int team_capacity_ = 0;
};

TEST_F(ScenarioWriterTest, TeamTypesWriteBackWhatTheyRead) {
  IniFile scenario;
  Parse(kScenarioText, scenario);
  TeamTypeClass::Read_INI(scenario);
  ASSERT_EQ(TheObjectHeaps().team_type().Count(), 1);

  IniFile written;
  TeamTypeClass::Write_INI(written, true);

  ASSERT_EQ(written.EntryCount("TeamTypes"), 1);
  EXPECT_STREQ(written.EntryName("TeamTypes", 0), "alpha");
  EXPECT_EQ(written.GetString("TeamTypes", "alpha", ""),
            scenario.GetString("TeamTypes", "alpha", "!"));
}

TEST_F(ScenarioWriterTest, TriggersWriteBackWhatTheyRead) {
  IniFile scenario;
  Parse(std::string(kScenarioText) +
            "[Triggers]\r\n"
            "trig=Time,Win,5,None,alpha,1\r\n"
            "tr2=Attacked,Lose,0,None,None,0\r\n",
        scenario);
  TeamTypeClass::Read_INI(scenario);
  TriggerClass::Read_INI(scenario);
  ASSERT_EQ(TheObjectHeaps().trigger().Count(), 2);

  IniFile written;
  TriggerClass::Write_INI(written, true);

  ASSERT_EQ(written.EntryCount("Triggers"), 2);
  for (const char* name : {"trig", "tr2"}) {
    EXPECT_EQ(written.GetString("Triggers", name, ""),
              scenario.GetString("Triggers", name, "!"))
        << name;
  }
}

// The editor writes into the scenario's own database, so what it does not
// own must survive and what it does own must not leave stale entries.
TEST_F(ScenarioWriterTest, WritingKeepsUnownedKeysAndDropsStaleOwnedOnes) {
  IniFile scenario;
  Parse(kScenarioText, scenario);
  TeamTypeClass::Read_INI(scenario);

  IniFile target;
  Parse(std::string(kScenarioText) +
            "[Teams]\r\nold=1\r\n[TeamTypes]\r\nstale=None,0\r\n",
        target);
  TeamTypeClass::Write_INI(target, true);

  EXPECT_EQ(target.GetString("Basic", "Name", ""), "Test");
  EXPECT_EQ(target.GetString("UNITS", "000", ""),
            "GoodGuy,MTNK,256,1234,64,Guard,None");
  EXPECT_FALSE(target.HasSection("Teams"));
  EXPECT_FALSE(target.HasEntry("TeamTypes", "stale"));
  EXPECT_TRUE(target.HasEntry("TeamTypes", "alpha"));
}

// The master list is updated, not replaced: refresh is off.
TEST_F(ScenarioWriterTest, WritingWithoutRefreshKeepsTheEntriesThere) {
  IniFile scenario;
  Parse(kScenarioText, scenario);
  TeamTypeClass::Read_INI(scenario);

  IniFile master;
  Parse("[TeamTypes]\r\nbeta=None,0\r\nalpha=None,0\r\n", master);
  TeamTypeClass::Write_INI(master, false);

  EXPECT_EQ(master.EntryCount("TeamTypes"), 2);
  EXPECT_EQ(master.GetString("TeamTypes", "beta", ""), "None,0");
  EXPECT_EQ(master.GetString("TeamTypes", "alpha", ""),
            scenario.GetString("TeamTypes", "alpha", "!"));
}

// The map's writers need its cells; the readers' sections hold one entry per
// cell that carries something.
class ScenarioMapWriterTest : public ScenarioWriterTest {
 protected:
  static void SetUpTestSuite() {
    TheMap().Replace_Cells(MAP_CELL_TOTAL);
    TheWorld().cell_triggers().resize(MAP_CELL_TOTAL);
    TheMap().Init_Cells();
  }
  static void TearDownTestSuite() {
    TheWorld().cell_triggers().clear();
    TheMap().Free_Cells();
  }
  void TearDown() override {
    TheMap().Init_Cells();
    std::ranges::fill(TheWorld().cell_triggers(), nullptr);
    base::At(TheWorld().waypoint(), 3) = Cell{};
    TheMap().Theater = THEATER_NONE;
    TheMap().MapCellX = 0;
    TheMap().MapCellY = 0;
    TheMap().MapCellWidth = 0;
    TheMap().MapCellHeight = 0;
    ScenarioWriterTest::TearDown();
  }
};

TEST_F(ScenarioMapWriterTest, MapWritesItsParametersWaypointsAndCellTriggers) {
  IniFile scenario;
  Parse(std::string(kScenarioText) +
            "[Triggers]\r\ntrig=Time,Win,5,None,None,1\r\n",
        scenario);
  TriggerClass::Read_INI(scenario);
  ASSERT_EQ(TheObjectHeaps().trigger().Count(), 1);
  TriggerClass* trigger = TheObjectHeaps().trigger().ActiveAt(0);
  TheMap().Theater = THEATER_WINTER;
  TheMap().MapCellX = 3;
  TheMap().MapCellY = 4;
  TheMap().MapCellWidth = 20;
  TheMap().MapCellHeight = 21;
  TheMap().at(Cell::FromIndex(10)).IsTrigger = true;
  TheWorld().cell_triggers().at(10) = trigger;
  base::At(TheWorld().waypoint(), 3) = Cell::FromIndex(777);

  IniFile written;
  written.PutString("CellTriggers", "99", "stale");
  TheMap().Write_INI(written);

  EXPECT_EQ(written.GetString("MAP", "Theater", ""),
            Theaters.at(THEATER_WINTER).Name);
  EXPECT_EQ(written.GetInt("MAP", "X", -2), 3);
  EXPECT_EQ(written.GetInt("MAP", "Y", -2), 4);
  EXPECT_EQ(written.GetInt("MAP", "Width", -2), 20);
  EXPECT_EQ(written.GetInt("MAP", "Height", -2), 21);
  EXPECT_EQ(written.EntryCount("Waypoints"), kWayptCount);
  EXPECT_EQ(written.GetInt("Waypoints", "3", -2), 777);
  EXPECT_EQ(written.GetInt("Waypoints", "4", -2), Cell{}.index());
  EXPECT_EQ(written.EntryCount("CellTriggers"), 1);
  EXPECT_EQ(written.GetString("CellTriggers", "10", ""), "trig");
}

TEST_F(ScenarioMapWriterTest, OverlaysAndTemplatesWriteOneEntryPerCell) {
  TheMap().at(Cell::FromIndex(5)).Overlay = OVERLAY_CONCRETE;
  TheMap().at(Cell::FromIndex(7)).TType = TEMPLATE_CLEAR1;
  TheMap().at(Cell::FromIndex(7)).TIcon = 0;

  IniFile written;
  written.PutString("OVERLAY", "999", "STALE");
  OverlayClass::Write_INI(written);
  TemplateClass::Write_INI(written);

  EXPECT_EQ(written.EntryCount("OVERLAY"), 1);
  EXPECT_EQ(written.GetString("OVERLAY", "005", ""),
            OverlayTypeClass::As_Reference(OVERLAY_CONCRETE).IniName);
  EXPECT_EQ(written.EntryCount("TEMPLATE"), 1);
  EXPECT_EQ(written.GetString("TEMPLATE", "007", ""),
            TemplateTypeClass::As_Reference(TEMPLATE_CLEAR1).IniName);
}

// The bib is part of the building that stands on it, so it is not written.
TEST_F(ScenarioMapWriterTest, SmudgesWriteTheirTypeCellAndDataButNotBibs) {
  ASSERT_FALSE(SmudgeTypeClass::As_Reference(SMUDGE_CRATER1).IsBib);
  ASSERT_TRUE(SmudgeTypeClass::As_Reference(SMUDGE_BIB1).IsBib);
  TheMap().at(Cell::FromIndex(9)).Smudge = SMUDGE_CRATER1;
  TheMap().at(Cell::FromIndex(9)).SmudgeData = 3;
  TheMap().at(Cell::FromIndex(11)).Smudge = SMUDGE_BIB1;

  IniFile written;
  written.PutString("SMUDGE", "999", "STALE,999,0");
  SmudgeClass::Write_INI(written);

  EXPECT_EQ(written.EntryCount("SMUDGE"), 1);
  EXPECT_EQ(
      written.GetString("SMUDGE", "009", ""),
      absl::StrFormat("%s,9,3",
                      SmudgeTypeClass::As_Reference(SMUDGE_CRATER1).IniName));
  EXPECT_FALSE(written.HasEntry("SMUDGE", "011"));
}

TEST(ScenarioIniBaseTest, PrebuiltBaseWritesBackWhatItReads) {
  const int first = Cell::FromIndex((MAP_CELL_W * 20) + 20).Center().packed();
  const int second = Cell::FromIndex((MAP_CELL_W * 22) + 25).Center().packed();
  IniFile scenario;
  Parse(absl::StrFormat(
            "[Base]\r\nCount=2\r\n000=%s,%d\r\n001=%s,%d\r\n",
            BuildingTypeClass::As_Reference(STRUCT_CONST).IniName, first,
            BuildingTypeClass::As_Reference(STRUCT_POWER).IniName, second),
        scenario);
  PrebuiltBase base;
  base.ReadIni(scenario);
  ASSERT_EQ(base.nodes().size(), 2U);

  IniFile written;
  written.PutString("Base", "007", "STALE,1");
  base.WriteIni(written);

  ASSERT_EQ(written.EntryCount("Base"), 3);
  for (const char* entry : {"Count", "000", "001"}) {
    EXPECT_EQ(written.GetString("Base", entry, ""),
              scenario.GetString("Base", entry, "!"))
        << entry;
  }
}

}  // namespace

}  // namespace td
