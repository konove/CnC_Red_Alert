// Draws the tiles of an icon set, the format the map's terrain templates are
// stored in: a header, the tiles' pixels one after another, a byte per tile
// saying whether color 0 in it is transparent, and a map from a template's
// cells to its tiles.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/tile.h"

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

// What drawing a tile needs from an icon set, the tables as views into the
// set's data.
struct IconSetTables {
  int tile_width = 0;
  int tile_height = 0;
  int tile_count = 0;
  // The tiles' pixels, tile_width * tile_height bytes each, from the start of
  // tile 0 to the end of the set.
  std::span<const std::byte> tiles;
  // One byte per tile: nonzero means color 0 in that tile is transparent.
  std::span<const std::byte> transparent;
  // One byte per template cell, giving the tile drawn there. Empty if the
  // header's map offset lies outside the set, in which case the cell number is
  // the tile number.
  std::span<const std::byte> cell_tiles;
};

// Reads the header of `icon_set`. Returns nullopt if the data is too short to
// hold one.
//
// The header is read on every draw rather than remembered by the set's
// address: sets live in the theater's MIX archive, which a theater change
// frees and replaces, so a different set can turn up at an address already
// seen.
static std::optional<IconSetTables> ReadIconSet(
    std::span<const std::byte> icon_set) {
  if (icon_set.size() < sizeof(IControl_Type)) {
    return std::nullopt;
  }

  IControl_Type header{};
  base::CopyBytes(base::ObjectBytes(header), icon_set, sizeof(header));

  // Tell Tiberian Dawn's header from Red Alert's. In Tiberian Dawn's the
  // bytes of MapWidth and MapHeight are the low and high halves of Size, so a
  // set under 64K reads as MapHeight 0. The width test catches a larger one
  // whose low half cannot be a map width; one whose low half is 256 or less
  // is taken for Red Alert's.
  int32_t tiles_offset = header.Icons;
  int32_t transparent_offset = header.TransFlag;
  int32_t cell_tiles_offset = header.Map;
  if (!header.MapHeight || header.MapWidth > 256) {
    TiberianDawnIconSetHeader old_header{};
    base::CopyBytes(base::ObjectBytes(old_header), icon_set,
                    sizeof(old_header));
    tiles_offset = old_header.icons;
    transparent_offset = old_header.trans_flag;
    cell_tiles_offset = old_header.map;
  }
  return IconSetTables{.tile_width = header.Width,
                       .tile_height = header.Height,
                       .tile_count = header.Count,
                       .tiles = TableAt(icon_set, tiles_offset),
                       .transparent = TableAt(icon_set, transparent_offset),
                       .cell_tiles = TableAt(icon_set, cell_tiles_offset)};
}

void PixelView::DrawStampLocked(std::span<const std::byte> icon_set, int cell,
                                int x, int y,
                                std::span<const uint8_t> remap_table,
                                int clip_x, int clip_y, int clip_width,
                                int clip_height) {
  const std::optional<IconSetTables> tables = ReadIconSet(icon_set);
  if (!tables) {
    return;
  }
  const int tile_width = tables->tile_width;
  const int tile_height = tables->tile_height;

  // Translate the template cell into the tile drawn there. A cell with no
  // tile maps to 255, which the tile_count check below turns away.
  int tile = cell;
  if (!tables->cell_tiles.empty()) {
    if (cell < 0 || base::ToSize(cell) >= tables->cell_tiles.size()) {
      return;
    }
    tile = std::to_integer<uint8_t>(
        base::At(tables->cell_tiles, base::ToSize(cell)));
  }

  // Every table is checked against the set's data, so a damaged set draws
  // nothing rather than reading past it. A tile is one byte per pixel.
  if (tile < 0 || tile >= tables->tile_count || tile_width <= 0 ||
      tile_height <= 0 || base::ToSize(tile) >= tables->transparent.size() ||
      base::ToSize(tile + 1) >
          tables->tiles.size() / base::ToSize(tile_width * tile_height)) {
    return;
  }

  // The part of the tile left to draw once it is clipped.
  int draw_width = tile_width;
  int draw_height = tile_height;

  auto src = tables->tiles.begin() +
             (static_cast<base::ssize>(tile) * tile_width * tile_height);

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
  if (x + tile_width < clip_x || y + tile_height < clip_y) {
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
  int src_skip = tile_width - draw_width;

  if (x + draw_width > clip_right) {
    const int unclipped_width = draw_width;
    draw_width = clip_right - x;
    src_skip += unclipped_width - draw_width;
  }

  if (y < clip_y) {
    draw_height -= clip_y - y;
    src += static_cast<base::ssize>(tile_width) * (clip_y - y);
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
    // Remapped draw. Color 0 is tested after the remap, so it is transparent
    // whatever the set's flag says, and a table that maps a color to 0 makes
    // that color transparent too.
    do {
      for (int column = 0; column < draw_width; column++) {
        const uint8_t pixel =
            base::At(remap_table, std::to_integer<uint8_t>(*src++));
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
  else if (base::At(tables->transparent, base::ToSize(tile)) != std::byte{}) {
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
      src += tile_width;
    } while (--draw_height);
  }
}
