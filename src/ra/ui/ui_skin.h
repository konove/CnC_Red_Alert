// File: UiSkin, how Red Alert draws the game::*<Skin> widgets.

#ifndef CNC_RED_ALERT_RA_UI_UI_SKIN_H_
#define CNC_RED_ALERT_RA_UI_UI_SKIN_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "engine/gfx/pixel_buffer.h"
#include "ra/core/defines.h"

namespace ra {

// The drawing the shared widgets leave to the game, as static functions the
// widget templates call.
struct UiSkin {
  // What a text widget's colour is: a remap scheme.
  using TextColor = RemapControlType*;
  using PrintFlags = TextPrintType;

  // The fields the message list keeps on each TextLabelClass: when the
  // message times out (0 = never) and its ID.
  struct LabelData {
    int64_t UserData1{0};
    int32_t UserData2{0};
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

  // Prints a text label, unclipped or clipped to `width` pixels.
  static void Print_Label(PixelView& view, const char* text, int x, int y,
                          TextColor color, PrintFlags style);
  static void Print_Clipped(PixelView& view, const char* text, int x, int y,
                            TextColor color, PrintFlags style, int width);

  // Draws a check box's frame and fill, ticked when `on`.
  static void Draw_Check_Box(PixelView& view, int x, int y, int width,
                             int height, bool on);
};

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_UI_UI_SKIN_H_
