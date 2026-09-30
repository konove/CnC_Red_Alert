// Re-implemented from the assembler blitter 2KEYFBUF.ASM, which git history
// holds (deleted with the other .asm sources).

#include "engine/gfx/shape_blitter.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <utility>

#include "base/array.h"
#include "base/buffer.h"
#include "base/clip.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"

namespace engine::gfx {
namespace {

// The effects one draw needs, one bit each, computed from the ShapeFlags on
// every call. Together they index kBlitters, as the assembler's FLAG_* bits
// indexed its table of line routines.
constexpr uint32_t kBlitTransparent = 1;
constexpr uint32_t kBlitGhost = 2;
constexpr uint32_t kBlitFading = 4;
constexpr uint32_t kBlitPredator = 8;
// One blitter per combination of the bits above.
constexpr std::size_t kBlitterCount = 16;
static_assert(kBlitterCount ==
              (kBlitTransparent | kBlitGhost | kBlitFading | kBlitPredator) +
                  1);

// How many pixels to the right of the one being drawn a predator pixel
// samples, walked in order and wrapped. Sampling the page instead of drawing
// the shape's color shows a displaced copy of the background, the shimmer.
constexpr std::array kPredatorRightOffsets{1, 3, 2, 5, 2, 3, 4, 1};
// ApplyEffects() lets its step counter wrap at 2^32, which keeps the walk in
// order only while the table's size divides that.
static_assert(std::has_single_bit(kPredatorRightOffsets.size()));

// The same walk for a negative predator offset: how many pixels to the left,
// one row down. The assembler reached these by indexing the right-hand table
// with the offset made negative, which landed 256 bytes below it on a copy it
// had filled in with the page's stride.
constexpr std::array kPredatorLeftOffsets{1, 3, 2, 5, 2, 4, 3, 1};

// A shadow is a ghost draw in which every color picks row 0, the shadow table.
constexpr std::array<uint8_t, 256> kShadowRowOfColor{};

// One clipped draw: line_count lines of pixel_count pixels, read from `src`
// and written to `dst`, whose rows are src_stride and dst_stride apart, with
// the tables its effects read.
struct BlitJob {
  std::span<const std::byte> src;
  std::span<uint8_t> dst;
  base::ssize src_stride = 0;
  base::ssize dst_stride = 0;
  int pixel_count = 0;
  int line_count = 0;
  // ShapeFlags::kGhost: ShapeEffects::ghost_table split into its two parts.
  std::span<const uint8_t> ghost_row_of_color;
  std::span<const uint8_t> ghost_rows;
  std::span<const uint8_t> fading_table;
  // ShapeFlags::kPredator: the offsets from the pixel being drawn to the page
  // pixel it samples, and the one the walk starts on.
  std::array<base::ssize, kPredatorRightOffsets.size()> predator_offsets{};
  uint32_t predator_start = 0;
};

// Returns the color the shape's `color` becomes at dst_index in job.dst with
// the effects in kEffects, advancing the predator walk. The effects apply in
// the assembler's order: the predator sample replaces the shape's color, the
// ghost table blends that with the page, and the fading table remaps the
// result.
template <uint32_t kEffects>
uint8_t ApplyEffects(const BlitJob& job, uint8_t color,
                     const base::ssize dst_index, uint32_t& predator_step) {
  if constexpr ((kEffects & kBlitPredator) != 0) {
    // Take the color from the page a table-driven distance away. Near the end
    // of the page there is nothing to sample and the shape's own color stays.
    // Reducing the step at the index rather than on each step lets the
    // compiler see the index is in range and drop the at() check.
    const base::ssize sample_index =
        dst_index +
        job.predator_offsets.at(predator_step++ % job.predator_offsets.size());
    if (sample_index < std::ssize(job.dst)) {
      color = base::At(job.dst, sample_index);
    }
  }

  if constexpr ((kEffects & kBlitGhost) != 0) {
    const uint8_t ghost_row = base::At(job.ghost_row_of_color, color);
    // 0xFF marks an opaque color; anything else is a remap row.
    if (ghost_row != 0xFF) {
      color = base::At(job.ghost_rows,
                       (ghost_row * 256) + base::At(job.dst, dst_index));
    }
  }

  if constexpr ((kEffects & kBlitFading) != 0) {
    color = base::At(job.fading_table, color);
  }
  return color;
}

// Draws `job` with the effects in kEffects. The assembler had a routine per
// combination; the template generates them. The job is taken by value: the
// uint8_t stores into dst may alias anything whose address is known, so a
// caller's job would have every field reloaded after each pixel.
template <uint32_t kEffects>
void BlitWithEffects(const BlitJob job) {
  if constexpr (kEffects == 0) {
    for (base::ssize line = 0; line < job.line_count; ++line) {
      base::CopyBytes(std::as_writable_bytes(
                          job.dst.subspan(base::ToSize(line * job.dst_stride))),
                      job.src.subspan(base::ToSize(line * job.src_stride)),
                      job.pixel_count);
    }
  } else {
    uint32_t predator_step = job.predator_start;
    for (base::ssize line = 0; line < job.line_count; ++line) {
      const base::ssize src_start = line * job.src_stride;
      const base::ssize dst_start = line * job.dst_stride;
      // The assembler unrolled this loop 32 times.
      for (base::ssize column = 0; column < job.pixel_count; ++column) {
        const auto color =
            std::to_integer<uint8_t>(base::At(job.src, src_start + column));
        if ((kEffects & kBlitTransparent) != 0 && color == 0) {
          continue;
        }
        const base::ssize dst_index = dst_start + column;
        base::At(job.dst, dst_index) =
            ApplyEffects<kEffects>(job, color, dst_index, predator_step);
      }
    }
  }
}

using Blitter = void (*)(BlitJob);

template <std::size_t... kIndex>
constexpr std::array<Blitter, sizeof...(kIndex)> MakeBlitters(
    std::index_sequence<kIndex...> /*indices*/) {
  return {&BlitWithEffects<static_cast<uint32_t>(kIndex)>...};
}

// BlitWithEffects for every combination of the kBlit* bits, indexed by them.
constexpr auto kBlitters =
    MakeBlitters(std::make_index_sequence<kBlitterCount>());

}  // namespace

void DrawShapeFrame(int x, int y, const int width, const int height,
                    const std::span<const std::byte> frame, PixelView& dest,
                    const ShapeFlags flags, const ShapeEffects& effects) {
  if (width <= 0 || height <= 0 ||
      base::ssize{width} * height > std::ssize(frame)) {
    return;
  }

  BlitJob job;
  // The kBlit* bits of the effects this draw needs.
  uint32_t effect_bits = 0;

  // ShapeFlags::kCentered: x,y name the frame's center rather than its top
  // left.
  if (base::Any(flags & ShapeFlags::kCentered)) {
    x -= width / 2;
    y -= height / 2;
  }

  if (base::Any(flags & ShapeFlags::kTransparent)) {
    effect_bits |= kBlitTransparent;
  }

  if (base::Any(flags & ShapeFlags::kShadow)) {
    if (effects.shadow_table.size() < 256) {
      return;
    }
    job.ghost_row_of_color = kShadowRowOfColor;
    job.ghost_rows = effects.shadow_table.first(256);
    effect_bits |= kBlitGhost;
  } else {
    if (base::Any(flags & ShapeFlags::kGhost)) {
      if (effects.ghost_table.size() < 256) {
        return;
      }
      job.ghost_row_of_color = effects.ghost_table.first(256);
      job.ghost_rows = effects.ghost_table.subspan(256);
      // Refuse a table whose rows do not all exist, so the blit need not check.
      for (const uint8_t ghost_row : job.ghost_row_of_color) {
        if (ghost_row != 0xff &&
            (base::ssize{ghost_row} + 1) * 256 > std::ssize(job.ghost_rows)) {
          return;
        }
      }
      effect_bits |= kBlitGhost;
    }

    if (base::Any(flags & ShapeFlags::kFading)) {
      job.fading_table = effects.fading_table;
      if (job.fading_table.size() < 256) {
        return;
      }
      effect_bits |= kBlitFading;
    }

    if (base::Any(flags & ShapeFlags::kPredator)) {
      effect_bits |= kBlitPredator;
      const int64_t start = effects.predator_offset;
      job.predator_start = static_cast<uint32_t>(
          std::abs(start) % std::ssize(job.predator_offsets));
      if (start < 0) {
        // The page's stride changes from view to view, so the down-and-left
        // offsets are worked out afresh for this one.
        std::ranges::transform(
            kPredatorLeftOffsets, job.predator_offsets.begin(),
            [&dest](const int left) { return dest.stride() - left; });
      } else {
        std::ranges::copy(kPredatorRightOffsets, job.predator_offsets.begin());
      }
    }
  }

  // Clip the destination rectangle to the view. The source is the whole frame
  // from its top left, so it starts however far the destination's near corner
  // was pulled in; clipping the far corner only shortens the lines.
  int dst_x = x;
  int dst_y = y;
  job.pixel_count = width;
  job.line_count = height;
  if (!ClipRect(dst_x, dst_y, job.pixel_count, job.line_count, dest.width(),
                dest.height())) {
    return;
  }
  // Clipping can still leave nothing: a far corner exactly on the left or
  // top edge.
  if (job.pixel_count <= 0 || job.line_count <= 0) {
    return;
  }

  job.src_stride = width;
  job.dst_stride = dest.stride();
  job.src =
      frame.subspan(base::ToSize((dst_x - x) + ((dst_y - y) * job.src_stride)));
  job.dst =
      dest.pixels().subspan(base::ToSize(dst_x + (dst_y * job.dst_stride)));

  kBlitters.at(effect_bits)(job);
}

}  // namespace engine::gfx
