#include "tech/wsa.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "base/attributes.h"
#include "base/buffer.h"
#include "base/flags.h"
#include "base/numeric.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "port/unaligned.h"
#include "sdllib/gbuffer.h"
#include "sdllib/iff.h"
#include "sdllib/xor_delta.h"
#include "tech/file.h"
#include "tech/game_file.h"

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

// The bytes of an attached palette: 256 RGB triples.
constexpr size_t kPaletteSize = 768;

}  // namespace

WsaAnimation::WsaAnimation(const std::string_view file_name,
                           const std::span<uint8_t> palette) {
  GameFile file(file_name);
  if (file.Open()) {
    Load(file, palette);
  }
}

WsaAnimation::WsaAnimation(File& file, const std::span<uint8_t> palette) {
  Load(file, palette);
}

void WsaAnimation::Close() { *this = WsaAnimation(); }

void WsaAnimation::Load(File& file, const std::span<uint8_t> palette) {
  WsaFileHeader file_header;
  if (!file.ReadObject(file_header)) {
    return;
  }

  // The table has an offset per frame, one for the wrap around (loop) delta
  // and one for the final end offset. Whatever follows it, the palette or
  // else frame 0, starts at table_end.
  const int offset_table_size =
      (file_header.total_frames + 2) * int{sizeof(uint32_t)};
  const int table_end = kWsaFileHeaderSize + offset_table_size;

  // An attached palette sits between the offset table and the first frame,
  // and the table's offsets do not count it. It is read in if the caller gave
  // us room for it.
  const int palette_size =
      base::Any(file_header.flags & WsaFileFlags::kHasPalette)
          ? int{kPaletteSize}
          : 0;
  if (palette_size != 0 && palette.size() >= kPaletteSize) {
    file.Seek(table_end, SeekOrigin::kBegin);
    file.Read(palette.first(kPaletteSize));
  }

  // A zero offset means the file has no frame 0. Otherwise its size is cut to
  // 16 bits like every frame size in this format; a bogus size is caught by the
  // capacity check below.
  const bool has_frame0 = file_header.frame0_offset != 0;
  const uint32_t full_frame0_size =
      has_frame0 ? file_header.frame0_end - file_header.frame0_offset : 0;
  const int frame0_size = static_cast<uint16_t>(full_frame0_size);

  // The header, the palette and frame 0 are not kept in the file buffer. What
  // is left must at least hold the offset table, and a frame with no area
  // cannot be drawn.
  const base::ssize file_buffer_size =
      file.Size() - (palette_size + frame0_size + kWsaFileHeaderSize);
  if (file_buffer_size < offset_table_size || file_header.pixel_width == 0 ||
      file_header.pixel_height == 0) {
    return;
  }

  // Frame 0 is read into the back of the delta buffer. A corrupt header whose
  // frame 0 is bigger than that, or whose largest_frame_size is too small to
  // include ANIMATE's header bytes, would write outside it.
  const int delta_capacity =
      file_header.largest_frame_size - kAnimateHeaderSize;
  if (delta_capacity < 0 || frame0_size > delta_capacity) {
    return;
  }

  // The file has passed its checks; nothing before this point touches the
  // animation, so that a rejected file leaves it closed.
  total_frames_ = file_header.total_frames;
  // The offsets are read as signed so that a negative one fails DrawFrame()'s
  // bounds check instead of wrapping to a large positive offset.
  x_ = static_cast<int16_t>(file_header.pixel_x);
  y_ = static_cast<int16_t>(file_header.pixel_y);
  width_ = file_header.pixel_width;
  height_ = file_header.pixel_height;
  has_frame0_ = has_frame0;
  frame0_is_delta_ =
      base::Any(file_header.flags & WsaFileFlags::kFrame0IsDelta);
  offset_bias_ = full_frame0_size + kWsaFileHeaderSize;

  // Read in the offset table, then skip over the palette and the first frame,
  // which are not kept, and read in the remaining frames.
  file_buffer_.resize(base::ToSize(file_buffer_size));
  const std::span file_buffer = file_buffer_;
  file.Seek(kWsaFileHeaderSize, SeekOrigin::kBegin);
  file.Read(file_buffer.first(base::ToSize(offset_table_size)));
  file.Seek(frame0_size + palette_size, SeekOrigin::kCurrent);
  file.Read(file_buffer.subspan(base::ToSize(offset_table_size)));

  // Find out if there is an ending value for the last frame, that is, an end
  // offset for the loop delta. If there is not, then this animation will not be
  // able to loop back to the beginning.
  has_loop_delta_ = ResidentFrameOffset(total_frames_ + 1) != 0;

  // Figure where to back load frame 0 into the delta buffer. Compressed data
  // goes at the very end so that LCW_Uncompress() can write its output from the
  // front of the same buffer, behind the input it has yet to read; ANIMATE's
  // largest_frame_size is what makes the buffer big enough for that.
  delta_buffer_.resize(base::ToSize(delta_capacity));
  const std::span delta_buffer = delta_buffer_;
  const auto compressed_delta =
      delta_buffer.subspan(delta_buffer.size() - base::ToSize(frame0_size));
  file.Seek(table_end + palette_size, SeekOrigin::kBegin);
  file.Read(compressed_delta);

  // Frame 0 now waits, uncompressed, at the front of the delta buffer until the
  // first DrawFrame() applies it. With no frame 0 this decodes nothing.
  LCW_Uncompress(compressed_delta, delta_buffer);
}

bool WsaAnimation::DrawFrame(GraphicViewPortClass& view,
                             const int frame_number) {
  // A closed animation has no frames, so every frame number is out of range.
  if (frame_number < 0 || total_frames_ <= frame_number) {
    return false;
  }
  // Deltas are XORed straight onto the view's pixels. They are clipped to the
  // end of the pixels but not to the view's edges, so a frame that sticks out
  // would wrap onto the next row.
  if (x_ < 0 || y_ < 0 || x_ + width_ > view.Get_Width() ||
      y_ + height_ > view.Get_Height()) {
    return false;
  }
  if (!view.Lock()) {
    return false;
  }

  // The distance between rows of the destination. For a whole page that is its
  // width; a viewport (part of a buffer) also has to step over the rest of the
  // buffer's row and any surface padding.
  const int dest_stride = view.Get_Width() + view.Get_XAdd() + view.Get_Pitch();
  const std::span<uint8_t> frame_buffer =
      view.Get_Pixels().subspan(base::ToSize((y_ * dest_stride) + x_));

  // Frame 0, which Load() left uncompressed in the delta buffer, comes first.
  // If it is a delta it is XORed onto the picture already there; otherwise it
  // replaces whatever is there.
  if (current_frame_ == kNothingDrawn) {
    if (has_frame0_) {
      ApplyXorDeltaToView(frame_buffer, delta_buffer_, width_, dest_stride,
                          /*copy=*/!frame0_is_delta_);
    }
    current_frame_ = 0;
  }

  // XOR deltas undo themselves, so the requested frame can be reached by
  // stepping in either direction, and through the loop delta if there is one.
  // The two routes add up to a full lap; take the wrapping one only if it is
  // strictly shorter, which also turns the direction of travel around.
  const int distance = std::abs(frame_number - current_frame_);
  const int wrap_distance = total_frames_ - distance;
  const bool wrap = has_loop_delta_ && wrap_distance < distance;
  const bool forward = (frame_number > current_frame_) != wrap;
  const int steps = wrap ? wrap_distance : distance;

  // Delta N turns frame N - 1 into frame N, and also back again, and delta
  // total_frames_ is the loop delta between the last frame and frame 0. Going
  // forward, apply the delta of the frame being arrived at; going back, the
  // delta of the frame being left.
  //
  // A delta that cannot be loaded leaves the picture alone, so stop there with
  // current_frame_ still naming the frame the picture holds.
  bool reached = true;
  for (int i = 0; i < steps; ++i) {
    const int last_delta = current_frame_ == 0 ? total_frames_ : current_frame_;
    const int delta_number = forward ? current_frame_ + 1 : last_delta;
    if (!ApplyFrameDelta(delta_number, frame_buffer, dest_stride)) {
      reached = false;
      break;
    }
    current_frame_ = forward ? delta_number % total_frames_ : delta_number - 1;
  }

  view.Unlock();
  return reached;
}

int64_t WsaAnimation::ResidentFrameOffset(const int frame) const {
  const std::span<const std::byte> file_buffer = file_buffer_;
  if (frame < 0 ||
      base::ToSize(frame) >= file_buffer.size() / sizeof(uint32_t)) {
    return 0;
  }
  const auto offset = port::ReadUnaligned<uint32_t>(
      file_buffer.subspan(base::ToSize(frame) * sizeof(uint32_t)));
  return offset == 0 ? 0 : offset - offset_bias_;
}

bool WsaAnimation::ApplyFrameDelta(const int delta_number,
                                   const std::span<uint8_t> dest,
                                   const int dest_stride) {
  // The delta runs from its own offset to that of the next one.
  const int64_t frame_offset = ResidentFrameOffset(delta_number);
  const int64_t frame_data_size =
      ResidentFrameOffset(delta_number + 1) - frame_offset;

  // A corrupt offset table must not copy from outside the loaded file data or
  // past the delta buffer. A zero offset for either frame (no such delta) also
  // ends up here.
  const std::span<const std::byte> file_buffer = file_buffer_;
  const std::span delta_buffer = delta_buffer_;
  if (frame_data_size <= 0 ||
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
