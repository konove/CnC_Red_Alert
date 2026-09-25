// re-implemented from assembly in 2keyfbuf.asm
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

// Per-line blit effect bits, computed fresh for every draw.
constexpr uint32_t kBlitTransparent = 1;
constexpr uint32_t kBlitGhost = 2;
constexpr uint32_t kBlitFading = 4;
constexpr uint32_t kBlitPredator = 8;
constexpr uint32_t kBlitOld =
    kBlitTransparent | kBlitGhost | kBlitFading | kBlitPredator;

// The predator table walk is legacy signed byte-offset arithmetic from the
// assembler blitter: a negative predator offset is meant to walk backwards
// into BFPredNegTable, so these masks stay on the signed value.
// TODO: a negative offset becomes ~0xFF | k and indexes BFPredTable at
// about -126, well before BFPredNegTable; that read is out of bounds.
constexpr int PRED_MASK = 0xE;

static int BFPredOffset;
static int BFPartialCount;
static int BFPartialPred;
static int16_t BFPredNegTable[]{-1, -3, -2, -5, -2, -4, -3, -1,
                                // calculated
                                0, 0, 0, 0, 0, 0, 0, 0};

static int16_t BFPredTable[]{1, 3, 2, 5, 2, 3, 4, 1};

// single helper that handles all combinations
// templated on flags to avoid writing every combination
template <uint32_t flags>
static void Do_Old_Blit(int line_count, int pixel_count,
                        std::span<const std::byte> src_offset,
                        std::span<uint8_t> dst_offset, int src_adjust_width,
                        int dst_adjust_width,
                        std::span<const uint8_t> Translucent,
                        std::span<const uint8_t> IsTranslucent, int FadingNum,
                        std::span<const uint8_t> FadingTable) {
  std::size_t input = 0;
  std::size_t output = 0;
  do {
    // original asm unrolled this 32 times
    for (int x = 0; x < pixel_count; x++) {
      auto pixel = std::to_integer<uint8_t>(base::At(src_offset, input++));
      if (pixel || !(flags & kBlitTransparent)) {
        if (flags & kBlitPredator) {
          const int pred = BFPartialCount + BFPartialPred;
          BFPartialCount = pred % 256;
          // is this a predator pixel?
          if (pred >= 256) {
            // pick up a color offset a pseudo-random amount from the current
            // viewport address
            // NOLINTNEXTLINE(bugprone-signed-bitwise)
            const int pred_index = BFPredOffset >> 1;
            const auto sample =
                output + base::ToSize(base::At(BFPredTable, pred_index));
            if (sample < dst_offset.size()) {
              pixel = base::At(dst_offset, sample);
            }
            // NOLINTNEXTLINE(bugprone-signed-bitwise)
            BFPredOffset = (BFPredOffset + 2) & PRED_MASK;
          }
        }

        if (flags & kBlitGhost) {
          const uint8_t is_trans =
              base::At(IsTranslucent, static_cast<std::size_t>(pixel));
          if (is_trans != 0xFF) {  // is it a translucent color?
            pixel = base::At(Translucent,
                             (is_trans * 256) + base::At(dst_offset, output));
          }
        }

        if (flags & kBlitFading) {
          // run color through fading table
          for (int f = 0; f < FadingNum; f++) {
            pixel = base::At(FadingTable, pixel);
          }
        }

        base::At(dst_offset, output) = pixel;
      }
      output++;
    }

    input += static_cast<std::size_t>(src_adjust_width);
    output += static_cast<std::size_t>(dst_adjust_width);
  } while (--line_count);
}

void Buffer_Frame_To_Page(int x, int y, const int w, const int h,
                          std::span<std::byte> src, PixelView& dest,
                          ShapeFlags_Type flags, const ShapeEffects& effects) {
  if (src.empty() || w <= 0 || h <= 0) {
    return;
  }

  std::span<const uint8_t> IsTranslucent;
  std::span<const uint8_t> Translucent;
  std::span<const uint8_t> FadingTable;
  int FadingNum = 0;

  if (static_cast<std::size_t>(w) * static_cast<std::size_t>(h) > src.size()) {
    return;
  }
  uint32_t jflags = 0;  // clear jump flags

  // See if we need to center the frame
  if (base::Any(flags & SHAPE_CENTER)) {
    x -= w / 2;
    y -= h / 2;
  }

  if (base::Any(flags & SHAPE_TRANS)) {
    jflags |= kBlitTransparent;
  }

  if (base::Any(flags & SHAPE_GHOST)) {
    // are we ghosting this shape
    jflags |= kBlitGhost;
    IsTranslucent = effects.ghost_table;
    if (IsTranslucent.size() < 256) {
      return;
    }
    Translucent = IsTranslucent.subspan(256);
    IsTranslucent = IsTranslucent.first(256);
    for (const uint8_t row : IsTranslucent) {
      if (row != 0xff &&
          (static_cast<std::size_t>(row) + 1) * 256 > Translucent.size()) {
        return;
      }
    }
  }

  // are we fading this shape
  if (base::Any(flags & SHAPE_FADING)) {
    // save address of fading tbl
    FadingTable = effects.fading_table;
    if (FadingTable.size() < 256) {
      return;
    }
    // get fade num, no need for more than 63
    FadingNum = effects.fading_count % 64;
    jflags |= kBlitFading;

    if (!FadingNum) {
      flags &= ~SHAPE_FADING;  // don't fade
    }

    // ShapeJumpTableAddress[4] = Single_Line_Single_Fade
    // ShapeJumpTableAddress[5] = Single_Line_Single_Fade_Trans

    if (FadingNum != 1) {
      // ShapeJumpTableAddress[4] = Single_Line_Fading
      // ShapeJumpTableAddress[5] = Single_Line_Fading_Trans
    }
  }

  if (base::Any(flags & SHAPE_PREDATOR))  // is predator effect on
  {
    int offset = effects.predator_offset;
    jflags |= kBlitPredator;

    offset *= 2;

    if (offset < 0) {
      // NOLINTNEXTLINE(bugprone-signed-bitwise)
      offset = (-offset & PRED_MASK) | ~0xFF;  // will be ffffff00-ffffff0E
    } else {
      offset &= PRED_MASK;  // NOLINT(bugprone-signed-bitwise)
    }

    BFPredOffset = offset;
    BFPartialCount = 0;   // clear the partial count
    BFPartialPred = 256;  // init partial to off

    for (int off = 0; off < 8; off++) {
      base::At(BFPredNegTable, off + 8) =
          static_cast<int16_t>(base::At(BFPredNegTable, off) + dest.width() +
                               dest.x_add() + dest.pitch());
    }
  }

  // is this a partial pred?
  if (base::Any(flags & SHAPE_PARTIAL)) {
    BFPartialPred = effects.partial_predator % 256;
  }

  // clip dest
  int src_x0 = 0;
  int src_y0 = 0;

  int dst_x0 = x;
  int dst_y0 = y;
  int dst_x1 = x + w;
  int dst_y1 = y + h;

  const OutCode code0 = OutCodeOf(dst_x0, dst_y0, dest.width(), dest.height());
  const OutCode code1 =
      OutCodeOf(dst_x1, dst_y1, dest.width() + 1, dest.height() + 1);

  // outside
  if (base::Any(code0 & code1)) {
    return;
  }

  if (base::Any(code0 | code1)) {
    // apply clip
    if (base::Any(code0 & OutCode::kLeft)) {
      src_x0 -= dst_x0;
      dst_x0 = 0;
    }
    if (base::Any(code1 & OutCode::kRight)) {
      dst_x1 = dest.width();
    }
    if (base::Any(code0 & OutCode::kAbove)) {
      src_y0 -= dst_y0;
      dst_y0 = 0;
    }
    if (base::Any(code1 & OutCode::kBelow)) {
      dst_y1 = dest.height();
    }
  }

  // do blit
  auto src_offset = src.subspan(
      base::ToSize(src_x0 + (static_cast<base::ssize>(src_y0) * w)));
  const int src_adjust_width = w - (dst_x1 - dst_x0);

  const base::ssize dst_area = dest.x_add() + dest.width() + dest.pitch();
  auto dst_offset =
      dest.pixels().subspan(base::ToSize(dst_x0 + (dst_y0 * dst_area)));
  const int dst_adjust_width = static_cast<int>(dst_area - (dst_x1 - dst_x0));

  if (dst_x1 <= dst_x0 || dst_y1 <= dst_y0) {
    return;
  }

  const int pixel_count = dst_x1 - dst_x0;
  int line_count = dst_y1 - dst_y0;

  switch (jflags & kBlitOld) {
    case 0:  // BF_Copy
    {
      // copy lines
      do {
        base::CopyBytes(std::as_writable_bytes(dst_offset), src_offset,
                        pixel_count);
        if (line_count > 1) {
          src_offset = src_offset.subspan(base::ToSize(w));
        }
        if (line_count > 1) {
          dst_offset = dst_offset.subspan(base::ToSize(dst_area));
        }
      } while (--line_count);
      break;
    }
    case kBlitTransparent:  // BF_Trans
      Do_Old_Blit<kBlitTransparent>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitGhost:  // BF_Ghost
      Do_Old_Blit<kBlitGhost>(line_count, pixel_count, src_offset, dst_offset,
                              src_adjust_width, dst_adjust_width, Translucent,
                              IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitGhost | kBlitTransparent:  // BF_Ghost_Trans
      Do_Old_Blit<kBlitGhost | kBlitTransparent>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitFading:  // BF_Fading
      Do_Old_Blit<kBlitFading>(line_count, pixel_count, src_offset, dst_offset,
                               src_adjust_width, dst_adjust_width, Translucent,
                               IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitFading | kBlitTransparent:  // BF_Fading_Trans
      Do_Old_Blit<kBlitFading | kBlitTransparent>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitFading | kBlitGhost:  // BF_Ghost_Fading
      Do_Old_Blit<kBlitFading | kBlitGhost>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitFading | kBlitGhost | kBlitTransparent:  // BF_Ghost_Fading_Trans
      Do_Old_Blit<kBlitFading | kBlitGhost | kBlitTransparent>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitPredator:  // BF_Predator
      Do_Old_Blit<kBlitPredator>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitPredator | kBlitTransparent:  // BF_Predator_Trans
      Do_Old_Blit<kBlitPredator | kBlitTransparent>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitPredator | kBlitGhost:  // BF_Predator_Ghost
      Do_Old_Blit<kBlitPredator | kBlitGhost>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitPredator | kBlitGhost |
        kBlitTransparent:  // BF_Predator_Ghost_Trans
      Do_Old_Blit<kBlitPredator | kBlitGhost | kBlitTransparent>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitPredator | kBlitFading:  // BF_Predator_Fading
      Do_Old_Blit<kBlitPredator | kBlitFading>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitPredator | kBlitFading |
        kBlitTransparent:  // BF_Predator_Fading_Trans
      Do_Old_Blit<kBlitPredator | kBlitFading | kBlitTransparent>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitPredator | kBlitFading | kBlitGhost:  // BF_Predator_Ghost_Fading
      Do_Old_Blit<kBlitPredator | kBlitFading | kBlitGhost>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    case kBlitPredator | kBlitFading | kBlitGhost |
        kBlitTransparent:  // BF_Predator_Ghost_Fading_Trans
      Do_Old_Blit<kBlitPredator | kBlitFading | kBlitGhost | kBlitTransparent>(
          line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
          dst_adjust_width, Translucent, IsTranslucent, FadingNum, FadingTable);
      break;
    default:
      break;
  }
}
