#include "game/ui/checkbox.h"

#include <cstddef>
#include <span>

#include "base/installed.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "game/ui/edit.h"
#include "game/ui/gadget.h"
#include "game/ui/input.h"
#include "gtest/gtest.h"

namespace game {

// Draws nothing; the check box's logic is what is under test.
struct CheckBoxTestSkin {
  using TextColor = int;
  using Font = int;
  using PrintFlags = int;
  struct LabelData {};
  static constexpr int kTextNone = 0;
  static constexpr Font kButtonDefaultFont = 0;
  static constexpr PrintFlags kButtonDefaultFlags = 0;
  static constexpr int kGaugeThumbPixels = 4;
  static constexpr int kEditHeightPad = 1;
  static constexpr bool kListOutOfRangeSelectsFirst = false;
  static void Draw_Shape(PixelView& /*view*/, std::span<const std::byte> /*s*/,
                         int /*frame*/, int /*x*/, int /*y*/) {}
  static void Print_Label(PixelView& /*view*/, const char* /*text*/, int /*x*/,
                          int /*y*/, TextColor /*color*/, Font /*font*/,
                          PrintFlags /*f*/) {}
  static void Print_Clipped(PixelView& /*view*/, const char* /*text*/,
                            int /*x*/, int /*y*/, TextColor /*color*/,
                            Font /*font*/, PrintFlags /*f*/, int /*width*/) {}
  static void Draw_Check_Box(PixelView& /*view*/, int /*x*/, int /*y*/,
                             int /*w*/, int /*h*/, bool /*on*/) {}
  static const char* Text_String(int /*id*/) { return ""; }
  static int Text_Width(Font /*font*/, PrintFlags /*f*/, const char* /*text*/) {
    return 0;
  }
  static int Text_Height(Font /*font*/, PrintFlags /*f*/) { return 0; }
  static void Draw_Button_Box(PixelView& /*view*/, int /*x*/, int /*y*/,
                              int /*w*/, int /*h*/, Font /*font*/,
                              PrintFlags /*f*/, bool /*disabled*/,
                              bool /*pressed*/) {}
  static void Print_Button_Text(PixelView& /*view*/, const char* /*text*/,
                                int /*x*/, int /*y*/, Font /*font*/,
                                PrintFlags /*f*/, bool /*disabled*/,
                                bool /*pressed*/, bool /*on*/) {}
  static int Scale(int amount, int numerator, int denominator) {
    return denominator == 0 ? 0 : amount * numerator / denominator;
  }
  static void Draw_Gauge_Box(PixelView& /*view*/, int /*x*/, int /*y*/,
                             int /*w*/, int /*h*/, bool /*raised*/) {}
  static unsigned char Gauge_Fill_Color() { return 0; }
  static FontStyle FontStyleFor(Font /*font*/, PrintFlags /*f*/) { return {}; }
  static PrintFlags Edit_Flags(PrintFlags flags) { return flags; }
  static TextColor Edit_Default_Color() { return 0; }
  static void Draw_Edit_Box(PixelView& /*view*/, int /*x*/, int /*y*/,
                            int /*w*/, int /*h*/) {}
  static void Draw_Edit_Text(PixelView& /*view*/, const char* /*text*/,
                             int /*x*/, int /*y*/, int /*w*/,
                             TextColor /*color*/, Font /*font*/,
                             PrintFlags /*f*/, bool /*focused*/,
                             int /*max_length*/) {}
  static void Draw_List_Box(PixelView& /*view*/, int /*x*/, int /*y*/,
                            int /*w*/, int /*h*/) {}
  static void Draw_List_Entry(PixelView& /*view*/, const char* /*text*/,
                              int /*x*/, int /*y*/, int /*w*/,
                              int /*line_height*/, Font /*font*/,
                              PrintFlags /*f*/, bool /*selected*/,
                              std::span<const int> /*tabs*/) {}
};

namespace {

constexpr unsigned kId = 7;

// Exposes Action(), which GadgetClass::Input() calls.
class TestCheckBox : public CheckBoxClass<CheckBoxTestSkin> {
 public:
  TestCheckBox() : CheckBoxClass<CheckBoxTestSkin>(kId, 10, 10) {}
  using CheckBoxClass<CheckBoxTestSkin>::Action;
};

// Records the font and flags an edit field measures its text with, and
// strips flag 1 in Edit_Flags() the way Red Alert's skin strips TPF_CENTER.
struct FontRecordingSkin : CheckBoxTestSkin {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  static inline Font last_font = -1;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  static inline PrintFlags last_flags = -1;
  static PrintFlags Edit_Flags(PrintFlags flags) { return flags / 2 * 2; }
  static FontStyle FontStyleFor(Font font, PrintFlags flags) {
    last_font = font;
    last_flags = flags;
    return {};
  }
};

class CheckBoxTest : public testing::Test {
 protected:
  Input input_{48, 48};
  base::Installed<Input>::Scope input_scope_{input_};
  TestCheckBox box_;
};

TEST_F(CheckBoxTest, ClickTurnsTheBoxOnAndReportsIt) {
  engine::window::KeyNumber key = engine::window::KN_NONE;
  box_.Action(GadgetClass::kLeftPress, key, 12, 12);
  key = engine::window::KN_NONE;
  box_.Action(GadgetClass::kLeftRelease, key, 12, 12);
  EXPECT_TRUE(box_.IsOn);
  EXPECT_EQ(key, engine::window::ButtonKey(static_cast<int>(kId)));
}

// A release that did not end a press on the box (the press began elsewhere)
// returns no ID, so the box must not change: the dialog would never learn of
// it, and would apply a setting the box no longer shows.
TEST_F(CheckBoxTest, ReleaseWithoutAPressLeavesTheBoxAlone) {
  engine::window::KeyNumber key = engine::window::KN_NONE;
  box_.Action(GadgetClass::kLeftRelease, key, 12, 12);
  EXPECT_FALSE(box_.IsOn);
  EXPECT_NE(key, engine::window::ButtonKey(static_cast<int>(kId)));
}

TEST(TextWidgetFontTest, EditFieldKeepsItsFontThroughEditFlags) {
  char text[8] = "abc";
  const EditClass<FontRecordingSkin> edit(1, text, sizeof(text), /*font=*/7,
                                          /*flags=*/3, 0, 0);
  EXPECT_EQ(FontRecordingSkin::last_font, 7);
  EXPECT_EQ(FontRecordingSkin::last_flags, 2);
}

}  // namespace
}  // namespace game
