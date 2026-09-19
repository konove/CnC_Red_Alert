#include "sdllib/xor_delta.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "base/numeric.h"
#include "port/unaligned.h"

namespace {

// Applies an uncompressed XOR delta (Westwood's "format 40") to an image
// `width` pixels wide whose rows start `stride` bytes apart in `target`. With
// `copy` set the delta's bytes replace the pixels instead of being XORed onto
// them. The delta is a list of commands that walk the pixels in order:
//
//   00 nn vv          XOR the next nn pixels with vv
//   01..7F ...        XOR the next 1..127 pixels with the bytes that follow
//   81..FF            skip 1..127 pixels
//   80 00 00          end of the delta
//   80 lo hi, hi bits 0x: skip hi:lo pixels
//                     10: XOR the next hi:lo & 0x3FFF pixels with the bytes
//                         that follow
//                     11: XOR the next hi:lo & 0x3FFF pixels with the one byte
//                         that follows
//
// Decoding stops at the end of `delta`, at a command that is cut short or has
// a zero count, or at one that would run past the end of `target`.
void DecodeDelta(const std::span<uint8_t> target,
                 std::span<const std::byte> delta, const size_t width,
                 const size_t stride, const bool copy) {
  if (width == 0 || stride < width) {
    return;
  }
  // The most pixels `target` can hold: whole rows, plus a final partial row
  // that is cut off at `width`. Checking logical pixels against it avoids
  // multiplying attacker-controlled offsets.
  const size_t capacity = ((target.size() / stride) * width) +
                          std::min(target.size() % stride, width);
  // Index of the next pixel in the width-wide image, not an offset into
  // `target`; the two differ once stride > width.
  size_t pixel = 0;
  while (!delta.empty()) {
    const auto command = std::to_integer<uint8_t>(base::ConsumeFront(delta));
    // Until shown otherwise, a short literal: `command` bytes to XOR.
    size_t count = command;
    bool run = false;   // One byte is repeated `count` times.
    bool skip = false;  // `count` pixels are left unchanged.
    if (command == 0) {
      if (delta.empty()) {
        return;
      }
      count = std::to_integer<uint8_t>(base::ConsumeFront(delta));
      run = true;
    } else if ((command & 0x80) != 0) {
      count = command & 0x7f;
      skip = true;
      // 0x80 escapes to a 16-bit little-endian count whose top bits select the
      // operation.
      if (count == 0) {
        if (delta.size() < 2) {
          return;
        }
        const auto code = port::ReadUnaligned<uint16_t>(delta);
        delta = delta.subspan(2);
        if (code == 0) {
          return;
        }
        skip = (code & 0x8000) == 0;
        run = (code & 0xc000) == 0xc000;
        count = code & (run ? 0x3fffU : 0x7fffU);
      }
    }
    // No command has a use for a zero count, so the delta is corrupt.
    if (count == 0) {
      return;
    }
    if (pixel > capacity || count > capacity - pixel) {
      return;
    }
    if (skip) {
      pixel += count;
      continue;
    }
    if (delta.size() < (run ? 1 : count)) {
      return;
    }
    // A command may cross the row end, and so has to step over the stride -
    // width bytes that are not part of the image: take it a row segment at a
    // time. The capacity check has shown that every segment lies in `target`.
    for (size_t done = 0; done < count;) {
      const size_t column = pixel % width;
      const size_t length = std::min(count - done, width - column);
      const auto segment =
          target.subspan(((pixel / width) * stride) + column, length);
      if (run) {
        const auto run_value = std::to_integer<uint8_t>(delta.front());
        std::ranges::transform(
            segment, segment.begin(), [=](const uint8_t old) {
              return static_cast<uint8_t>(copy ? run_value : old ^ run_value);
            });
      } else {
        std::ranges::transform(
            segment, delta.subspan(done, length), segment.begin(),
            [=](const uint8_t old, const std::byte value) {
              const auto literal = std::to_integer<uint8_t>(value);
              return static_cast<uint8_t>(copy ? literal : old ^ literal);
            });
      }
      pixel += length;
      done += length;
    }
    delta = delta.subspan(run ? 1 : count);
  }
}

}  // namespace

void ApplyXorDelta(const std::span<uint8_t> target,
                   const std::span<const std::byte> delta) {
  // One row as wide as the whole target makes the decoder treat it as a
  // contiguous run of pixels.
  DecodeDelta(target, delta, target.size(), target.size(), false);
}

void ApplyXorDeltaToView(const std::span<uint8_t> target,
                         const std::span<const std::byte> delta,
                         const int width, const int stride, const bool copy) {
  if (width <= 0 || stride <= 0) {
    return;
  }
  DecodeDelta(target, delta, base::ToSize(width), base::ToSize(stride), copy);
}
