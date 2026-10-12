// Tests TechnoClass against a real Game with no game data: two houses and
// their vehicles are built by hand.

#include "ra/objects/techno.h"

#include <array>
#include <cstddef>

#include "base/pooled.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/text_window.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_sink.h"
#include "game/sim/enums.h"
#include "game/ui/standard_colors.h"
#include "gtest/gtest.h"
#include "ra/core/defines.h"
#include "ra/map/world.h"
#include "ra/net/session.h"
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

  // What drawing a selected object painted.
  struct Drawn {
    // Pixels of the health bar's black frame and green interior.
    int health_bar_pixels = 0;
    // The first and last rows holding a white pixel of the selection box,
    // measured down from the row the object was drawn at.
    int box_top = 0;
    int box_bottom = 0;
  };

  // Draws a selected enemy ranger whose shape is `max_size` pixels across
  // onto a page of kBackground. It is undamaged, or has no strength left
  // when `alive` is false.
  static Drawn DrawSelected(const int max_size, const bool alive = true) {
    UnitTypeClass& type = UnitTypeClass::As_Reference(UNIT_JEEP);
    type.MaxSize = max_size;
    // Without rules a ranger has no strength, and nothing that weak has a bar.
    type.MaxStrength = 100;
    TechnoClass& unit = MakeUnit(HOUSE_USSR);
    unit.IsSelected = true;
    if (!alive) {
      unit.Strength = 0;
    }

    PixelBuffer page(kPageWidth, kPageHeight);
    page.view().Clear(kBackground);
    // The vehicle's own Draw_It() stops early without a shape to draw.
    unit.TechnoClass::Draw_It(page.view(), kPageWidth / 2, kCenterRow,
                              WINDOW_MAIN);

    Drawn drawn;
    bool has_box = false;
    for (int y = 0; y < kPageHeight; ++y) {
      for (int x = 0; x < kPageWidth; ++x) {
        const int color = page.view().GetPixel(x, y);
        if (color == game::kBlack || color == game::kLightGreen) {
          ++drawn.health_bar_pixels;
        }
        if (color == game::kWhite) {
          if (!has_box) {
            drawn.box_top = y - kCenterRow;
            has_box = true;
          }
          drawn.box_bottom = y - kCenterRow;
        }
      }
    }
    EXPECT_TRUE(has_box);
    return drawn;
  }

  // How many pixels the health bar of an undamaged object paints.
  static int HealthBarPixels(const int max_size) {
    return DrawSelected(max_size).health_bar_pixels;
  }

  static constexpr int kPageWidth = 640;
  static constexpr int kPageHeight = 400;
  static constexpr int kCenterRow = kPageHeight / 2;
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

// A ranger standing on the map is a threat to the other house in its
// region; captured, it has to become a threat to the house it was taken from
// instead, or lifting it later takes the amount from the wrong house.
TEST_F(TechnoTest, ACapturedObjectMovesItsThreatToTheOtherHouse) {
  TheSession().Type = GAME_NORMAL;
  UnitTypeClass& type = UnitTypeClass::As_Reference(UNIT_JEEP);
  const int old_risk = type.Risk;
  type.Risk = 8;
  const Cell cell = Cell::FromXY(40, 40);
  const int region = MapEditClass::Cell_Region(cell);
  HouseClass& capturer = *HouseClass::As_Pointer(HOUSE_GREECE);
  const HouseClass& loser = *HouseClass::As_Pointer(HOUSE_USSR);

  TechnoClass& unit = MakeUnit(HOUSE_USSR);
  unit.Coord = cell.Center();
  unit.IsInLimbo = false;
  ASSERT_TRUE(unit.ObjectClass::Mark(MARK_DOWN));
  ASSERT_EQ(capturer.RegionThreatAt(region), 8);
  ASSERT_EQ(loser.RegionThreatAt(region), 0);

  EXPECT_TRUE(unit.TechnoClass::Captured(&capturer));
  EXPECT_EQ(capturer.RegionThreatAt(region), 0);
  EXPECT_EQ(loser.RegionThreatAt(region), 8);
  // The regions beside it got their shares back too.
  EXPECT_EQ(capturer.RegionThreatAt(region + 1), 0);
  EXPECT_EQ(loser.RegionThreatAt(region + 1), 4);

  EXPECT_TRUE(unit.ObjectClass::Mark(MARK_UP));
  EXPECT_EQ(capturer.RegionThreatAt(region), 0);
  EXPECT_EQ(loser.RegionThreatAt(region), 0);

  // Back in limbo, as the fixture expects to find it when it ends the game.
  unit.IsInLimbo = true;
  type.Risk = old_risk;
}

// Multiplayer games keep no threat, so a capture must not invent any.
TEST_F(TechnoTest, ACaptureLeavesTheRegionsAloneOutsideTheCampaign) {
  TheSession().Type = GAME_SKIRMISH;
  UnitTypeClass& type = UnitTypeClass::As_Reference(UNIT_JEEP);
  const int old_risk = type.Risk;
  type.Risk = 8;
  const Cell cell = Cell::FromXY(40, 40);
  const int region = MapEditClass::Cell_Region(cell);
  HouseClass& capturer = *HouseClass::As_Pointer(HOUSE_GREECE);
  const HouseClass& loser = *HouseClass::As_Pointer(HOUSE_USSR);

  TechnoClass& unit = MakeUnit(HOUSE_USSR);
  unit.Coord = cell.Center();
  unit.IsInLimbo = false;
  ASSERT_TRUE(unit.ObjectClass::Mark(MARK_DOWN));

  EXPECT_TRUE(unit.TechnoClass::Captured(&capturer));
  EXPECT_EQ(capturer.RegionThreatAt(region), 0);
  EXPECT_EQ(loser.RegionThreatAt(region), 0);

  unit.IsInLimbo = true;
  type.Risk = old_risk;
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

// The top corners of the selection box move down four rows to clear the bar,
// and only then.
TEST_F(TechnoTest, TheSelectionBoxMakesRoomForTheHealthBar) {
  const Drawn drawn = DrawSelected(24);
  EXPECT_GT(drawn.health_bar_pixels, 0);
  EXPECT_EQ(drawn.box_top, 4 - drawn.box_bottom);
}

TEST_F(TechnoTest, TheSelectionBoxStaysPutWithoutAHealthBar) {
  for (const Drawn& drawn :
       {DrawSelected(2), DrawSelected(0), DrawSelected(24, /*alive=*/false)}) {
    EXPECT_EQ(drawn.health_bar_pixels, 0);
    EXPECT_EQ(drawn.box_top, -drawn.box_bottom);
  }
}

// Two rangers with strength, which a radio needs to accept contact.
class TechnoRadioTest : public TechnoTest {
 protected:
  TechnoRadioTest() {
    caller_.Strength = 1;
    callee_.Strength = 1;
  }

  TechnoClass& caller_ = MakeUnit(HOUSE_GREECE);
  TechnoClass& callee_ = MakeUnit(HOUSE_GREECE);
};

TEST_F(TechnoRadioTest, HelloPutsTwoTechnosInContact) {
  EXPECT_EQ(caller_.Transmit_Message(RADIO_HELLO, &callee_), RADIO_ROGER);

  EXPECT_EQ(caller_.Contact_With_Whom(), &callee_);
  EXPECT_EQ(callee_.Contact_With_Whom(), &caller_);
  EXPECT_TRUE(callee_.radio().InContact());
}

TEST_F(TechnoRadioTest, ADestroyedTechnoAcceptsNoContact) {
  callee_.Strength = 0;

  EXPECT_EQ(caller_.Transmit_Message(RADIO_HELLO, &callee_), RADIO_NEGATIVE);

  EXPECT_EQ(caller_.Contact_With_Whom(), nullptr);
  EXPECT_FALSE(callee_.radio().InContact());
}

TEST_F(TechnoRadioTest, OverAndOutEndsContactOnBothSides) {
  caller_.Transmit_Message(RADIO_HELLO, &callee_);

  EXPECT_EQ(caller_.Transmit_Message(RADIO_OVER_OUT), RADIO_ROGER);

  EXPECT_FALSE(caller_.radio().InContact());
  EXPECT_FALSE(callee_.radio().InContact());
}

TEST_F(TechnoRadioTest, AMessageWithNoContactIsStatic) {
  EXPECT_EQ(caller_.Transmit_Message(RADIO_REDRAW), RADIO_STATIC);
}

TEST_F(TechnoRadioTest, ARedrawRequestIsAcknowledged) {
  EXPECT_EQ(caller_.Transmit_Message(RADIO_REDRAW, &callee_), RADIO_ROGER);
}

TEST_F(TechnoRadioTest, AMessageNoClassHandlesIsStatic) {
  EXPECT_EQ(caller_.Transmit_Message(RADIO_CANT, &callee_), RADIO_STATIC);
}

TEST_F(TechnoRadioTest, GoingIntoLimboEndsContact) {
  caller_.Transmit_Message(RADIO_HELLO, &callee_);
  // Limbo() only hangs up for an object that is on the map; the base class
  // then finds it was never placed and does nothing more.
  caller_.IsInLimbo = false;

  caller_.Limbo();

  EXPECT_FALSE(caller_.radio().InContact());
  EXPECT_FALSE(callee_.radio().InContact());
}

// A vehicle with strength, which an object needs to perform a mission.
class TechnoMissionTest : public TechnoTest {
 protected:
  TechnoMissionTest() { unit_.Strength = 1; }

  // The mission state as a saved game holds it: it changes when the mission
  // function has run, because that restarts the timer.
  std::array<std::byte, 64> SavedMission() {
    std::array<std::byte, 64> bytes{};
    SpanSink sink(bytes);
    ArchiveWriter writer(sink);
    writer(unit_.mission_state());
    return bytes;
  }

  TechnoClass& unit_ = MakeUnit(HOUSE_GREECE);
};

TEST_F(TechnoMissionTest, AnAssignedMissionIsQueuedUntilItCommences) {
  unit_.Assign_Mission(MISSION_HARVEST);
  EXPECT_EQ(unit_.mission_state().current(), MISSION_NONE);
  EXPECT_EQ(unit_.Get_Mission(), MISSION_HARVEST);

  EXPECT_TRUE(unit_.Commence());
  EXPECT_EQ(unit_.mission_state().current(), MISSION_HARVEST);
  EXPECT_FALSE(unit_.Commence());
}

TEST_F(TechnoMissionTest, SetMissionTakesEffectAtOnce) {
  unit_.Assign_Mission(MISSION_HARVEST);
  unit_.Set_Mission(MISSION_GUARD);
  EXPECT_EQ(unit_.mission_state().current(), MISSION_GUARD);
  EXPECT_EQ(unit_.mission_state().queued(), MISSION_NONE);
}

TEST_F(TechnoMissionTest, AnOverriddenMissionComesBackOnRestore) {
  unit_.Assign_Mission(MISSION_HARVEST);
  unit_.Commence();

  unit_.Override_Mission(MISSION_STOP, Target{}, Target{});
  EXPECT_EQ(unit_.mission_state().suspended(), MISSION_HARVEST);
  EXPECT_EQ(unit_.mission_state().queued(), MISSION_STOP);
  unit_.Commence();

  EXPECT_TRUE(unit_.Restore_Mission());
  EXPECT_EQ(unit_.mission_state().queued(), MISSION_HARVEST);
  EXPECT_EQ(unit_.mission_state().suspended(), MISSION_NONE);
  EXPECT_FALSE(unit_.Restore_Mission());
}

TEST_F(TechnoMissionTest, AnOverrideSetsAsideTheQueuedMissionFirst) {
  unit_.Assign_Mission(MISSION_HARVEST);
  unit_.Commence();
  unit_.Assign_Mission(MISSION_GUARD);

  unit_.Override_Mission(MISSION_STOP, Target{}, Target{});

  EXPECT_EQ(unit_.mission_state().suspended(), MISSION_GUARD);
}

// TechnoClass::AI() is called directly: the vehicle's own would drive it.
TEST_F(TechnoMissionTest, AIRunsTheMissionOfALivingObject) {
  const auto before = SavedMission();
  unit_.TechnoClass::AI();
  EXPECT_NE(SavedMission(), before);
}

TEST_F(TechnoMissionTest, AIRunsNoMissionForADestroyedObject) {
  unit_.Strength = 0;
  const auto before = SavedMission();
  unit_.TechnoClass::AI();
  EXPECT_EQ(SavedMission(), before);
}

}  // namespace
}  // namespace ra
