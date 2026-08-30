// File: Reading Windows device-independent bitmaps out of memory.
//
// The Westwood Online lobby downloads .bmp files -- the icons beside each game
// type and the latency indicators -- and hands the bytes to Windows' DIB
// sample code (dibapi.h, dibfile.cc, dibutil.cc: about 2,000 lines of 1991
// Microsoft example that needs windows.h, GDI and the global heap). Only a
// handful of those entry points is ever called, and all of them are arithmetic
// over a block of memory.
//
// This is that handful, owning its pixels instead of returning a handle that
// has to be locked, unlocked and freed by hand.
//
// Example:
//   auto icon = dib::Image::FromBmp(file_bytes);
//   if (icon.has_value()) {
//     Draw(icon->Bits(), icon->Width(), icon->Height());
//   }

#ifndef CNC_RED_ALERT_RA_DIB_H_
#define CNC_RED_ALERT_RA_DIB_H_

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "base/types.h"

namespace dib {

// One colour-table entry, in the order a BMP stores it.
struct Color {
  std::uint8_t blue;
  std::uint8_t green;
  std::uint8_t red;
  std::uint8_t reserved;

  friend bool operator==(const Color&, const Color&) = default;
};

// A palettised bitmap: a colour table plus one byte of index per pixel.
//
// Rows are stored bottom-up and padded to a multiple of four bytes, the way a
// BMP stores them; Stride() gives the padded row length. Only 8-bit
// uncompressed bitmaps are accepted, which is all the game's own art and all
// the artwork the lobby ever served.
class Image {
 public:
  // Parses a whole .bmp file. Returns nullopt if the bytes are truncated, are
  // not a BMP, or are not an 8-bit uncompressed one.
  static std::optional<Image> FromBmp(std::span<const std::uint8_t> bmp);

  int Width() const { return width_; }
  int Height() const { return height_; }

  // Bytes per row including the padding to a four-byte boundary.
  base::ssize Stride() const {
    return (static_cast<base::ssize>(width_) + 3) & ~base::ssize{3};
  }

  // The pixels, bottom row first, Stride() bytes per row.
  std::span<const std::uint8_t> Bits() const { return bits_; }

  // The colour table. Mutable because the lobby remaps downloaded artwork onto
  // the game's own palette in place.
  std::span<const Color> Colors() const { return colors_; }
  std::span<Color> MutableColors() { return colors_; }

 private:
  Image() = default;

  int width_ = 0;
  int height_ = 0;
  std::vector<Color> colors_;
  std::vector<std::uint8_t> bits_;
};

}  // namespace dib

#endif  // CNC_RED_ALERT_RA_DIB_H_
