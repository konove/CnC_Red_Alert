// Tests for the team types in a real Game with no game data: how one reads
// and writes its scenario INI entry.

#include "td/ai/teamtype.h"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include "engine/file/ini_file.h"
#include "engine/stream/span_source.h"
#include "gtest/gtest.h"
#include "td/core/object_heaps.h"
#include "td/startup/game_test_util.h"

namespace td {
namespace {

using engine::file::IniFile;

// A Game with room for four team types and four teams.
class TeamTypeTest : public GameFixture {
 protected:
  TeamTypeTest() {
    TheObjectHeaps().team().Reset(4);
    TheObjectHeaps().team_type().Reset(4);
  }

  // Loads `text` into `ini`, as the scenario reader does.
  static void Parse(const std::string_view text, IniFile& ini) {
    SpanSource source(std::as_bytes(std::span(text)));
    ASSERT_TRUE(ini.Load(source));
  }
};

TEST_F(TeamTypeTest, EntriesPastTheHeapsCapacityAreDropped) {
  TheObjectHeaps().team_type().Reset(2);
  IniFile ini;
  Parse(
      "[TeamTypes]\r\n"
      "one=GoodGuy,0,0,0,0,0,7,1,0,0,0,0\r\n"
      "two=GoodGuy,0,0,0,0,0,7,1,0,0,0,0\r\n"
      "three=GoodGuy,0,0,0,0,0,7,1,0,0,0,0\r\n",
      ini);

  TeamTypeClass::Read_INI(ini);

  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 2);
  EXPECT_NE(TeamTypeClass::As_Pointer("two"), nullptr);
  EXPECT_EQ(TeamTypeClass::As_Pointer("three"), nullptr);
}

TEST_F(TeamTypeTest, OldFormatEntriesPastTheHeapsCapacityAreDropped) {
  TheObjectHeaps().team_type().Reset(1);
  IniFile ini;
  Parse(
      "[Teams]\r\n"
      "one=GoodGuy,0,0,0,0,0,7,1,0,Hunt,E1:2\r\n"
      "two=GoodGuy,0,0,0,0,0,7,1,0,Hunt,E1:2\r\n",
      ini);

  TeamTypeClass::Read_INI(ini);

  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 1);
}

// The scenario need not have the house a team type is written for.
TEST_F(TeamTypeTest, ATeamTypeIsWrittenWithItsHousesNameWithoutThatHouse) {
  IniFile ini;
  Parse(
      "[TeamTypes]\r\n"
      "one=BadGuy,0,0,0,0,0,7,1,0,0,0,0,1,1\r\n",
      ini);
  TeamTypeClass::Read_INI(ini);
  ASSERT_EQ(TheObjectHeaps().team_type().Count(), 1);

  IniFile written;
  TeamTypeClass::Write_INI(written, true);

  EXPECT_EQ(written.GetString("TeamTypes", "one", ""),
            "BadGuy,0,0,0,0,0,7,1,0,0,0,0,1,1");
}

}  // namespace
}  // namespace td
