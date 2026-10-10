// Tests TechnoClass against a real Game with no game data: two houses and
// their vehicles are built by hand.

#include "ra/objects/techno.h"

#include "base/pooled.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/text_window.h"
#include "game/sim/enums.h"
#include "game/ui/standard_colors.h"
#include "gtest/gtest.h"
#include "ra/core/defines.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/startup/game_test_util.h"

namespace ra {
namespace {

class TechnoTest : public GameFixture {
 protected:
  TechnoTest() {
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GREECE);
    base::NewPooled<HouseClass>(HOUSE_USSR);

    // Draw_It() clips to a window, and no window has a size until the game
    // lays out its screen.
    auto& window = WindowList[static_cast<int>(WINDOW_MAIN)];
    window[kWindowX] = 0;
    window[kWindowY] = 0;
    window[kWindowWidth] = kPageWidth;
    window[kWindowHeight] = kPageHeight;
  }

  // A ranger owned by `house`, in limbo.
  static UnitClass& MakeUnit(const HousesType house) {
    auto* unit = base::NewPooled<UnitClass>(UNIT_JEEP, house);
    EXPECT_NE(unit, nullptr);
    return *unit;
  }

  // Draws a selected, undamaged enemy ranger whose shape is `max_size` pixels
  // across onto a page of kBackground and returns how many pixels the health
  // bar's black frame or green interior painted.
  static int HealthBarPixels(const int max_size) {
    UnitTypeClass& type = UnitTypeClass::As_Reference(UNIT_JEEP);
    type.MaxSize = max_size;
    // Without rules a ranger has no strength, and nothing that weak has a bar.
    type.MaxStrength = 100;
    TechnoClass& unit = MakeUnit(HOUSE_USSR);
    unit.IsSelected = true;

    PixelBuffer page(kPageWidth, kPageHeight);
    page.view().Clear(kBackground);
    // The vehicle's own Draw_It() stops early without a shape to draw.
    unit.TechnoClass::Draw_It(page.view(), kPageWidth / 2, kPageHeight / 2,
                              WINDOW_MAIN);

    int painted = 0;
    for (int y = 0; y < kPageHeight; ++y) {
      for (int x = 0; x < kPageWidth; ++x) {
        const int color = page.view().GetPixel(x, y);
        if (color == game::kBlack || color == game::kLightGreen) {
          ++painted;
        }
      }
    }
    return painted;
  }

  static constexpr int kPageWidth = 640;
  static constexpr int kPageHeight = 400;
  // A colour that neither the bar nor the selection box uses.
  static constexpr unsigned char kBackground = 100;
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

// The mode ends in Record_The_Kill(), which Take_Damage() reaches through
// ObjectClass::Take_Damage().
TEST_F(TechnoTest, KillingTheUnitBeingTeleportedEndsChronosphereTargeting) {
  TechnoClass& victim = MakeUnit(HOUSE_GREECE);
  victim.House->UnitToTeleport = victim.As_Target();
  TheMap().IsTargettingMode = kSpcChrono2;

  victim.Strength = 10;
  int damage = 10;
  EXPECT_EQ(
      victim.TechnoClass::Take_Damage(damage, 0, WARHEAD_HE, nullptr, true),
      game::ResultType::RESULT_DESTROYED);

  EXPECT_EQ(TheMap().IsTargettingMode, SPC_NONE);
  EXPECT_FALSE(victim.House->UnitToTeleport.is_valid());
}

TEST_F(TechnoTest, KillingAnotherHousesTeleportUnitKeepsThePlayersTargeting) {
  const TechnoClass& own = MakeUnit(HOUSE_GREECE);
  TechnoClass& victim = MakeUnit(HOUSE_USSR);
  own.House->UnitToTeleport = own.As_Target();
  victim.House->UnitToTeleport = victim.As_Target();
  TheMap().IsTargettingMode = kSpcChrono2;

  victim.Strength = 10;
  int damage = 10;
  victim.TechnoClass::Take_Damage(damage, 0, WARHEAD_HE, nullptr, true);

  EXPECT_EQ(TheMap().IsTargettingMode, kSpcChrono2);
  EXPECT_EQ(own.House->UnitToTeleport, own.As_Target());
}

TEST_F(TechnoTest, ASelectedObjectShowsItsHealthBar) {
  // A 24-pixel shape is 18 pixels wide: an 18x4 frame around a 16x2 interior.
  EXPECT_EQ(HealthBarPixels(24), 18 * 4);
}

// The interior is two pixels narrower than the object, so an object under
// three pixels wide has no room for it.
TEST_F(TechnoTest, AnObjectTooNarrowForAPipGetsNoHealthBar) {
  EXPECT_EQ(HealthBarPixels(2), 0);  // One pixel wide.
  EXPECT_EQ(HealthBarPixels(0), 0);  // No shape loaded.
}

}  // namespace
}  // namespace ra
