// Tests how a warhead reads its section of the rules, against a real Game
// whose warhead heap holds the one warhead under test.

#include "ra/objects/warhead.h"

#include "base/fixed.h"
#include "base/pooled.h"
#include "engine/file/ini_file.h"
#include "game/sim/enums.h"
#include "gtest/gtest.h"
#include "ra/core/type_heaps.h"
#include "ra/startup/game_test_util.h"

namespace ra {
namespace {

constexpr const char* kName = "AP";

class WarheadTest : public GameFixture {
 protected:
  WarheadTest() : warhead_(MakeWarhead()) {}

  // Makes room in the type heap for the warhead it returns.
  static WarheadTypeClass* MakeWarhead() {
    TheTypeHeaps().warhead().Reset(1);
    return base::NewPooled<WarheadTypeClass>(kName);
  }

  // Reads a rules file whose [AP] section holds `verses`.
  bool ReadVerses(const char* verses) {
    engine::file::IniFile ini;
    EXPECT_TRUE(ini.PutString(kName, "Verses", verses));
    return warhead_->Read_INI(ini);
  }

  [[nodiscard]] Fixed Modifier(game::ArmorType armor) const {
    return warhead_->Modifier.at(armor);
  }

  WarheadTypeClass* warhead_;  // owned by the type heap
};

TEST_F(WarheadTest, DoesFullDamageToEveryArmorUnread) {
  EXPECT_EQ(Modifier(game::ARMOR_NONE), Fixed(1));
  EXPECT_EQ(Modifier(game::ARMOR_CONCRETE), Fixed(1));
}

TEST_F(WarheadTest, MissingSectionIsNotRead) {
  engine::file::IniFile ini;
  EXPECT_FALSE(warhead_->Read_INI(ini));
}

TEST_F(WarheadTest, ReadsOneModifierPerArmor) {
  EXPECT_TRUE(ReadVerses("30%,75%,75%,100%,50%"));

  EXPECT_EQ(Modifier(game::ARMOR_NONE), Fixed(30, 100));
  EXPECT_EQ(Modifier(game::ARMOR_WOOD), Fixed(75, 100));
  EXPECT_EQ(Modifier(game::ARMOR_ALUMINUM), Fixed(75, 100));
  EXPECT_EQ(Modifier(game::ARMOR_STEEL), Fixed(1));
  EXPECT_EQ(Modifier(game::ARMOR_CONCRETE), Fixed(50, 100));
}

TEST_F(WarheadTest, ShortVersesKeepsTheOtherModifiers) {
  ASSERT_TRUE(ReadVerses("30%,75%,75%,100%,50%"));

  EXPECT_TRUE(ReadVerses("90%,20%"));

  EXPECT_EQ(Modifier(game::ARMOR_NONE), Fixed(90, 100));
  EXPECT_EQ(Modifier(game::ARMOR_WOOD), Fixed(20, 100));
  EXPECT_EQ(Modifier(game::ARMOR_ALUMINUM), Fixed(75, 100));
  EXPECT_EQ(Modifier(game::ARMOR_STEEL), Fixed(1));
  EXPECT_EQ(Modifier(game::ARMOR_CONCRETE), Fixed(50, 100));
}

TEST_F(WarheadTest, ShortVersesLeavesFullDamageOnAFreshWarhead) {
  EXPECT_TRUE(ReadVerses("40%"));

  EXPECT_EQ(Modifier(game::ARMOR_NONE), Fixed(40, 100));
  EXPECT_EQ(Modifier(game::ARMOR_STEEL), Fixed(1));
  EXPECT_FALSE(warhead_->IsOrganic);
}

TEST_F(WarheadTest, SectionWithoutVersesKeepsEveryModifier) {
  ASSERT_TRUE(ReadVerses("30%,75%,75%,0%,50%"));
  engine::file::IniFile ini;
  ASSERT_TRUE(ini.PutInt(kName, "Spread", 6));

  EXPECT_TRUE(warhead_->Read_INI(ini));

  EXPECT_EQ(warhead_->SpreadFactor, 6);
  EXPECT_EQ(Modifier(game::ARMOR_NONE), Fixed(30, 100));
  EXPECT_EQ(Modifier(game::ARMOR_CONCRETE), Fixed(50, 100));
  EXPECT_TRUE(warhead_->IsOrganic);
}

TEST_F(WarheadTest, NoDamageToSteelMakesTheWarheadOrganic) {
  EXPECT_TRUE(ReadVerses("100%,100%,100%,0%,100%"));

  EXPECT_TRUE(warhead_->IsOrganic);
}

}  // namespace
}  // namespace ra
