// re-implemented from assembly in 2keyfbuf.asm
#include "tech/2keyfbuf.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "port/unaligned.h"
#include "sdllib/gbuffer.h"
#include "sdllib/shape.h"

// should match 2keyfram.cpp
struct ShapeHeaderType {
  unsigned draw_flags;  // only 16 bits used
  char* shape_data;     // really an offset
  int shape_buffer;     // 0 or 1
};

// Per-line blit effect bits stored in the shape header's line flags.
constexpr uint32_t kBlitTransparent = 1;
constexpr uint32_t kBlitGhost = 2;
constexpr uint32_t kBlitFading = 4;
constexpr uint32_t kBlitPredator = 8;
constexpr uint32_t kBlitSkip = 16;
// The first four, used for the "old" draw.
constexpr uint32_t kBlitOld =
    kBlitTransparent | kBlitGhost | kBlitFading | kBlitPredator;
constexpr uint32_t kBlitAll = kBlitOld | kBlitSkip;

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

// copied from blit funcs
static inline uint32_t Make_Code(int x, int y, int w, int h) {
  return (x < 0 ? 0b1000U : 0U) | (x >= w ? 0b0100U : 0U) |
         (y < 0 ? 0b0010U : 0U) | (y >= h ? 0b0001U : 0U);
}

static void Setup_Shape_Header(int pixel_width, int pixel_height,
                               std::span<const std::byte> src,
                               ShapeHeaderType& header,
                               std::span<std::byte> line_flags_out,
                               ShapeFlags_Type flags,
                               std::span<const uint8_t> /*Translucent*/,
                               std::span<const uint8_t> IsTranslucent) {
  header.draw_flags = static_cast<uint32_t>(ShapeEffectFlags(flags));
  std::size_t input = 0;
  std::size_t output = 0;
  do {
    uint32_t line_flags = 0;
    int trans_count = 0;
    int x_count = pixel_width;
    do {
      const int pixel = std::to_integer<uint8_t>(src[input++]);
      if (!pixel && base::Any(flags & SHAPE_TRANS)) {
        line_flags = kBlitTransparent;
        trans_count++;  // keep track of number of transparent pixels
      } else {
        if (base::Any(flags & SHAPE_PREDATOR)) {
          line_flags |= kBlitPredator;
        }

        if (base::Any(flags & SHAPE_GHOST) &&
            IsTranslucent[static_cast<std::size_t>(pixel)] != 0xFF) {
          line_flags |= kBlitGhost;
        }

        if (base::Any(flags & SHAPE_FADING)) {
          line_flags |= kBlitFading;
        }
      }
    } while (--x_count);

    // all pixels in the line were transparent so we dont need to draw it at all
    if (line_flags & kBlitTransparent && trans_count == pixel_width) {
      line_flags = kBlitSkip;
    }

    line_flags_out[output++] = static_cast<std::byte>(line_flags);
  } while (--pixel_height != 0);
}

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
      auto pixel = std::to_integer<uint8_t>(src_offset[input++]);
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
            const auto sample = output + base::ToSize(base::At(BFPredTable, pred_index));
            if (sample < dst_offset.size()) {
              pixel = dst_offset[sample];
            }
            // NOLINTNEXTLINE(bugprone-signed-bitwise)
            BFPredOffset = (BFPredOffset + 2) & PRED_MASK;
          }
        }

        if (flags & kBlitGhost) {
          const uint8_t is_trans =
              IsTranslucent[static_cast<std::size_t>(pixel)];
          if (is_trans != 0xFF) {  // is it a translucent color?
            pixel = Translucent[(is_trans * 256) + dst_offset[output]];
          }
        }

        if (flags & kBlitFading) {
          // run color through fading table
          for (int f = 0; f < FadingNum; f++) {
            pixel = FadingTable[pixel];
          }
        }

        dst_offset[output] = pixel;
      }
      output++;
    }

    input += static_cast<std::size_t>(src_adjust_width);
    output += static_cast<std::size_t>(dst_adjust_width);
  } while (--line_count);
}

void Buffer_Frame_To_Page(int x, int y, const int w, const int h,
                          std::span<std::byte> src, GraphicViewPortClass& dest,
                          ShapeFlags_Type flags, const ShapeEffects& effects) {
  if (src.empty() || w <= 0 || h <= 0) {
    return;
  }

  std::span<const uint8_t> IsTranslucent;
  std::span<const uint8_t> Translucent;
  std::span<const uint8_t> FadingTable;
  int FadingNum = 0;

  ShapeHeaderType header{};
  std::span<std::byte> header_bytes;

  bool use_new_draw = !UseOldShapeDraw && UseBigShapeBuffer;

  // Save the line attributes pointers and modify the src pointer to point to
  // the actual image.
  if (use_new_draw) {
    if (src.size() < sizeof(ShapeHeaderType) + static_cast<std::size_t>(h)) {
      return;
    }
    header_bytes = src;
    header = port::ReadUnaligned<ShapeHeaderType>(src);
    const auto shape_buffer = std::as_writable_bytes(
        header.shape_buffer ? TheaterShapeBufferBytes : BigShapeBufferBytes);
    const auto offset = std::bit_cast<uintptr_t>(header.shape_data);
    if (offset > shape_buffer.size()) {
      return;
    }
    src = shape_buffer.subspan(offset);
  }
  // else just use the old shape drawing system

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

  // If this is the first time through for this shape then
  // set up the shape header

  bool use_all_flags = false;

  if (use_new_draw &&
      (header.draw_flags == ~0U ||
       header.draw_flags != static_cast<uint32_t>(ShapeEffectFlags(flags)))) {
    Setup_Shape_Header(w, h, src, header,
                       header_bytes.subspan(sizeof(ShapeHeaderType)), flags,
                       Translucent, IsTranslucent);
    port::WriteUnaligned(header_bytes, header);
    // ShapeJumpTableAddress = AllFlagsJumpTable;
    use_all_flags = true;
  } else {
    // int eax = 0;
    // if (flags & SHAPE_PREDATOR) eax |= kBlitPredator;
    // if (flags & SHAPE_FADING) eax |= kBlitFading;
    // if (flags & SHAPE_TRANS) eax |= kBlitTransparent;
    // if (flags & SHAPE_GHOST) eax |= kBlitGhost;
    //
    // eax <<= 7;
    // ShapeJumpTableAddress = NewShapeJumpTable + eax
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
      base::At(BFPredNegTable, off + 8) = static_cast<int16_t>(
          base::At(BFPredNegTable, off) + dest.Get_Width() + dest.Get_XAdd() +
          dest.Get_Pitch());
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

  const uint32_t code0 =
      Make_Code(dst_x0, dst_y0, dest.Get_Width(), dest.Get_Height());
  const uint32_t code1 =
      Make_Code(dst_x1, dst_y1, dest.Get_Width() + 1, dest.Get_Height() + 1);

  // outside
  if (code0 & code1) {
    return;
  }

  if (code0 | code1) {
    // If the shape needs to be clipped then we cant handle it with the new
    // header system so draw it with the old shape drawer.
    use_new_draw = false;

    // apply clip
    if (code0 & 0b1000) {
      src_x0 -= dst_x0;
      dst_x0 = 0;
    }
    if (code1 & 0b0100) {
      dst_x1 = dest.Get_Width();
    }
    if (code0 & 0b0010) {
      src_y0 -= dst_y0;
      dst_y0 = 0;
    }
    if (code1 & 0b0001) {
      dst_y1 = dest.Get_Height();
    }
  }

  // do blit
  auto src_offset = src.subspan(
      base::ToSize(src_x0 + (static_cast<base::ssize>(src_y0) * w)));
  const int src_adjust_width = w - (dst_x1 - dst_x0);

  const base::ssize dst_area =
      dest.Get_XAdd() + dest.Get_Width() + dest.Get_Pitch();
  auto dst_offset =
      dest.Get_Pixels().subspan(base::ToSize(dst_x0 + (dst_y0 * dst_area)));
  const int dst_adjust_width = static_cast<int>(dst_area - (dst_x1 - dst_x0));

  if (!use_new_draw) {
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
        Do_Old_Blit<kBlitTransparent>(line_count, pixel_count, src_offset,
                                      dst_offset, src_adjust_width,
                                      dst_adjust_width, Translucent,
                                      IsTranslucent, FadingNum, FadingTable);
        break;
      case kBlitGhost:  // BF_Ghost
        Do_Old_Blit<kBlitGhost>(line_count, pixel_count, src_offset, dst_offset,
                                src_adjust_width, dst_adjust_width, Translucent,
                                IsTranslucent, FadingNum, FadingTable);
        break;
      case kBlitGhost | kBlitTransparent:  // BF_Ghost_Trans
        Do_Old_Blit<kBlitGhost | kBlitTransparent>(
            line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
            dst_adjust_width, Translucent, IsTranslucent, FadingNum,
            FadingTable);
        break;
      case kBlitFading:  // BF_Fading
        Do_Old_Blit<kBlitFading>(line_count, pixel_count, src_offset,
                                 dst_offset, src_adjust_width, dst_adjust_width,
                                 Translucent, IsTranslucent, FadingNum,
                                 FadingTable);
        break;
      case kBlitFading | kBlitTransparent:  // BF_Fading_Trans
        Do_Old_Blit<kBlitFading | kBlitTransparent>(
            line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
            dst_adjust_width, Translucent, IsTranslucent, FadingNum,
            FadingTable);
        break;
      case kBlitFading | kBlitGhost:  // BF_Ghost_Fading
        Do_Old_Blit<kBlitFading | kBlitGhost>(
            line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
            dst_adjust_width, Translucent, IsTranslucent, FadingNum,
            FadingTable);
        break;
      case kBlitFading | kBlitGhost |
          kBlitTransparent:  // BF_Ghost_Fading_Trans
        Do_Old_Blit<kBlitFading | kBlitGhost | kBlitTransparent>(
            line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
            dst_adjust_width, Translucent, IsTranslucent, FadingNum,
            FadingTable);
        break;
      case kBlitPredator:  // BF_Predator
        Do_Old_Blit<kBlitPredator>(line_count, pixel_count, src_offset,
                                   dst_offset, src_adjust_width,
                                   dst_adjust_width, Translucent, IsTranslucent,
                                   FadingNum, FadingTable);
        break;
      case kBlitPredator | kBlitTransparent:  // BF_Predator_Trans
        Do_Old_Blit<kBlitPredator | kBlitTransparent>(
            line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
            dst_adjust_width, Translucent, IsTranslucent, FadingNum,
            FadingTable);
        break;
      case kBlitPredator | kBlitGhost:  // BF_Predator_Ghost
        Do_Old_Blit<kBlitPredator | kBlitGhost>(
            line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
            dst_adjust_width, Translucent, IsTranslucent, FadingNum,
            FadingTable);
        break;
      case kBlitPredator | kBlitGhost |
          kBlitTransparent:  // BF_Predator_Ghost_Trans
        Do_Old_Blit<kBlitPredator | kBlitGhost | kBlitTransparent>(
            line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
            dst_adjust_width, Translucent, IsTranslucent, FadingNum,
            FadingTable);
        break;
      case kBlitPredator | kBlitFading:  // BF_Predator_Fading
        Do_Old_Blit<kBlitPredator | kBlitFading>(
            line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
            dst_adjust_width, Translucent, IsTranslucent, FadingNum,
            FadingTable);
        break;
      case kBlitPredator | kBlitFading |
          kBlitTransparent:  // BF_Predator_Fading_Trans
        Do_Old_Blit<kBlitPredator | kBlitFading | kBlitTransparent>(
            line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
            dst_adjust_width, Translucent, IsTranslucent, FadingNum,
            FadingTable);
        break;
      case kBlitPredator | kBlitFading |
          kBlitGhost:  // BF_Predator_Ghost_Fading
        Do_Old_Blit<kBlitPredator | kBlitFading | kBlitGhost>(
            line_count, pixel_count, src_offset, dst_offset, src_adjust_width,
            dst_adjust_width, Translucent, IsTranslucent, FadingNum,
            FadingTable);
        break;
      case kBlitPredator | kBlitFading | kBlitGhost |
          kBlitTransparent:  // BF_Predator_Ghost_Fading_Trans
        Do_Old_Blit<kBlitPredator | kBlitFading | kBlitGhost |
                    kBlitTransparent>(line_count, pixel_count, src_offset,
                                      dst_offset, src_adjust_width,
                                      dst_adjust_width, Translucent,
                                      IsTranslucent, FadingNum, FadingTable);
        break;
      default:
        break;
    }
  } else {
    // super jump table fun!
    absl::PrintF("%s new f %x all flags %i\n", __func__,
                 header.draw_flags & kBlitAll, static_cast<int>(use_all_flags));
  }
}
