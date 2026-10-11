#ifndef CNC_RED_ALERT_TD_UI_DIALOG_H_
#define CNC_RED_ALERT_TD_UI_DIALOG_H_

#include <span>
#include <string>

#include "absl/base/attributes.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "game/core/point.h"
#include "game/ui/standard_colors.h"
#include "td/core/defines.h"
#include "td/core/jshell.h"

namespace td {

// Word wraps `string` in place so that no line exceeds `max_line_len` pixels
// in `font`, writing '\r' at each break. `width` and `height` receive the
// size of the wrapped text. Returns the number of lines.
int Format_Window_String(const FontStyle& font, std::span<char> string,
                         int max_line_len, int& width, int& height);
extern void Dialog_Box(PixelView& view, int x, int y, int w, int h);
void Draw_Box(PixelView& view, int x, int y, int w, int h, BoxStyleEnum up,
              bool filled);
void Window_Box(PixelView& view, WindowNumberType window, BoxStyleEnum style);
// What a text print draws with: its adjusted flags, colour and font.
struct TextStyle {
  TextPrintType flag{};  // The style flags after the font's own fixups.
  int forecolor = 0;     // Palette index the glyphs print in.
  FontStyle font_style;  // The font, spacing and glyph palette.
};

// Returns the font and spacing `font` and the shadow in `flags` select, with
// the identity palette: all that measuring text needs.
FontStyle TextFontStyle(FontType font, TextPrintType flags);

// Returns the style a print in `font` with `flags` in the colours `fore` and
// `back` uses: TextFontStyle() plus the glyph palette.
TextStyle TextStyleFor(FontType font, TextPrintType flags,
                       int fore = game::kTransparentBlack,
                       int back = game::kTransparentBlack);

// The colours, font and flags a text prints with.
struct PrintStyle {
  int fore = game::kTransparentBlack;  // Palette index of the glyphs.
  int back = game::kTransparentBlack;  // Palette index behind the glyphs.
  FontType font = FontType::k8Point;
  TextPrintType flags = TPF_DROPSHADOW;
};

// Prints text into a view in one style. A dialog makes one for each look it
// draws and reuses it; With() gives the same printer with more flags set.
//
// Example:
//   const TextPrinter label(view, {kCcGreen, game::kTransparentBlack,
//                                  kTextFont, kTextFlags});
//   label.Print(TXT_NAME_COLON, {x, y});
//   label.With(TPF_RIGHT).Print(absl::StrFormat("%d", count), {x + w, y});
class TextPrinter {
 public:
  TextPrinter(PixelView& view ABSL_ATTRIBUTE_LIFETIME_BOUND,
              const PrintStyle& style);

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
  PixelView* view_;
  PrintStyle style_;
};

}  // namespace td

#endif  // CNC_RED_ALERT_TD_UI_DIALOG_H_
