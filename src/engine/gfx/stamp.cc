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
#include "base/numeric.h"
#include "base/types.h"
#include "base/unaligned.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/tile.h"

namespace {

// The icon set header of Tiberian Dawn, which lacks Red Alert's MapWidth,
// MapHeight and ColorMap fields. engine_gfx is compiled without TD, so
// IControl_Type is always Red Alert's layout and Tiberian Dawn's sets are read
// through this one. Only the three table offsets are read; the rest hold
// their place in the layout.
struct TiberianDawnIconSetHeader {
  [[maybe_unused]] int16_t width;      // Width of icons (pixels).
  [[maybe_unused]] int16_t height;     // Height of icons (pixels).
  [[maybe_unused]] int16_t count;      // Number of (logical) icons in this set.
  [[maybe_unused]] int16_t allocated;  // Was this IconSet allocated?
  [[maybe_unused]] int32_t size;       // Size of the whole IconSet.
  int32_t icons;                       // Offset of the icon data.
  [[maybe_unused]] int32_t palettes;   // Offset of the palette data.
  [[maybe_unused]] int32_t remaps;     // Offset of the remap index data.
  int32_t trans_flag;                  // Offset of the transparency flags.
  int32_t map;                         // Offset of the icon map, if present.
};

// What drawing a tile needs from an icon set, the tables as views into the
// set's data.
struct IconSetTables {
  // Both positive.
  int tile_width = 0;
  int tile_height = 0;
  // The tiles the set can actually draw: the header's count, cut down to what
  // the pixel and transparency tables hold. Negative if the header's is.
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

}  // namespace

// Returns the part of `icon_set` from `offset` to its end, or an empty span if
// the header's offset lies outside the set.
static std::span<const std::byte> TableAt(
    const std::span<const std::byte> icon_set, const int offset) {
  return offset >= 0 && base::ToSize(offset) <= icon_set.size()
             ? icon_set.subspan(base::ToSize(offset))
             : std::span<const std::byte>{};
}

// Reads the header of `icon_set`. Returns nullopt if the data is too short to
// hold one, or the tile size is not positive.
//
// The header is read on every draw rather than remembered by the set's
// address: sets live in the theater's MIX archive, which a theater change
// frees and replaces, so a different set can turn up at an address already
// seen.
static std::optional<IconSetTables> ReadIconSet(
    const std::span<const std::byte> icon_set) {
  if (icon_set.size() < sizeof(IControl_Type)) {
    return std::nullopt;
  }
  const auto header = base::ReadUnaligned<IControl_Type>(icon_set);
  if (header.Width <= 0 || header.Height <= 0) {
    return std::nullopt;
  }

  // Tell Tiberian Dawn's header from Red Alert's. In Tiberian Dawn's the
  // bytes of MapWidth and MapHeight are the low and high halves of Size, so a
  // set under 64K reads as MapHeight 0. The width test catches a larger one
  // whose low half cannot be a map width; one whose low half is 256 or less
  // is taken for Red Alert's.
  int32_t tiles_offset = header.Icons;
  int32_t transparent_offset = header.TransFlag;
  int32_t cell_tiles_offset = header.Map;
  if (!header.MapHeight || header.MapWidth > 256) {
    const auto old_header =
        base::ReadUnaligned<TiberianDawnIconSetHeader>(icon_set);
    tiles_offset = old_header.icons;
    transparent_offset = old_header.trans_flag;
    cell_tiles_offset = old_header.map;
  }

  const std::span<const std::byte> tiles = TableAt(icon_set, tiles_offset);
  const std::span<const std::byte> transparent =
      TableAt(icon_set, transparent_offset);
  // A tile is one byte per pixel.
  const base::ssize tile_bytes = base::ssize{header.Width} * header.Height;
  const base::ssize tiles_held =
      std::min(std::ssize(transparent), std::ssize(tiles) / tile_bytes);
  return IconSetTables{.tile_width = header.Width,
                       .tile_height = header.Height,
                       .tile_count = static_cast<int>(
                           std::min<base::ssize>(header.Count, tiles_held)),
                       .tiles = tiles,
                       .transparent = transparent,
                       .cell_tiles = TableAt(icon_set, cell_tiles_offset)};
}

void PixelView::DrawStampLocked(const std::span<const std::byte> icon_set,
                                const int cell, int x, int y,
                                std::span<const uint8_t> remap_table,
                                const int clip_x, const int clip_y,
                                const int clip_width, const int clip_height) {
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
  if (tile < 0 || tile >= tables->tile_count) {
    return;
  }

  // x,y arrive relative to the clip rectangle's corner. Intersect the tile
  // with the rectangle, in view coordinates.
  x += clip_x;
  y += clip_y;
  const int left = std::max(x, clip_x);
  const int top = std::max(y, clip_y);
  const int right = std::min(x + tile_width, clip_x + clip_width);
  const int bottom = std::min(y + tile_height, clip_y + clip_height);
  if (left >= right || top >= bottom) {
    return;
  }
  const int draw_width = right - left;
  const int draw_height = bottom - top;

  // Without a remap table the faster loops below are used. A table too short
  // to translate every color is refused rather than read past.
  const bool remapping = !remap_table.empty();
  if (remapping && remap_table.size() < 256) {
    return;
  }

  // The first pixel drawn, in the tile and in the view.
  const auto src_start =
      tables->tiles.begin() +
      (static_cast<base::ssize>(tile) * tile_width * tile_height) +
      (static_cast<base::ssize>(top - y) * tile_width) + (left - x);
  const base::ssize dst_stride = stride();
  const auto dst_start = pixels().begin() + left + (top * dst_stride);

  // Draws the clipped tile with each pixel passed through `translate`,
  // leaving the destination alone where that gives color 0.
  const auto draw_transparent = [&](auto translate) {
    for (int row = 0; row < draw_height; ++row) {
      auto src = src_start + (static_cast<base::ssize>(row) * tile_width);
      auto dst = dst_start + (row * dst_stride);
      for (int column = 0; column < draw_width; ++column) {
        if (const uint8_t pixel = translate(std::to_integer<uint8_t>(*src++))) {
          *dst = pixel;
        }
        ++dst;
      }
    }
  };

  if (remapping) {
    // Color 0 is tested after the remap, so it is transparent whatever the
    // set's flag says, and a table that maps a color to 0 makes that color
    // transparent too.
    draw_transparent([remap_table](const uint8_t pixel) {
      return base::At(remap_table, pixel);
    });
  } else if (base::At(tables->transparent, base::ToSize(tile)) != std::byte{}) {
    // The set's per-tile flag: color 0 leaves the destination alone.
    draw_transparent([](const uint8_t pixel) { return pixel; });
  } else {
    // Opaque: whole rows are copied.
    for (int row = 0; row < draw_height; ++row) {
      const auto src = src_start + (static_cast<base::ssize>(row) * tile_width);
      std::transform(src, src + draw_width, dst_start + (row * dst_stride),
                     [](const std::byte value) {
                       return std::to_integer<uint8_t>(value);
                     });
    }
  }
}
