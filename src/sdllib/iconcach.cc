// Draws the tiles of an icon set, the format the map's terrain templates are
// stored in: a header, the tiles' pixels one after another, a byte per tile
// saying whether color 0 in it is transparent, and a map from a template's
// cells to its tiles. What remains of the Windows 95 library's cache, which
// kept icon sets in video memory, is two stubs.

#include "sdllib/iconcach.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/strings/str_format.h"
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
struct IControl_Type_Old {
  int16_t Width;      // Width of icons (pixels).
  int16_t Height;     // Height of icons (pixels).
  int16_t Count;      // Number of (logical) icons in this set.
  int16_t Allocated;  // Was this iconset allocated?
  int32_t Size;       // Size of entire iconset memory block.
  int32_t Icons;      // Offset from buffer start to icon data.
  int32_t Palettes;   // Offset from buffer start to palette data.
  int32_t Remaps;     // Offset from buffer start to remap index data.
  int32_t TransFlag;  // Offset for transparency flag table.
  int32_t Map;        // Icon map offset (if present).
};

// Returns the part of `data` from `offset` to its end, or an empty span if the
// header's offset lies outside the data.
static std::span<const std::byte> Table(std::span<const std::byte> data,
                                        int offset) {
  return offset >= 0 && base::ToSize(offset) <= data.size()
             ? data.subspan(base::ToSize(offset))
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
    IControl_Type_Old old_header{};
    base::CopyBytes(base::ObjectBytes(old_header), icon_ptr,
                    sizeof(old_header));
    const auto* old = &old_header;
    MapPtr = Table(icon_ptr, old->Map);
    StampPtr = Table(icon_ptr, old->Icons);
    IsTrans = Table(icon_ptr, old->TransFlag);
  } else {
    MapPtr = Table(icon_ptr, control->Map);
    StampPtr = Table(icon_ptr, control->Icons);
    IsTrans = Table(icon_ptr, control->TransFlag);
  }
  return true;
}

void PixelView::DrawStampLocked(std::span<const std::byte> icon_data, int icon,
                                int x, int y,
                                std::span<const uint8_t> remap_table, int min_x,
                                int min_y, int max_x, int max_y) {
  if (!Init_Stamps(icon_data)) {
    return;
  }

  // Translate the template cell into the tile drawn there. A cell with no
  // tile maps to 255, which the IconCount check below turns away.
  if (!MapPtr.empty()) {
    if (icon < 0 || base::ToSize(icon) >= MapPtr.size()) {
      return;
    }
    icon = std::to_integer<uint8_t>(base::At(MapPtr, base::ToSize(icon)));
  }

  // Every table is checked against the set's data, so a damaged set draws
  // nothing rather than reading past it.
  if (icon < 0 || icon >= IconCount || IconWidth <= 0 || IconHeight <= 0 ||
      base::ToSize(icon) >= IsTrans.size() ||
      base::ToSize(icon + 1) > StampPtr.size() / base::ToSize(IconSize)) {
    return;
  }

  // The part of the tile left to draw once it is clipped.
  int iwidth = IconWidth;
  int iheight = IconHeight;

  auto ptr = StampPtr.begin() + (static_cast<base::ssize>(icon) * IconSize);

  // The clip rectangle arrives as corner and size, and x,y relative to its
  // corner; make everything view coordinates, with max_x,max_y the rectangle's
  // exclusive right and bottom edges.
  max_x += min_x;
  x += min_x;
  max_y += min_y;
  y += min_y;

  // Nothing to draw if the tile starts past the rectangle's right or bottom
  // edge...
  if (x >= max_x || y >= max_y) {
    return;
  }

  // ...or ends before its left or top edge. A tile ending exactly on the
  // edge gets through here and is caught by the empty-size check below.
  if (x + IconWidth < min_x || y + IconHeight < min_y) {
    return;
  }

  // Clip the tile to the rectangle, moving ptr to the first pixel still
  // drawn.
  if (x < min_x) {
    ptr += min_x - x;
    iwidth -= min_x - x;
    x = min_x;
  }

  // Source pixels to step over at the end of each row: the columns clipped
  // on the left, and below those clipped on the right.
  int skip = IconWidth - iwidth;

  if (x + iwidth > max_x) {
    const int ow = iwidth;
    iwidth = max_x - x;
    skip += ow - iwidth;
  }

  if (y < min_y) {
    iheight -= min_y - y;
    ptr += static_cast<base::ssize>(IconWidth) * (min_y - y);
    y = min_y;
  }

  if (y + iheight > max_y) {
    iheight = max_y - y;
  }

  if (!iwidth || !iheight) {
    return;
  }

  // Without a remap table the faster loops below are used. A table too short
  // to translate every color is refused rather than read past.
  const bool doremap = !remap_table.empty();
  if (doremap && remap_table.size() < 256) {
    return;
  }

  const base::ssize dst_area = stride();
  auto dst_offset = pixels().begin() + x + (y * dst_area);

  // Destination bytes from the end of a drawn row to the start of the next.
  const base::ssize modulo = dst_area - iwidth;

  if (doremap) {
    const auto remap8 = remap_table;
    // Remapped draw. Color 0 is tested after the remap, so it is transparent
    // whatever the set's flag says, and a table that maps a color to 0 makes
    // that color transparent too.
    do {
      for (int column = 0; column < iwidth; column++) {
        const uint8_t pixel =
            base::At(remap8, std::to_integer<uint8_t>(*ptr++));
        if (pixel) {
          *dst_offset = pixel;
        }
        dst_offset++;
      }

      ptr += skip;
      dst_offset += modulo;
    } while (--iheight);
  }
  // Unremapped: the set's per-tile flag picks the transparent or the opaque
  // loop.
  else if (base::At(IsTrans, base::ToSize(icon)) != std::byte{}) {
    // Transparent draw: color 0 leaves the destination alone.
    do {
      for (int column = 0; column < iwidth; column++) {
        const auto pixel = std::to_integer<uint8_t>(*ptr++);
        if (pixel) {
          *dst_offset = pixel;
        }
        dst_offset++;
      }

      ptr += skip;
      dst_offset += modulo;
    } while (--iheight);
  } else {
    // Opaque draw: whole rows are copied.
    do {
      std::transform(ptr, ptr + iwidth, dst_offset, [](std::byte value) {
        return std::to_integer<uint8_t>(value);
      });
      dst_offset += dst_area;
      ptr += IconWidth;
    } while (--iheight);
  }
}

void Restore_Cached_Icons() { absl::PrintF("%s\n", __func__); }

void Register_Icon_Set(const void* /*icon_data*/, bool /*pre_cache*/) {}
