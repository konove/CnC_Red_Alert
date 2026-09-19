#include "sdllib/wsa.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "base/array.h"
#include "base/attributes.h"
#include "base/buffer.h"
#include "base/flags.h"
#include "base/numeric.h"
#include "base/types.h"
#include "port/unaligned.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/gbuffer.h"
#include "sdllib/iff.h"
#include "sdllib/wwstd.h"

namespace {

// The size of the run-time animation header ANIMATE.EXE allocated in front of
// the delta buffer: the packed DOS layout with two 32-bit buffer pointers. It
// added this to largest_frame_size before saving the file, so the delta buffer
// is the file's largest_frame_size less these bytes.
constexpr int kAnimateHeaderSize = 37;

// WsaFileHeader::flags, as ANIMATE wrote them.
enum class CNC_FLAG_ENUM WsaFileFlags : uint16_t {
  kNone = 0,
  // A 768-byte palette sits between the offset table and the first frame.
  kHasPalette = 0x01,
  // The animation was made from a .LBM and a .ANM, so frame 0 is an XOR delta
  // against that picture rather than against black.
  kFrame0IsDelta = 0x02,
};

}  // namespace

// A specialization has to be declared in the template's own namespace.
template <>
inline constexpr bool base::kIsFlagEnum<WsaFileFlags> = true;

namespace {

// Header structure for the file, little-endian and unpadded.
// NOTE:  The 'total_frames' field is used to differentiate between Amiga and
// IBM animations.  Amiga animations have the HIGH bit set. Nothing in this file
// checks for it.
#pragma pack(push, 1)
struct WsaFileHeader {
  uint16_t total_frames = 0;
  uint16_t pixel_x = 0;
  uint16_t pixel_y = 0;
  uint16_t pixel_width = 0;
  uint16_t pixel_height = 0;
  // Includes kAnimateHeaderSize.
  uint16_t largest_frame_size = 0;
  WsaFileFlags flags = WsaFileFlags::kNone;
  // The first two entries of the frame offset table, read along with the header
  // because their difference is the size of frame 0. frame0_offset is 0 if the
  // file has no frame 0.
  uint32_t frame0_offset = 0;
  uint32_t frame0_end = 0;
  // The rest of the offset table follows: total_frames + 2 uint32_t entries in
  // all, one per frame, one for the loop delta that turns the last frame back
  // into frame 0, and one for the end of the data. The last entry is 0 if the
  // file has no loop delta. Then come the optional palette and the frames.
};

#pragma pack(pop)

// The header proper: frame0_offset and frame0_end belong to the offset table.
constexpr int kWsaFileHeaderSize{sizeof(WsaFileHeader) -
                                 (2 * sizeof(uint32_t))};

// Returns the position in `file_buffer` of the delta that produces `frame`, or
// 0 if the offset table has no entry for it. 0 is never a real position, since
// the table itself sits there. Frame total_frames is the loop delta and
// total_frames + 1 is the end of the data.
int64_t ResidentFrameOffset(std::span<const std::byte> file_buffer, int frame);

}  // namespace

WsaAnimation::WsaAnimation(const std::string_view file_name,
                           const std::span<uint8_t> palette) {
  const int file_handle = OpenFileHandle(file_name, FileAccess::kRead);
  if (file_handle == kInvalidHandle) {
    return;
  }
  if (!Load(file_handle, palette)) {
    Close();
  }
  CloseFileHandle(file_handle);
}

void WsaAnimation::Close() { *this = WsaAnimation(); }

bool WsaAnimation::Load(const int file_handle,
                        const std::span<uint8_t> palette) {
  // The size of the palette between the offset table and the first frame,
  // which the table's offsets do not count.
  int palette_size = 0;
  WsaFileHeader file_header;
  ReadFileHandle(file_handle, base::ObjectBytes(file_header));

  // An attached palette has to be skipped on the way to the frames, and is
  // read in if the caller gave us room for its 256 RGB triples.
  if (base::Any(file_header.flags & WsaFileFlags::kHasPalette)) {
    palette_size = 768;

    if (palette.size() >= 768) {
      // The palette follows the offset table. The header read already took the
      // table's first two entries, leaving total_frames more to skip.
      SeekFileHandle(
          file_handle,
          static_cast<int32_t>(sizeof(uint32_t) * file_header.total_frames),
          SEEK_CUR);
      ReadFileHandle(file_handle, std::as_writable_bytes(palette.first(768)));
    }
  }
  frame0_is_delta_ =
      base::Any(file_header.flags & WsaFileFlags::kFrame0IsDelta);

  // Get the total file size minus the size of the first frame, the palette and
  // the file header.  These will not be kept in the file buffer, to save even
  // more space.
  base::ssize file_buffer_size = SeekFileHandle(file_handle, 0, SEEK_END);

  // A zero offset means the file has no frame 0. Otherwise its size is cut to
  // 16 bits like every frame size in this format; a bogus size is caught by the
  // capacity check below.
  int frame0_size = 0;
  has_frame0_ = file_header.frame0_offset != 0;
  if (has_frame0_) {
    const auto full_frame0_size = static_cast<int32_t>(
        file_header.frame0_end - file_header.frame0_offset);
    frame0_size = static_cast<uint16_t>(full_frame0_size);
  }

  // What is left must at least hold the offset table, and a frame with no area
  // cannot be drawn.
  file_buffer_size -= palette_size + frame0_size + kWsaFileHeaderSize;
  if (file_buffer_size < (base::ssize{file_header.total_frames} + 2) * 4 ||
      file_header.pixel_width == 0 || file_header.pixel_height == 0) {
    return false;
  }

  // Frame 0 is read into the back of the delta buffer. A corrupt header whose
  // frame 0 is bigger than that, or whose largest_frame_size is too small to
  // include ANIMATE's header bytes, would write outside it.
  const int delta_capacity =
      file_header.largest_frame_size - kAnimateHeaderSize;
  if (delta_capacity < 0 || frame0_size > delta_capacity) {
    return false;
  }
  delta_buffer_.resize(base::ToSize(delta_capacity));

  // current_frame_ is set to total_frames_ so that DrawFrame() knows that
  // nothing has been drawn yet and frame 0 has to be applied first.
  current_frame_ = total_frames_ = file_header.total_frames;
  // The offsets are read as signed so that a negative one fails DrawFrame()'s
  // bounds check instead of wrapping to a large positive offset.
  x_ = static_cast<int16_t>(file_header.pixel_x);
  y_ = static_cast<int16_t>(file_header.pixel_y);
  width_ = file_header.pixel_width;
  height_ = file_header.pixel_height;

  // Figure how much room the frame offsets take up in the file.
  // Add 2 - one for the wrap around (loop) delta and one for the final end
  // offset.
  const int offset_table_size = (total_frames_ + 2) * 4;

  // Skip over the header information and read in the offsets. Then skip over
  // the palette and the first frame, which are not kept, and read in the
  // remaining frames.
  file_buffer_.resize(base::ToSize(file_buffer_size));
  const std::span file_buffer = file_buffer_;
  SeekFileHandle(file_handle, kWsaFileHeaderSize, SEEK_SET);
  ReadFileHandle(file_handle,
                 file_buffer.first(base::ToSize(offset_table_size)));
  SeekFileHandle(file_handle, frame0_size + palette_size, SEEK_CUR);
  ReadFileHandle(file_handle,
                 file_buffer.subspan(base::ToSize(offset_table_size)));

  // Find out if there is an ending value for the last frame, that is, an end
  // offset for the loop delta. If there is not, then this animation will not be
  // able to loop back to the beginning.
  has_loop_delta_ = ResidentFrameOffset(file_buffer_, total_frames_ + 1) != 0;

  // Figure where to back load frame 0 into the delta buffer. Compressed data
  // goes at the very end so that LCW_Uncompress() can write its output from the
  // front of the same buffer, behind the input it has yet to read; ANIMATE's
  // largest_frame_size is what makes the buffer big enough for that.
  const std::span delta_buffer = delta_buffer_;
  const auto compressed_delta =
      delta_buffer.subspan(delta_buffer.size() - base::ToSize(frame0_size));

  // Read the first frame into the delta buffer and uncompress it.
  SeekFileHandle(file_handle,
                 kWsaFileHeaderSize + offset_table_size + palette_size,
                 SEEK_SET);
  ReadFileHandle(file_handle, compressed_delta);

  // Frame 0 now waits, uncompressed, at the front of the delta buffer until the
  // first DrawFrame() applies it. With no frame 0 this decodes nothing.
  LCW_Uncompress(compressed_delta, delta_buffer);

  is_open_ = true;
  return true;
}

bool WsaAnimation::DrawFrame(GraphicViewPortClass& view,
                             const int frame_number) {
  // A closed animation has no frames, so every frame number is out of range.
  if (frame_number < 0 || total_frames_ <= frame_number) {
    return false;
  }
  // How many deltas have to be applied to get to frame_number.
  int steps = 0;
  if (!view.Lock()) {
    return false;
  }

  // The distance between rows of the destination. For a whole page that is its
  // width; a viewport (part of a buffer) also has to step over the rest of the
  // buffer's row and any surface padding.
  const int dest_stride = view.Get_Width() + view.Get_XAdd() + view.Get_Pitch();

  // Deltas are XORed straight onto the view's pixels. They are clipped to the
  // end of the pixels but not to the view's edges, so a frame that sticks out
  // would wrap onto the next row.
  if (x_ < 0 || y_ < 0 || x_ + width_ > view.Get_Width() ||
      y_ + height_ > view.Get_Height()) {
    view.Unlock();
    return false;
  }
  const std::span<uint8_t> frame_buffer =
      view.Get_Pixels().subspan(base::ToSize((y_ * dest_stride) + x_));

  // If current_frame_ is equal to total_frames_, then no animations have taken
  // place, so frame 0, which Load() left uncompressed in the delta buffer, must
  // be applied to the view if it exists.
  if (current_frame_ == total_frames_) {
    if (has_frame0_) {
      // The last parameter says whether to copy or to XOR.  If the first frame
      // is a DELTA, then it must be XOR'd onto the picture already there;
      // otherwise it replaces whatever is there. `dest_stride` is the full
      // stride, not the gap between the end of one row and the next.
      ApplyXorDeltaToView(frame_buffer, delta_buffer_, width_, dest_stride,
                          /*copy=*/!frame0_is_delta_);
    }
    current_frame_ = 0;
  }

  // Get the current frame. XOR deltas undo themselves, so the requested frame
  // can be reached by stepping in either direction, and through the loop delta
  // if there is one. Pick whichever route applies the fewest deltas.
  int cursor_frame = current_frame_;

  // Get absolute distance from our current frame to the target frame, which is
  // the cost of the route that does not wrap.
  const int distance = std::abs(cursor_frame - frame_number);

  // Direction to search for the desired frame: 1 is right, towards higher frame
  // numbers, and -1 is left. Assume we are searching right.
  int direction = 1;

  if (frame_number > cursor_frame) {
    // Calculate the number of frames to search if we go left and wrap from
    // frame 0 round to the last frame.
    steps = total_frames_ - frame_number + cursor_frame;

    // Is wrapping faster than going right? If no looping is allowed, are they
    // trying to do it anyway?
    if (steps < distance && has_loop_delta_) {
      direction = -1;  // Yes, so go left
    } else {
      steps = distance;
    }
  } else {
    // Calculate the number of frames to search if we go right and wrap from the
    // last frame round to frame 0.
    steps = total_frames_ - cursor_frame + frame_number;

    // Is going straight left at least as fast as wrapping? Or are they trying
    // to loop when they should not?
    if (steps >= distance || !has_loop_delta_) {
      direction = -1;  // Yes, so go left
      steps = distance;
    }
  }

  // Delta N turns frame N - 1 into frame N, and also back again. Going right,
  // step first and then apply the delta of the frame arrived at; going left,
  // apply the delta of the frame being left and then step.
  //
  // A delta that cannot be loaded leaves the picture alone, so stop there with
  // `shown_frame` still naming the frame the picture holds.
  int shown_frame = cursor_frame;
  bool reached = true;
  if (direction > 0) {
    for (int i = 0; i < steps; i++) {
      // Move the logical frame number ordinally right
      cursor_frame += direction;

      if (!ApplyFrameDelta(cursor_frame, frame_buffer, dest_stride)) {
        reached = false;
        break;
      }

      // Adjust the current frame number, taking into consideration that we
      // could have wrapped: delta total_frames is the loop delta, which has
      // just produced frame 0.
      if (cursor_frame == total_frames_) {
        cursor_frame = 0;
      }
      shown_frame = cursor_frame;
    }
  } else {
    for (int i = 0; i < steps; i++) {
      // If we are going backwards and we are on frame 0, the delta to get
      // to the last frame is the n + 1 delta (wrap delta). The step below then
      // lands on frame total_frames - 1.
      if (cursor_frame == 0) {
        cursor_frame = total_frames_;
      }

      if (!ApplyFrameDelta(cursor_frame, frame_buffer, dest_stride)) {
        reached = false;
        break;
      }

      cursor_frame += direction;
      shown_frame = cursor_frame;
    }
  }

  current_frame_ = shown_frame;

  view.Unlock();
  return reached;
}

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
  // Index of the next pixel in the width-wide image, not an offset into
  // `target`; the two differ once stride > width.
  size_t pixel = 0;
  while (!delta.empty()) {
    const auto command = std::to_integer<uint8_t>(delta.front());
    delta = delta.subspan(1);
    // Until shown otherwise, a short literal: `command` bytes to XOR.
    size_t count = command;
    bool run = false;   // One byte is repeated `count` times.
    bool skip = false;  // `count` pixels are left unchanged.
    if (command == 0) {
      if (delta.empty()) {
        return;
      }
      count = std::to_integer<uint8_t>(delta.front());
      delta = delta.subspan(1);
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
        const auto code = static_cast<uint16_t>(
            std::to_integer<uint8_t>(base::At(delta, 0)) |
            (std::to_integer<uint32_t>(base::At(delta, 1)) << 8));
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
    // The most pixels `target` can hold: whole rows, plus a final partial row
    // that is cut off at `width`. Checking logical pixels against it avoids
    // multiplying attacker-controlled offsets.
    const size_t capacity = ((target.size() / stride) * width) +
                            std::min(target.size() % stride, width);
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
    // Map each pixel to its row and column; a run may cross the row end and so
    // has to step over the stride - width bytes that are not part of the image.
    for (size_t i = 0; i < count; ++i) {
      const auto value = std::to_integer<uint8_t>(base::At(delta, run ? 0 : i));
      const auto offset = ((pixel / width) * stride) + (pixel % width);
      base::At(target, offset) =
          copy ? value : static_cast<uint8_t>(base::At(target, offset) ^ value);
      ++pixel;
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

namespace {

int64_t ResidentFrameOffset(const std::span<const std::byte> file_buffer,
                            const int frame) {
  // The first two table entries are always read, to size frame 0, and then the
  // entry for `frame` itself.
  if (frame < 0 || file_buffer.size() < 8 ||
      base::ToSize(frame) >= file_buffer.size() / sizeof(uint32_t)) {
    return 0;
  }
  uint32_t frame0_size = 0;
  if (const auto frame0_offset = port::ReadUnaligned<uint32_t>(file_buffer)) {
    frame0_size =
        port::ReadUnaligned<uint32_t>(file_buffer.subspan(sizeof(uint32_t))) -
        frame0_offset;
  }

  // The table holds file positions, worked out as if the file had no palette.
  // The file buffer starts at the table and leaves frame 0 out, so take off
  // the header and frame 0 to get a position in the buffer.
  const auto offset = port::ReadUnaligned<uint32_t>(
      file_buffer.subspan(base::ToSize(frame) * sizeof(uint32_t)));
  if (offset) {
    return offset - (frame0_size + kWsaFileHeaderSize);
  }
  return 0L;
}

}  // namespace

bool WsaAnimation::ApplyFrameDelta(const int delta_number,
                                   const std::span<uint8_t> dest,
                                   const int dest_stride) {
  // Get the offset of the given frame in the file buffer and its size, which
  // is (frame + 1 offset) - (offset).
  const std::span<const std::byte> file_buffer = file_buffer_;
  const int64_t frame_offset = ResidentFrameOffset(file_buffer, delta_number);
  const int64_t frame_data_size =
      ResidentFrameOffset(file_buffer, delta_number + 1) - frame_offset;

  // A corrupt offset table must not copy from outside the loaded file data or
  // past the delta buffer. A zero offset for either frame (no such delta) also
  // ends up here.
  const std::span delta_buffer = delta_buffer_;
  if (frame_offset < 0 || frame_data_size <= 0 ||
      std::cmp_greater(frame_data_size, delta_buffer.size()) ||
      std::cmp_greater(frame_offset + frame_data_size, file_buffer.size())) {
    return false;
  }

  // Copy the compressed delta to the end of the delta buffer; see Load() for
  // why the end.
  const auto data = file_buffer.subspan(base::ToSize(frame_offset),
                                        base::ToSize(frame_data_size));
  const auto compressed_delta =
      delta_buffer.subspan(delta_buffer.size() - base::ToSize(frame_data_size));
  base::CopyBytes(compressed_delta, data, frame_data_size);

  // Uncompress it to the beginning of the delta buffer, then XOR the delta
  // onto the view.
  LCW_Uncompress(compressed_delta, delta_buffer);
  ApplyXorDeltaToView(dest, delta_buffer, width_, dest_stride, /*copy=*/false);
  return true;
}
