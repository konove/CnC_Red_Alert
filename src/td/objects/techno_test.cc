// Tests TechnoClass against a real Game with no game data: two houses and a
// vehicle are built by hand.

#include "td/objects/techno.h"

#include <array>
#include <cstddef>

#include "base/pooled.h"
#include "engine/gfx/palette_index.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/text_window.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_sink.h"
#include "game/ui/standard_colors.h"
#include "gtest/gtest.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/editor/mapedit.h"
#include "td/map/world.h"
#include "td/net/session.h"
#include "td/objects/house.h"
#include "td/objects/type.h"
#include "td/objects/unit.h"
#include "td/startup/game_test_util.h"

namespace td {
namespace {

class TechnoTest : public GameFixture {
 protected:
  TechnoTest() {
    TheObjectHeaps().house().Reset(2);
    TheObjectHeaps().unit().Reset(4);
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GOOD);
    base::NewPooled<HouseClass>(HOUSE_BAD);

    // Draw_It() clips to a window, and no window has a size until the game
    // lays out its screen. Tiberian Dawn keeps the columns in characters.
    auto& window = WindowList[static_cast<int>(WINDOW_MAIN)];
    window[kWindowX] = 0;
    window[kWindowY] = 0;
    window[kWindowWidth] = kPageWidth / 8;
    window[kWindowHeight] = kPageHeight;
  }

  // The type table is static and outlives the Game.
  void TearDown() override {
    UnitTypeClass::As_Reference(UNIT_LTANK).Set_Max_Size(0);
    GameFixture::TearDown();
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

  // Draws a selected enemy light tank whose shape is `max_size` pixels across
  // onto a page of kBackground. It is undamaged, or has no strength left
  // when `alive` is false.
  static Drawn DrawSelected(const int max_size, const bool alive = true) {
    UnitTypeClass::As_Reference(UNIT_LTANK).Set_Max_Size(max_size);
    auto* made = base::NewPooled<UnitClass>(UNIT_LTANK, HOUSE_BAD);
    EXPECT_NE(made, nullptr);
    TechnoClass& unit = *made;
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
        const engine::gfx::PaletteIndex color = page.view().GetPixel(x, y);
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

  static constexpr int kPageWidth = 320;
  static constexpr int kPageHeight = 200;
  static constexpr int kCenterRow = kPageHeight / 2;
  // A colour that neither the bar nor the selection box uses.
  static constexpr unsigned char kBackground = 100;
};

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

// A light tank standing on the map is a threat to the other house in its
// region; captured, it has to become a threat to the house it was taken from
// instead, or lifting it later takes the amount from the wrong house.
TEST_F(TechnoTest, ACapturedObjectMovesItsThreatToTheOtherHouse) {
  TheSession().type() = GAME_NORMAL;
  const Cell cell = Cell::FromXY(40, 40);
  const int region = MapEditClass::Cell_Region(cell);
  HouseClass& capturer = *HouseClass::As_Pointer(HOUSE_GOOD);
  const HouseClass& loser = *HouseClass::As_Pointer(HOUSE_BAD);

  TechnoClass& unit = *base::NewPooled<UnitClass>(UNIT_LTANK, HOUSE_BAD);
  const int risk = unit.Risk();
  ASSERT_GT(risk, 0);
  unit.Coord = cell.Center();
  unit.IsInLimbo = false;
  ASSERT_TRUE(unit.ObjectClass::Mark(MARK_DOWN));
  ASSERT_EQ(capturer.RegionThreatAt(region), risk);
  ASSERT_EQ(loser.RegionThreatAt(region), 0);

  EXPECT_TRUE(unit.TechnoClass::Captured(&capturer));
  EXPECT_EQ(capturer.RegionThreatAt(region), 0);
  EXPECT_EQ(loser.RegionThreatAt(region), risk);
  // The regions beside it got their shares back too.
  EXPECT_EQ(capturer.RegionThreatAt(region + 1), 0);
  EXPECT_EQ(loser.RegionThreatAt(region + 1), risk / 2);

  EXPECT_TRUE(unit.ObjectClass::Mark(MARK_UP));
  EXPECT_EQ(capturer.RegionThreatAt(region), 0);
  EXPECT_EQ(loser.RegionThreatAt(region), 0);

  // Back in limbo, as the fixture expects to find it when it ends the game.
  unit.IsInLimbo = true;
}

// Multiplayer games keep no threat, so a capture must not invent any.
TEST_F(TechnoTest, ACaptureLeavesTheRegionsAloneOutsideTheCampaign) {
  TheSession().type() = GAME_MODEM;
  const Cell cell = Cell::FromXY(40, 40);
  const int region = MapEditClass::Cell_Region(cell);
  HouseClass& capturer = *HouseClass::As_Pointer(HOUSE_GOOD);
  const HouseClass& loser = *HouseClass::As_Pointer(HOUSE_BAD);

  TechnoClass& unit = *base::NewPooled<UnitClass>(UNIT_LTANK, HOUSE_BAD);
  unit.Coord = cell.Center();
  unit.IsInLimbo = false;
  ASSERT_TRUE(unit.ObjectClass::Mark(MARK_DOWN));

  EXPECT_TRUE(unit.TechnoClass::Captured(&capturer));
  EXPECT_EQ(capturer.RegionThreatAt(region), 0);
  EXPECT_EQ(loser.RegionThreatAt(region), 0);

  unit.IsInLimbo = true;
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

  TechnoClass& unit_ = *base::NewPooled<UnitClass>(UNIT_LTANK, HOUSE_GOOD);
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

TEST_F(TechnoMissionTest, HelloPutsTwoTechnosInContact) {
  TechnoClass& other = *base::NewPooled<UnitClass>(UNIT_LTANK, HOUSE_GOOD);
  other.Strength = 1;

  EXPECT_EQ(unit_.Transmit_Message(RADIO_HELLO, &other), RADIO_ROGER);
  EXPECT_EQ(unit_.Contact_With_Whom(), &other);
  EXPECT_EQ(other.Contact_With_Whom(), &unit_);

  EXPECT_EQ(unit_.Transmit_Message(RADIO_OVER_OUT), RADIO_ROGER);
  EXPECT_FALSE(unit_.radio().InContact());
  EXPECT_FALSE(other.radio().InContact());
}

TEST_F(TechnoMissionTest, ADestroyedTechnoAcceptsNoContact) {
  TechnoClass& other = *base::NewPooled<UnitClass>(UNIT_LTANK, HOUSE_GOOD);
  other.Strength = 0;

  EXPECT_EQ(unit_.Transmit_Message(RADIO_HELLO, &other), RADIO_NEGATIVE);
  EXPECT_FALSE(unit_.radio().InContact());
}

}  // namespace
}  // namespace td
