// Tests TechnoClass against a real Game with no game data: two houses and
// their vehicles are built by hand.

#include "ra/objects/techno.h"

#include "base/pooled.h"
#include "gtest/gtest.h"
#include "ra/core/defines.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/unit.h"
#include "ra/startup/game_test_util.h"

namespace ra {
namespace {

class TechnoTest : public GameFixture {
 protected:
  TechnoTest() {
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GREECE);
    base::NewPooled<HouseClass>(HOUSE_USSR);
  }

  // A ranger owned by `house`, in limbo.
  static UnitClass& MakeUnit(const HousesType house) {
    auto* unit = base::NewPooled<UnitClass>(UNIT_JEEP, house);
    EXPECT_NE(unit, nullptr);
    return *unit;
  }
};

TEST_F(TechnoTest, AKillIsCreditedToTheKiller) {
  TechnoClass& victim = MakeUnit(HOUSE_GREECE);
  TechnoClass& killer = MakeUnit(HOUSE_USSR);

  victim.Record_The_Kill(&killer);

  EXPECT_EQ(killer.kills(), 1);
  EXPECT_EQ(victim.kills(), 0);
  EXPECT_EQ(killer.House->UnitsKilled.at(HOUSE_GREECE), 1);
  EXPECT_EQ(victim.House->UnitsLost, 1);
}

TEST_F(TechnoTest, ADeathWithNoKillerCreditsNobody) {
  TechnoClass& victim = MakeUnit(HOUSE_GREECE);
  const TechnoClass& bystander = MakeUnit(HOUSE_USSR);

  victim.Record_The_Kill(nullptr);

  EXPECT_EQ(bystander.kills(), 0);
  EXPECT_EQ(victim.kills(), 0);
  EXPECT_EQ(victim.House->UnitsLost, 1);
}

}  // namespace
}  // namespace ra
