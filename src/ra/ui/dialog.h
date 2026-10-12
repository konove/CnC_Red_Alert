#ifndef CNC_RED_ALERT_RA_UI_DIALOG_H_
#define CNC_RED_ALERT_RA_UI_DIALOG_H_

#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include "absl/base/attributes.h"
#include "engine/gfx/font.h"
#include "engine/gfx/palette_index.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/rect.h"
#include "game/core/point.h"
#include "game/ui/standard_colors.h"
#include "ra/core/defines.h"

namespace ra {

void Draw_Caption(PixelView& view, int text, int x, int y, int w);
void Draw_Caption(PixelView& view, const char* text, int x, int y, int w);
// Word wraps "string" in place so that no line exceeds "max_line_len" pixels
// when rendered in `font`.
//
// Line breaks are written directly into the buffer: the space (or the '@'
// marker, which callers use to request an explicit break) at each break point
// is overwritten with '\r'. The string therefore keeps its original length and
// must be writable; never pass a string literal. If a single word is wider
// than "max_line_len" the line is broken mid word, which costs one character.
//
// "width" receives the pixel width of the widest resulting line and "height"
// the total pixel height of all lines. Returns the number of lines, or 0 if
// "string" is nullptr.
int Format_Window_String(const FontStyle& font, std::span<char> string,
                         int max_line_len, int& width, int& height);
extern void Dialog_Box(int x, int y, int w, int h);
// Draws a bordered box into `view`.
//
// "x,y" is the upper left corner and "w,h" the size, both in pixels. "up"
// selects the border style, which also picks the color set used for the fill,
// edges, and corners. When "filled" is true the interior is filled first.
//
// This is a low level routine: it draws with raw palette indices and does no
// color adjustment for the current graphic mode.
void Draw_Box(PixelView& view, int x, int y, int w, int h, BoxStyleEnum up,
              bool filled);
// Draws the swatch a setup dialog shows for one player colour: `box` with a
// one-pixel bevel, sunken when `selected` and raised otherwise, and everything
// inside the bevel in the palette index `color`.
void DrawColorBox(PixelView& view, const Rect& box,
                  engine::gfx::PaletteIndex color, bool selected);
void Window_Box(PixelView& view, WindowNumberType window, BoxStyleEnum style);
// What a text print draws with: its adjusted flags, colour and font.
struct TextStyle {
  TextPrintType flag{};  // The style flags after the font's own fixups.
  engine::gfx::PaletteIndex forecolor;  // What the glyphs print in.
  FontStyle font_style;  // The font, spacing and glyph palette.
};

// Returns the font and spacing `font` and the shadow in `flags` select, with
// the identity palette: all that measuring text needs.
FontStyle TextFontStyle(FontType font, TextPrintType flags);

// Returns the style a print in `font` with `flags` in the colours `fore`
// (PCOLOR_RED if nullptr) and `back` uses: TextFontStyle() plus the glyph
// palette.
TextStyle TextStyleFor(
    FontType font, TextPrintType flags, const RemapControlType* fore = nullptr,
    engine::gfx::PaletteIndex back = game::kTransparentBlack);

// The colours, font and flags a text prints with.
struct PrintStyle {
  const RemapControlType* scheme = nullptr;  // nullptr is the red scheme.
  engine::gfx::PaletteIndex back =
      game::kTransparentBlack;  // Behind the glyphs.
  FontType font = FontType::k8Point;
  TextPrintType flags = TPF_DROPSHADOW;
};

// Prints text into a view in one style. A dialog makes one for each look it
// draws and reuses it; With() gives the same printer with more flags set.
//
// Example:
//   const TextPrinter label(view, {scheme, game::kTransparentBlack,
//                                  kTextFont, kTextFlags});
//   label.Print(TXT_NAME_COLON, {x, y});
//   label.With(TPF_RIGHT).Print(absl::StrFormat("%d", count), {x + w, y});
class TextPrinter {
 public:
  TextPrinter(PixelView& view ABSL_ATTRIBUTE_LIFETIME_BOUND,
              const PrintStyle& style);
  // Returns a printer that paints in the single palette colour `fore` in place
  // of a colour scheme. Do not give it TPF_USE_GRAD_PAL: a flat colour has no
  // gradient.
  static TextPrinter Plain(PixelView& view ABSL_ATTRIBUTE_LIFETIME_BOUND,
                           engine::gfx::PaletteIndex fore,
                           engine::gfx::PaletteIndex back, FontType font,
                           TextPrintType flags);

  // Returns this printer with `flags` set on top of its own.
  [[nodiscard]] TextPrinter With(TextPrintType flags) const;

  // Prints `text` at `at` as it is; a nullptr text prints nothing. TPF_CENTER
  // and TPF_RIGHT make `at.x` the middle or the right edge of the text. Format
  // beforehand: absl::StrFormat() for a literal, base::FormatRuntime() for a
  // text from the string table.
  void Print(const char* text, game::Point at) const;
  void Print(const std::string& text, game::Point at) const;
  // Same, with the text looked up by string-table number; TXT_NONE prints
  // nothing.
  void Print(int text, game::Point at) const;
  // Prints `text` cut off `width` pixels right of `at`: a list box's row. A
  // tab in the text moves on to the next of `tabs`, which are pixel offsets
  // from `at.x`, or to the next multiple of 50 when there are none.
  void PrintClipped(const char* text, game::Point at, int width,
                    std::span<const int> tabs = {}) const;

 private:
  [[nodiscard]] const RemapControlType* scheme() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  PixelView* view_;
  PrintStyle style_;
  std::optional<RemapControlType> plain_scheme_;  // Set by Plain() only.
};

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_UI_DIALOG_H_
