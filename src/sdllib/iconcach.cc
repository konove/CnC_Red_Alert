// Draws the tiles of an icon set, the format the map's terrain templates are
// stored in: a header, the tiles' pixels one after another, a byte per tile
// saying whether color 0 in it is transparent, and a map from a template's
// cells to its tiles.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/tile.h"

// The tiles' pixels, IconSize bytes each, from the start of tile 0 to the end
// of the set's data.
static std::span<const std::byte> StampPtr;

// One byte per tile: nonzero means color 0 in that tile is transparent.
static std::span<const std::byte> IsTrans;

// One byte per template cell, giving the tile drawn there. Empty if the
// header's map offset lies outside the set, in which case the icon number is
// the tile number.
static std::span<const std::byte> MapPtr;
static int IconWidth = 0;
static int IconHeight = 0;
static int IconSize = 0;
static int IconCount = 0;

// The icon set header of Tiberian Dawn, which lacks Red Alert's MapWidth,
// MapHeight and ColorMap fields. sdllib is compiled without TD, so
// IControl_Type is always Red Alert's layout and Tiberian Dawn's sets are read
// through this one.
struct TiberianDawnIconSetHeader {
  int16_t width;       // Width of icons (pixels).
  int16_t height;      // Height of icons (pixels).
  int16_t count;       // Number of (logical) icons in this set.
  int16_t allocated;   // Was this iconset allocated?
  int32_t size;        // Size of entire iconset memory block.
  int32_t icons;       // Offset from buffer start to icon data.
  int32_t palettes;    // Offset from buffer start to palette data.
  int32_t remaps;      // Offset from buffer start to remap index data.
  int32_t trans_flag;  // Offset for transparency flag table.
  int32_t map;         // Icon map offset (if present).
};

// Returns the part of `icon_set` from `offset` to its end, or an empty span if
// the header's offset lies outside the set.
static std::span<const std::byte> TableAt(std::span<const std::byte> icon_set,
                                          int offset) {
  return offset >= 0 && base::ToSize(offset) <= icon_set.size()
             ? icon_set.subspan(base::ToSize(offset))
             : std::span<const std::byte>{};
}

// Reads the header of `icon_ptr` into the globals above. Returns false, and
// leaves them alone, if the data is too short to hold a header.
//
// The header is read on every draw rather than remembered by the set's
// address: sets live in the theater's MIX archive, which a theater change
// frees and replaces, so a different set can turn up at an address already
// seen.
static bool Init_Stamps(std::span<const std::byte> icon_ptr) {
  if (icon_ptr.size() < sizeof(IControl_Type)) {
    return false;
  }

  IControl_Type header{};
  base::CopyBytes(base::ObjectBytes(header), icon_ptr, sizeof(header));
  const auto* control = &header;
  IconCount = control->Count;
  IconWidth = control->Width;
  IconHeight = control->Height;
  // A tile is one byte per pixel.
  IconSize = IconWidth * IconHeight;

  // Tell Tiberian Dawn's header from Red Alert's. In Tiberian Dawn's the
  // bytes of MapWidth and MapHeight are the low and high halves of Size, so a
  // set under 64K reads as MapHeight 0. The width test catches a larger one
  // whose low half cannot be a map width; one whose low half is 256 or less
  // is taken for Red Alert's.
  if (!control->MapHeight || control->MapWidth > 256) {
    TiberianDawnIconSetHeader old_header{};
    base::CopyBytes(base::ObjectBytes(old_header), icon_ptr,
                    sizeof(old_header));
    const auto* old = &old_header;
    MapPtr = TableAt(icon_ptr, old->map);
    StampPtr = TableAt(icon_ptr, old->icons);
    IsTrans = TableAt(icon_ptr, old->trans_flag);
  } else {
    MapPtr = TableAt(icon_ptr, control->Map);
    StampPtr = TableAt(icon_ptr, control->Icons);
    IsTrans = TableAt(icon_ptr, control->TransFlag);
  }
  return true;
}

void PixelView::DrawStampLocked(std::span<const std::byte> icon_set, int cell,
                                int x, int y,
                                std::span<const uint8_t> remap_table,
                                int clip_x, int clip_y, int clip_width,
                                int clip_height) {
  if (!Init_Stamps(icon_set)) {
    return;
  }

  // Translate the template cell into the tile drawn there. A cell with no
  // tile maps to 255, which the IconCount check below turns away.
  int tile = cell;
  if (!MapPtr.empty()) {
    if (cell < 0 || base::ToSize(cell) >= MapPtr.size()) {
      return;
    }
    tile = std::to_integer<uint8_t>(base::At(MapPtr, base::ToSize(cell)));
  }

  // Every table is checked against the set's data, so a damaged set draws
  // nothing rather than reading past it.
  if (tile < 0 || tile >= IconCount || IconWidth <= 0 || IconHeight <= 0 ||
      base::ToSize(tile) >= IsTrans.size() ||
      base::ToSize(tile + 1) > StampPtr.size() / base::ToSize(IconSize)) {
    return;
  }

  // The part of the tile left to draw once it is clipped.
  int draw_width = IconWidth;
  int draw_height = IconHeight;

  auto src = StampPtr.begin() + (static_cast<base::ssize>(tile) * IconSize);

  // x,y arrive relative to the clip rectangle's corner; make them view
  // coordinates, like the rectangle's exclusive right and bottom edges.
  const int clip_right = clip_x + clip_width;
  x += clip_x;
  const int clip_bottom = clip_y + clip_height;
  y += clip_y;

  // Nothing to draw if the tile starts past the rectangle's right or bottom
  // edge...
  if (x >= clip_right || y >= clip_bottom) {
    return;
  }

  // ...or ends before its left or top edge. A tile ending exactly on the
  // edge gets through here and is caught by the empty-size check below.
  if (x + IconWidth < clip_x || y + IconHeight < clip_y) {
    return;
  }

  // Clip the tile to the rectangle, moving src to the first pixel still
  // drawn.
  if (x < clip_x) {
    src += clip_x - x;
    draw_width -= clip_x - x;
    x = clip_x;
  }

  // Source pixels to step over at the end of each row: the columns clipped
  // on the left, and below those clipped on the right.
  int src_skip = IconWidth - draw_width;

  if (x + draw_width > clip_right) {
    const int unclipped_width = draw_width;
    draw_width = clip_right - x;
    src_skip += unclipped_width - draw_width;
  }

  if (y < clip_y) {
    draw_height -= clip_y - y;
    src += static_cast<base::ssize>(IconWidth) * (clip_y - y);
    y = clip_y;
  }

  if (y + draw_height > clip_bottom) {
    draw_height = clip_bottom - y;
  }

  if (!draw_width || !draw_height) {
    return;
  }

  // Without a remap table the faster loops below are used. A table too short
  // to translate every color is refused rather than read past.
  const bool remapping = !remap_table.empty();
  if (remapping && remap_table.size() < 256) {
    return;
  }

  const base::ssize dst_stride = stride();
  auto dst = pixels().begin() + x + (y * dst_stride);

  // Destination bytes from the end of a drawn row to the start of the next.
  const base::ssize dst_skip = dst_stride - draw_width;

  if (remapping) {
    const auto remap8 = remap_table;
    // Remapped draw. Color 0 is tested after the remap, so it is transparent
    // whatever the set's flag says, and a table that maps a color to 0 makes
    // that color transparent too.
    do {
      for (int column = 0; column < draw_width; column++) {
        const uint8_t pixel =
            base::At(remap8, std::to_integer<uint8_t>(*src++));
        if (pixel) {
          *dst = pixel;
        }
        dst++;
      }

      src += src_skip;
      dst += dst_skip;
    } while (--draw_height);
  }
  // Unremapped: the set's per-tile flag picks the transparent or the opaque
  // loop.
  else if (base::At(IsTrans, base::ToSize(tile)) != std::byte{}) {
    // Transparent draw: color 0 leaves the destination alone.
    do {
      for (int column = 0; column < draw_width; column++) {
        const auto pixel = std::to_integer<uint8_t>(*src++);
        if (pixel) {
          *dst = pixel;
        }
        dst++;
      }

      src += src_skip;
      dst += dst_skip;
    } while (--draw_height);
  } else {
    // Opaque draw: whole rows are copied.
    do {
      std::transform(src, src + draw_width, dst, [](std::byte value) {
        return std::to_integer<uint8_t>(value);
      });
      dst += dst_stride;
      src += IconWidth;
    } while (--draw_height);
  }
}
