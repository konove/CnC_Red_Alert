// Tests TechnoClass against a real Game with no game data: two houses and a
// vehicle are built by hand.

#include "td/objects/techno.h"

#include "base/pooled.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/text_window.h"
#include "game/ui/standard_colors.h"
#include "gtest/gtest.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/map/world.h"
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

}  // namespace
}  // namespace td
