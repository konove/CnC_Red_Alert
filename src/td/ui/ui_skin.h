// File: UiSkin, how Tiberian Dawn draws the game::*<Skin> widgets.

#ifndef CNC_RED_ALERT_TD_UI_UI_SKIN_H_
#define CNC_RED_ALERT_TD_UI_UI_SKIN_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "td/core/defines.h"

namespace td {

// The drawing the shared widgets leave to the game, as static functions the
// widget templates call.
struct UiSkin {
  // What a text widget's colour is: a palette index.
  using TextColor = int;
  using PrintFlags = TextPrintType;

  // The fields the message list keeps on each TextLabelClass: when the
  // message times out (0 = never), and the segments received so far and the
  // CRC of a multi-packet message.
  struct LabelData {
    int64_t UserData{0};
    uint8_t Segments{0};
    uint16_t CRC = 0;
  };

  // Draws frame `frame` of `shapes` at (x, y) in the main window.
  static void Draw_Shape(PixelView& view, std::span<const std::byte> shapes,
                         int frame, int x, int y);

  // The string-table number for "no text", and the flags a default-built
  // text button prints with.
  static constexpr int kTextNone = 0;  // TXT_NONE
  static constexpr PrintFlags kButtonDefaultFlags = TPF_8POINT;

  // Looks up text number `id`.
  static const char* Text_String(int id);

  // The width of `text` and the line height in the font `flags` select.
  static int Text_Width(PrintFlags flags, const char* text);
  static int Text_Height(PrintFlags flags);

  // Draws a text button's box and its caption centered at x, in its state.
  static void Draw_Button_Box(PixelView& view, int x, int y, int width,
                              int height, PrintFlags flags, bool disabled,
                              bool pressed);
  static void Print_Button_Text(PixelView& view, const char* text, int x, int y,
                                PrintFlags flags, bool disabled, bool pressed,
                                bool on);

  // A gauge's thumb width for hit tests, in pixels (the thumb drawn is 4).
  static constexpr int kGaugeThumbPixels = 8;

  // `amount` scaled by numerator/denominator in the game's gauge
  // fixed-point arithmetic.
  static int Scale(int amount, int numerator, int denominator);

  // Draws a gauge's body (sunken) or thumb (raised).
  static void Draw_Gauge_Box(PixelView& view, int x, int y, int width,
                             int height, bool raised);

  // The colour a gauge fills up to its value with.
  static unsigned char Gauge_Fill_Color();

  // The font `flags` select.
  static FontStyle Font(PrintFlags flags);

  // An edit field's height above its font's tallest glyph.
  static constexpr int kEditHeightPad = 2;

  // The flags and colour an edit field draws its text with.
  static PrintFlags Edit_Flags(PrintFlags flags);
  static TextColor Edit_Default_Color();

  // Draws an edit field's box, and its text with the cursor after it while
  // it has the focus and room for another character.
  static void Draw_Edit_Box(PixelView& view, int x, int y, int width,
                            int height);
  static void Draw_Edit_Text(PixelView& view, const char* text, int x, int y,
                             int width, TextColor color, PrintFlags flags,
                             bool focused, int max_length);

  // Prints a text label, unclipped or clipped to `width` pixels.
  static void Print_Label(PixelView& view, const char* text, int x, int y,
                          TextColor color, PrintFlags style);
  static void Print_Clipped(PixelView& view, const char* text, int x, int y,
                            TextColor color, PrintFlags style, int width);

  // Draws a check box's frame and fill, ticked when `on`.
  static void Draw_Check_Box(PixelView& view, int x, int y, int width,
                             int height, bool on);
};

}  // namespace td

#endif  // CNC_RED_ALERT_TD_UI_UI_SKIN_H_
