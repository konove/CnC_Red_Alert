// Re-implemented from the assembler blitter, kept for reference in
// src/ra/2keyfbuf.asm.

#include "engine/gfx/2keyfbuf.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "base/buffer.h"
#include "base/clip.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"

// The effects one draw needs, one bit each, computed from the SHAPE_* flags on
// every call. Together they pick the BlitWithEffects instantiation, as the
// assembler's FLAG_* bits indexed its table of line routines.
constexpr uint32_t kBlitTransparent = 1;
constexpr uint32_t kBlitGhost = 2;
constexpr uint32_t kBlitFading = 4;
constexpr uint32_t kBlitPredator = 8;
constexpr uint32_t kBlitOld =
    kBlitTransparent | kBlitGhost | kBlitFading | kBlitPredator;

// The predator walk keeps the assembler's arithmetic: BFPredOffset is a byte
// offset into a table of 16-bit entries, so it steps by 2 and PRED_MASK keeps
// it on the eight entries (0..14). The assembler made a negative predator
// offset 0xFFFFFF00 | k, which added to BFPredTable's address landed in
// BFPredNegTable's computed half, 256 bytes below it in its data segment; here
// BFPredSteps says which of the two tables the walk is on.
constexpr int PRED_MASK = 0xE;

// Predator state for the draw in progress, reset by each draw that asks for
// the effect.
// The byte offset of the next BFPredSteps entry to sample with (see above).
static int BFPredOffset;
// Accumulates BFPartialPred per pixel; each time it passes 256 the pixel gets
// the effect and the count keeps the remainder.
static int BFPartialCount;
// How many pixels in 256 get the effect; 256 means all of them.
static int BFPartialPred;
// The offsets a negative predator offset samples at. The first eight are the
// assembler's; the second eight are computed per draw as one row down plus
// the first eight, that is one row below and one to five pixels left.
static int16_t BFPredNegTable[]{-1, -3, -2, -5, -2, -4, -3, -1,
                                // calculated
                                0, 0, 0, 0, 0, 0, 0, 0};

// How many pixels to the right of the one being drawn a predator pixel
// samples, walked in order and wrapped. Sampling the page instead of drawing
// the shape's color shows a displaced copy of the background, the shimmer.
static constexpr int16_t BFPredTable[]{1, 3, 2, 5, 2, 3, 4, 1};

// The table the walk samples with: BFPredTable, or the computed half of
// BFPredNegTable for a negative predator offset.
static std::span<const int16_t> BFPredSteps;

// Draws line_count lines of pixel_count pixels from `src` onto `dst` with the
// effects in kEffects, stepping src_skip and dst_skip past the end of each
// line to reach the start of the next. The assembler had a routine per
// combination; the template generates them. The effects apply in the
// assembler's order: the predator sample replaces the shape's color, the ghost
// table blends that with the page, and the fading table remaps the result.
template <uint32_t kEffects>
static void BlitWithEffects(int line_count, int pixel_count,
                            std::span<const std::byte> src,
                            std::span<uint8_t> dst, int src_skip, int dst_skip,
                            std::span<const uint8_t> ghost_rows,
                            std::span<const uint8_t> ghost_row_of_color,
                            int FadingNum,
                            std::span<const uint8_t> fading_table) {
  base::ssize src_index = 0;
  base::ssize dst_index = 0;
  do {
    // The assembler unrolled this loop 32 times.
    for (int column = 0; column < pixel_count; column++) {
      auto pixel = std::to_integer<uint8_t>(base::At(src, src_index++));
      if (pixel || !(kEffects & kBlitTransparent)) {
        if (kEffects & kBlitPredator) {
          const int pred = BFPartialCount + BFPartialPred;
          BFPartialCount = pred % 256;
          // Is this a predator pixel?
          if (pred >= 256) {
            // Take the color from the page a table-driven distance to the
            // right. Near the end of the page there is nothing to sample and
            // the shape's own color stays.
            // NOLINTNEXTLINE(bugprone-signed-bitwise)
            const int pred_index = BFPredOffset >> 1;
            const base::ssize sample_index =
                dst_index + base::At(BFPredSteps, pred_index);
            if (sample_index < std::ssize(dst)) {
              pixel = base::At(dst, sample_index);
            }
            // NOLINTNEXTLINE(bugprone-signed-bitwise)
            BFPredOffset = (BFPredOffset + 2) & PRED_MASK;
          }
        }

        if (kEffects & kBlitGhost) {
          const uint8_t ghost_row = base::At(ghost_row_of_color, pixel);
          // 0xFF marks an opaque color; anything else is a remap row.
          if (ghost_row != 0xFF) {
            pixel = base::At(ghost_rows,
                             (ghost_row * 256) + base::At(dst, dst_index));
          }
        }

        if (kEffects & kBlitFading) {
          // Run the color through the fading table once per fade step.
          for (int f = 0; f < FadingNum; f++) {
            pixel = base::At(fading_table, pixel);
          }
        }

        base::At(dst, dst_index) = pixel;
      }
      dst_index++;
    }

    src_index += src_skip;
    dst_index += dst_skip;
  } while (--line_count);
}

void DrawShapeFrame(int x, int y, const int width, const int height,
                    std::span<const std::byte> frame, PixelView& dest,
                    ShapeFlags_Type flags, const ShapeEffects& effects) {
  if (frame.empty() || width <= 0 || height <= 0) {
    return;
  }

  std::span<const uint8_t> ghost_row_of_color;
  std::span<const uint8_t> ghost_rows;
  std::span<const uint8_t> fading_table;
  int FadingNum = 0;

  if (base::ssize{width} * height > std::ssize(frame)) {
    return;
  }
  // The kBlit* bits of the effects this draw needs.
  uint32_t effect_bits = 0;

  // SHAPE_CENTER: x,y name the frame's center rather than its top left.
  if (base::Any(flags & SHAPE_CENTER)) {
    x -= width / 2;
    y -= height / 2;
  }

  if (base::Any(flags & SHAPE_TRANS)) {
    effect_bits |= kBlitTransparent;
  }

  if (base::Any(flags & SHAPE_GHOST)) {
    effect_bits |= kBlitGhost;
    ghost_row_of_color = effects.ghost_table;
    if (ghost_row_of_color.size() < 256) {
      return;
    }
    ghost_rows = ghost_row_of_color.subspan(256);
    ghost_row_of_color = ghost_row_of_color.first(256);
    // Refuse a table whose rows do not all exist, so the blit need not check.
    for (const uint8_t ghost_row : ghost_row_of_color) {
      if (ghost_row != 0xff &&
          (base::ssize{ghost_row} + 1) * 256 > std::ssize(ghost_rows)) {
        return;
      }
    }
  }

  if (base::Any(flags & SHAPE_FADING)) {
    fading_table = effects.fading_table;
    if (fading_table.size() < 256) {
      return;
    }
    // The assembler kept the low six bits: no fade needs more than 63 steps.
    FadingNum = effects.fading_count % 64;
    effect_bits |= kBlitFading;

    if (!FadingNum) {
      flags &= ~SHAPE_FADING;
    }
  }

  if (base::Any(flags & SHAPE_PREDATOR)) {
    int offset = effects.predator_offset;
    effect_bits |= kBlitPredator;

    // Entries are two bytes wide.
    offset *= 2;

    if (offset < 0) {
      offset = -offset & PRED_MASK;  // NOLINT(bugprone-signed-bitwise)
      BFPredSteps = std::span(BFPredNegTable).subspan(8);
    } else {
      offset &= PRED_MASK;  // NOLINT(bugprone-signed-bitwise)
      BFPredSteps = BFPredTable;
    }

    BFPredOffset = offset;
    BFPartialCount = 0;
    // Every pixel, unless SHAPE_PARTIAL below says otherwise.
    BFPartialPred = 256;

    // The page's stride changes from view to view, so the down-and-left
    // offsets are worked out afresh for this one.
    for (int off = 0; off < 8; off++) {
      base::At(BFPredNegTable, off + 8) =
          static_cast<int16_t>(base::At(BFPredNegTable, off) + dest.width() +
                               dest.x_add() + dest.pitch());
    }
  }

  if (base::Any(flags & SHAPE_PARTIAL)) {
    BFPartialPred = effects.partial_predator % 256;
  }

  // Clip the destination rectangle to the view and move the source origin by
  // however much came off the left and top. The far corner is tested against
  // a window one larger because it is exclusive (see OutCodeOf).
  int src_x0 = 0;
  int src_y0 = 0;

  int dst_x0 = x;
  int dst_y0 = y;
  int dst_x1 = x + width;
  int dst_y1 = y + height;

  const OutCode near_corner =
      OutCodeOf(dst_x0, dst_y0, dest.width(), dest.height());
  const OutCode far_corner =
      OutCodeOf(dst_x1, dst_y1, dest.width() + 1, dest.height() + 1);

  // Wholly past one edge.
  if (base::Any(near_corner & far_corner)) {
    return;
  }

  if (base::Any(near_corner | far_corner)) {
    if (base::Any(near_corner & OutCode::kLeft)) {
      src_x0 -= dst_x0;
      dst_x0 = 0;
    }
    if (base::Any(far_corner & OutCode::kRight)) {
      dst_x1 = dest.width();
    }
    if (base::Any(near_corner & OutCode::kAbove)) {
      src_y0 -= dst_y0;
      dst_y0 = 0;
    }
    if (base::Any(far_corner & OutCode::kBelow)) {
      dst_y1 = dest.height();
    }
  }

  // The first source and destination pixels, and how far past the end of a
  // clipped line each must skip to reach the start of the next.
  auto src_line = frame.subspan(
      base::ToSize(src_x0 + (static_cast<base::ssize>(src_y0) * width)));
  const int src_skip = width - (dst_x1 - dst_x0);

  const base::ssize dst_stride = dest.x_add() + dest.width() + dest.pitch();
  auto dst_line =
      dest.pixels().subspan(base::ToSize(dst_x0 + (dst_y0 * dst_stride)));
  const int dst_skip = static_cast<int>(dst_stride - (dst_x1 - dst_x0));

  // Clipping can still leave nothing: a far corner exactly on the left or
  // top edge.
  if (dst_x1 <= dst_x0 || dst_y1 <= dst_y0) {
    return;
  }

  const int pixel_count = dst_x1 - dst_x0;
  int line_count = dst_y1 - dst_y0;

  switch (effect_bits & kBlitOld) {
    case 0: {  // BF_Copy
      // No effects: whole lines at a time. The advance is skipped after the
      // last line, whose tail may be all the buffer has left.
      do {
        base::CopyBytes(std::as_writable_bytes(dst_line), src_line,
                        pixel_count);
        if (line_count > 1) {
          src_line = src_line.subspan(base::ToSize(width));
        }
        if (line_count > 1) {
          dst_line = dst_line.subspan(base::ToSize(dst_stride));
        }
      } while (--line_count);
      break;
    }
    case kBlitTransparent:  // BF_Trans
      BlitWithEffects<kBlitTransparent>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitGhost:  // BF_Ghost
      BlitWithEffects<kBlitGhost>(line_count, pixel_count, src_line, dst_line,
                                  src_skip, dst_skip, ghost_rows,
                                  ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitGhost | kBlitTransparent:  // BF_Ghost_Trans
      BlitWithEffects<kBlitGhost | kBlitTransparent>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitFading:  // BF_Fading
      BlitWithEffects<kBlitFading>(line_count, pixel_count, src_line, dst_line,
                                   src_skip, dst_skip, ghost_rows,
                                   ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitFading | kBlitTransparent:  // BF_Fading_Trans
      BlitWithEffects<kBlitFading | kBlitTransparent>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitFading | kBlitGhost:  // BF_Ghost_Fading
      BlitWithEffects<kBlitFading | kBlitGhost>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitFading | kBlitGhost | kBlitTransparent:  // BF_Ghost_Fading_Trans
      BlitWithEffects<kBlitFading | kBlitGhost | kBlitTransparent>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitPredator:  // BF_Predator
      BlitWithEffects<kBlitPredator>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitPredator | kBlitTransparent:  // BF_Predator_Trans
      BlitWithEffects<kBlitPredator | kBlitTransparent>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitPredator | kBlitGhost:  // BF_Predator_Ghost
      BlitWithEffects<kBlitPredator | kBlitGhost>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitPredator | kBlitGhost |
        kBlitTransparent:  // BF_Predator_Ghost_Trans
      BlitWithEffects<kBlitPredator | kBlitGhost | kBlitTransparent>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitPredator | kBlitFading:  // BF_Predator_Fading
      BlitWithEffects<kBlitPredator | kBlitFading>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitPredator | kBlitFading |
        kBlitTransparent:  // BF_Predator_Fading_Trans
      BlitWithEffects<kBlitPredator | kBlitFading | kBlitTransparent>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitPredator | kBlitFading | kBlitGhost:  // BF_Predator_Ghost_Fading
      BlitWithEffects<kBlitPredator | kBlitFading | kBlitGhost>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    case kBlitPredator | kBlitFading | kBlitGhost |
        kBlitTransparent:  // BF_Predator_Ghost_Fading_Trans
      BlitWithEffects<kBlitPredator | kBlitFading | kBlitGhost |
                      kBlitTransparent>(
          line_count, pixel_count, src_line, dst_line, src_skip, dst_skip,
          ghost_rows, ghost_row_of_color, FadingNum, fading_table);
      break;
    default:
      break;
  }
}
