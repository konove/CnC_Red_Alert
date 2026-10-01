// File: UiSkin, how Tiberian Dawn draws the game::*<Skin> widgets.

#ifndef CNC_RED_ALERT_TD_UI_UI_SKIN_H_
#define CNC_RED_ALERT_TD_UI_UI_SKIN_H_

#include <cstddef>
#include <cstdint>
#include <span>

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
