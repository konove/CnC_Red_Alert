#include "sdllib/wsa.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <utility>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/flags.h"
#include "base/numeric.h"
#include "base/types.h"
#include "port/aligned_buffer.h"
#include "port/unaligned.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/gbuffer.h"
#include "sdllib/iff.h"
#include "sdllib/memflag.h"
#include "sdllib/wwstd.h"

// SysAnimHeaderType::flags: how Open_Animation() set the animation up. Exactly
// one of WSA_FILE and WSA_RESIDENT is set.

// Deltas are read from the file, which stays open in file_handle.
#define WSA_FILE 0x04U
// The whole file is in memory and deltas are copied out of file_buffer.
#define WSA_RESIDENT 0x08U
// Frames are built in target_buffer and then copied to the destination, rather
// than XORed straight onto it.
#define WSA_TARGET_IN_BUFFER 0x10U
// The file has no loop delta, so playback cannot wrap from the last frame to
// frame 0 or back; Animate_Frame() always takes the direct route.
#define WSA_LINEAR_ONLY 0x20U
// The file has no frame 0: the animation starts from whatever is already on
// the destination.
#define WSA_FRAME_0_ON_PAGE 0x40U
// A 768-byte palette sits between the offset table and the first frame, and
// the table's offsets do not count it.
#define WSA_PALETTE_PRESENT 0x100U
// Frame 0 is a delta against a picture the caller has already drawn, so it is
// XORed onto a direct destination instead of overwriting it.
#define WSA_FRAME_0_IS_DELTA 0x200U

// Values for the `copy` parameter of Apply_XOR_Delta_To_Page_Or_Viewport().
// They were shared with lp_asm.asm; its replacement, DecodeDelta(), treats
// anything other than DO_XOR as a copy.
#define DO_XOR 0x0
#define DO_COPY 0x01

// Run-time state of an open animation. The handle Open_Animation() returns
// points at one of these, at the start of a single allocation laid out as
//
//   SysAnimHeaderType | target buffer | delta buffer | file buffer
//
// The target buffer is absent for WSA_OPEN_DIRECT animations and the file
// buffer for animations played from disk.
struct SysAnimHeaderType {
  // The frame the destination currently shows. Equal to total_frames until the
  // first Animate_Frame(), meaning that not even frame 0 has been applied yet.
  uint16_t current_frame;
  uint16_t total_frames;
  // Where the frame goes on the destination. Read back as signed.
  uint16_t pixel_x;
  uint16_t pixel_y;
  uint16_t pixel_width;
  uint16_t pixel_height;
  // Size of delta_buffer. Unlike the file's field of the same name, this does
  // not include ANIMATE's 37 header bytes.
  uint16_t largest_frame_size;
  // Scratch space that holds one frame's delta, first compressed at the back
  // and then decompressed at the front.
  std::span<uint8_t> delta_buffer;
  // The file's offset table and frames 1 and up. Empty unless WSA_RESIDENT.
  std::span<uint8_t> file_buffer;
  // The current frame, pixel_width * pixel_height bytes. Empty unless
  // WSA_TARGET_IN_BUFFER.
  std::span<uint8_t> target_buffer;
  // An 8.3 file name and its terminator; longer names are truncated.
  char file_name[13];
  uint16_t flags;  // WSA_* header flags above.
  // New fields that ANIMATE does not know about below this point. See
  // kExtraBytesAnimateDoesNotKnowAbout.
  int16_t file_handle;  // -1 if WSA_RESIDENT; the file is already closed.
};

// The original note: "THIS IS A BAD THING. SINCE sizeof(SysAnimHeaderType)
// CHANGED, THE ANIMATE.EXE UTILITY DID NOT KNOW I UPDATED IT, IT ADDS IT TO
// largest_frame_size BEFORE SAVING IT TO THE FILE. THIS MEANS I HAVE TO ADD
// THESE charS ON NOW FOR IT TO WORK."
//
// That is, the file's largest_frame_size is the size of one allocation for the
// header plus the delta buffer, using the header ANIMATE knew: 37 bytes, the
// packed DOS layout through `flags` with two 32-bit buffer pointers. The header
// has grown since (file_handle then, the spans now), so the growth is added
// back when sizing the allocation and the delta buffer always comes out as the
// file's largest_frame_size - 37 bytes.
constexpr int kExtraBytesAnimateDoesNotKnowAbout =
    int{sizeof(SysAnimHeaderType) - 37};

// Header structure for the file, little-endian and unpadded.
// NOTE:  The 'total_frames' field is used to differentiate between Amiga and
// IBM animations.  Amiga animations have the HIGH bit set. Nothing in this file
// checks for it.
#pragma pack(push, 1)
struct WSA_FileHeaderType {
  uint16_t total_frames;
  uint16_t pixel_x;
  uint16_t pixel_y;
  uint16_t pixel_width;
  uint16_t pixel_height;
  // Includes 37 bytes for ANIMATE's idea of SysAnimHeaderType; see
  // kExtraBytesAnimateDoesNotKnowAbout.
  uint16_t largest_frame_size;
  // Bit 0: a palette is present. Bit 1: frame 0 is a delta against a picture.
  uint16_t flags;
  // The first two entries of the frame offset table, read along with the header
  // because their difference is the size of frame 0. frame0_offset is 0 if the
  // file has no frame 0.
  uint32_t frame0_offset;
  uint32_t frame0_end;
  // The rest of the offset table follows: total_frames + 2 uint32_t entries in
  // all, one per frame, one for the loop delta that turns the last frame back
  // into frame 0, and one for the end of the data. The last entry is 0 if the
  // file has no loop delta. Then come the optional palette and the frames.
};

#pragma pack(pop)

// The header proper: frame0_offset and frame0_end belong to the offset table.
constexpr int kWsaFileHeaderSize{sizeof(WSA_FileHeaderType) -
                                 (2 * sizeof(uint32_t))};

// Returns the position in `file_buffer` of the delta that produces `frame`, or
// 0 if the offset table has no entry for it. 0 is never a real position, since
// the table itself sits there. Frame total_frames is the loop delta and
// total_frames + 1 is the end of the data.
static int64_t Get_Resident_Frame_Offset(std::span<const uint8_t> file_buffer,
                                         int frame);

// As Get_Resident_Frame_Offset(), but reads the table from the open file and
// returns a file position, or 0 if the entry is missing or cannot be read.
// `palette_adjust` is the size of the file's palette, 0 if it has none.
static int64_t Get_File_Frame_Offset(int file_handle, int frame,
                                     int palette_adjust);

// Loads the delta that produces `curr_frame` from the frame before it and XORs
// it onto `dest_ptr`. `dest_w` is the stride of `dest_ptr`, used only when it
// is the destination view rather than the target buffer. Returns false, with
// `dest_ptr` untouched, if the offset table is corrupt or the file read comes
// up short.
static bool Apply_Delta(const SysAnimHeaderType* sys_header, int curr_frame,
                        std::span<uint8_t> dest_ptr, int dest_w);

void* Open_Animation(const char* file_name, WSAOpenType user_flags,
                     std::span<uint8_t> palette) {
  int palette_adjust = 0;
  int frame0_size = 0;
  base::ssize target_buffer_size = 0;
  WSA_FileHeaderType file_header = {};

  // Open the file to get the header information.
  uint16_t anim_flags = 0;
  const int fh = OpenFileHandle(file_name, FileAccess::kRead);
  if (fh == kInvalidHandle) {
    return nullptr;
  }
  ReadFileHandle(fh, base::ObjectBytes(file_header));

  // If the file has an attached palette (bit 0 of its flags), it has to be
  // allowed for in every file position from here on, and is read in if the
  // caller gave us room for its 256 RGB triples.
  if (file_header.flags & 1) {
    anim_flags |= WSA_PALETTE_PRESENT;
    palette_adjust = 768;

    if (palette.size() >= 768) {
      // The palette follows the offset table. The header read already took the
      // table's first two entries, leaving total_frames more to skip.
      SeekFileHandle(
          fh, static_cast<int32_t>(sizeof(uint32_t) * file_header.total_frames),
          SEEK_CUR);
      ReadFileHandle(fh, std::as_writable_bytes(palette.first(768)));
    }

  } else {
    palette_adjust = 0;
  }

  // Check for the flag from ANIMATE (bit 1) indicating that this animation was
  // created from a .LBM and a .ANM.  This means that the first frame is an XOR
  // delta from a picture, not from black.
  if (file_header.flags & 2) {
    anim_flags |= WSA_FRAME_0_IS_DELTA;
  }

  // Get the total file size minus the size of the first frame, the palette and
  // the file header.  These will not be kept in the file buffer, to save even
  // more space.
  base::ssize file_buffer_size = SeekFileHandle(fh, 0, SEEK_END);

  // A zero offset means the file has no frame 0. Otherwise its size is cut to
  // 16 bits like every frame size in this format; a bogus size is caught by the
  // capacity check below.
  if (file_header.frame0_offset) {
    const auto tlong = static_cast<int32_t>(file_header.frame0_end -
                                            file_header.frame0_offset);
    frame0_size = static_cast<uint16_t>(tlong);
  } else {
    anim_flags |= WSA_FRAME_0_ON_PAGE;
    frame0_size = 0;
  }

  // What is left must at least hold the offset table, and a frame with no area
  // cannot be drawn.
  file_buffer_size -= palette_adjust + frame0_size + kWsaFileHeaderSize;
  if (file_buffer_size < (base::ssize{file_header.total_frames} + 2) * 4 ||
      file_header.pixel_width == 0 || file_header.pixel_height == 0) {
    CloseFileHandle(fh);
    return nullptr;
  }

  // We need to determine the buffer sizes required for the animation.  At a
  // minimum, we need a target buffer for the uncompressed frame and a delta
  // buffer for the delta data.  We may be able to make the file resident,
  // so we will determine the file size.
  //
  // A direct animation XORs its deltas straight onto the destination and needs
  // no target buffer; otherwise reserve one byte per pixel of the frame.
  if (base::Any(user_flags & WSA_OPEN_DIRECT)) {
    target_buffer_size = 0L;
  } else {
    anim_flags |= WSA_TARGET_IN_BUFFER;
    target_buffer_size =
        base::ssize{file_header.pixel_width} * file_header.pixel_height;
  }

  // Despite the name, this is the size of the header and the delta buffer
  // together, as ANIMATE saved it; see kExtraBytesAnimateDoesNotKnowAbout.
  const base::ssize delta_buffer_size =
      base::ssize{file_header.largest_frame_size} +
      kExtraBytesAnimateDoesNotKnowAbout;

  // Frame 0 is read into the back of the delta buffer, which holds the file's
  // largest_frame_size - 37 bytes. A corrupt header whose frame 0 is bigger
  // than that, or whose largest_frame_size is too small to include ANIMATE's 37
  // header bytes, would write outside it.
  const base::ssize frame_capacity =
      delta_buffer_size - base::ssize{sizeof(SysAnimHeaderType)};
  if (frame_capacity < 0 || frame0_size > frame_capacity) {
    CloseFileHandle(fh);
    return nullptr;
  }
  const base::ssize min_buffer_size = target_buffer_size + delta_buffer_size;
  const base::ssize max_buffer_size = min_buffer_size + file_buffer_size;

  // Hold the whole file in memory unless the caller wants it read from disk or
  // there is only room for the minimum.
  base::ssize buffer_size = base::Any(user_flags & WSA_OPEN_FROM_DISK)
                                ? min_buffer_size
                                : max_buffer_size;
  if (buffer_size > Ram_Free(MEM_NORMAL)) {
    if (min_buffer_size > Ram_Free(MEM_NORMAL)) {
      CloseFileHandle(fh);
      return nullptr;
    }
    buffer_size = min_buffer_size;
  }

  // Value-initialized, so the target buffer starts out black. Close_Animation()
  // frees it.
  auto* allocation = new uint8_t[base::ToSize(buffer_size)]();
  // This owner allocates exactly buffer_size elements.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const auto buffer = std::span(allocation, base::ToSize(buffer_size));

  // Set the pointers to the RAM buffers: the target buffer follows the header
  // and the delta buffer follows that. The file buffer, if any, comes last.
  const auto target_buffer = buffer.subspan(sizeof(SysAnimHeaderType),
                                            base::ToSize(target_buffer_size));
  const auto delta_buffer = buffer.subspan(
      sizeof(SysAnimHeaderType) + base::ToSize(target_buffer_size),
      base::ToSize(frame_capacity));

  // Poke data into the system animation header (start of the buffer).
  // current_frame is set to total_frames so that Animate_Frame() knows that
  // nothing has been drawn yet and frame 0 has to be applied first.

  // new[] storage is aligned for any fundamental type.
  auto* sys_header = port::AlignedObject<SysAnimHeaderType>(buffer.data());
  sys_header->current_frame = sys_header->total_frames =
      file_header.total_frames;
  sys_header->pixel_x = file_header.pixel_x;
  sys_header->pixel_y = file_header.pixel_y;
  sys_header->pixel_width = file_header.pixel_width;
  sys_header->pixel_height = file_header.pixel_height;
  sys_header->delta_buffer = delta_buffer;
  sys_header->target_buffer = target_buffer;
  sys_header->largest_frame_size = static_cast<uint16_t>(
      delta_buffer_size - base::ssize{sizeof(SysAnimHeaderType)});

  absl::SNPrintF(sys_header->file_name, sizeof(sys_header->file_name), "%s",
                 file_name);

  // Figure how much room the frame offsets take up in the file.
  // Add 2 - one for the wrap around (loop) delta and one for the final end
  // offset.
  const int offsets_size = (file_header.total_frames + 2) * 4;

  // Is there room for the whole file?
  if (buffer_size == max_buffer_size) {
    // Set the file buffer pointer, skip over the header information and read
    // in the offsets. Then skip over the palette and the first frame, which are
    // not kept, and read in the remaining frames.
    sys_header->file_buffer = buffer.subspan(base::ToSize(min_buffer_size),
                                             base::ToSize(file_buffer_size));
    SeekFileHandle(fh, kWsaFileHeaderSize, SEEK_SET);
    ReadFileHandle(fh, std::as_writable_bytes(sys_header->file_buffer.first(
                           base::ToSize(offsets_size))));
    SeekFileHandle(fh, frame0_size + palette_adjust, SEEK_CUR);
    ReadFileHandle(fh, std::as_writable_bytes(sys_header->file_buffer.subspan(
                           base::ToSize(offsets_size))));

    // Find out if there is an ending value for the last frame, that is, an end
    // offset for the loop delta. If there is not, then this animation will not
    // be able to loop back to the beginning.
    if (Get_Resident_Frame_Offset(sys_header->file_buffer,
                                  sys_header->total_frames + 1)) {
      anim_flags |= WSA_RESIDENT;
    } else {
      anim_flags |= WSA_LINEAR_ONLY | WSA_RESIDENT;
    }
  } else {
    // There is only room for the minimum, or the caller asked for disk: leave
    // the frames in the file and make the same loop delta check there.
    if (Get_File_Frame_Offset(fh, sys_header->total_frames + 1,
                              palette_adjust)) {
      anim_flags |= WSA_FILE;
    } else {
      anim_flags |= WSA_LINEAR_ONLY | WSA_FILE;
    }
    sys_header->file_buffer = {};
  }

  // Figure where to back load frame 0 into the delta buffer. Compressed data
  // goes at the very end so that LCW_Uncompress() can write its output from the
  // front of the same buffer, behind the input it has yet to read; ANIMATE's
  // largest_frame_size is what makes the buffer big enough for that.
  const auto delta_back = delta_buffer.subspan(
      base::ToSize(sys_header->largest_frame_size - frame0_size));

  // Read the first frame into the delta buffer and uncompress it (below).
  // Then close the file, unless later frames will come from it.
  SeekFileHandle(fh, kWsaFileHeaderSize + offsets_size + palette_adjust,
                 SEEK_SET);
  ReadFileHandle(fh, std::as_writable_bytes(delta_back));

  // We do not use the file handle when the file is in RAM; -1 marks it closed.
  if (anim_flags & WSA_RESIDENT) {
    sys_header->file_handle = static_cast<int16_t>(-1);
    CloseFileHandle(fh);
  } else {
    sys_header->file_handle = static_cast<int16_t>(fh);
  }

  // Frame 0 now waits, uncompressed, at the front of the delta buffer until the
  // first Animate_Frame() applies it. With no frame 0 this decodes nothing.
  LCW_Uncompress(delta_back, delta_buffer);

  // Finally set the flags.
  sys_header->flags = anim_flags;

  // The handle is the allocation itself; Close_Animation() deletes it as such.
  return buffer.data();
}

void Close_Animation(void* handle) {
  if (handle == nullptr) {
    return;
  }

  // The system header sits at the beginning of the handle space.
  auto* sys_header = static_cast<SysAnimHeaderType*>(handle);

  // Close the WSA file if it was disk based.
  if (sys_header->flags & WSA_FILE) {
    CloseFileHandle(sys_header->file_handle);
  }

  // The handle is the start of the buffer Open_Animation() allocated.
  delete[] static_cast<uint8_t*>(handle);
}

bool Animate_Frame(void* handle, GraphicViewPortClass& view, int frame_number) {
  if (handle == nullptr || frame_number < 0) {
    return false;
  }
  // How many deltas have to be applied to get to frame_number.
  int search_frames = 0;
  // Where the deltas are applied: the target buffer or the view's own pixels.
  std::span<uint8_t> frame_buffer;
  // Are we going directly to the destination?
  bool direct_to_dest = false;

  // Assign local pointer to the beginning of the buffer where the system
  // information resides.
  auto* sys_header = static_cast<SysAnimHeaderType*>(handle);

  // Get the total number of frames.
  const int total_frames = sys_header->total_frames;

  // Is the frame number valid?
  if (total_frames <= frame_number) {
    return false;
  }

  if (!view.Lock()) {
    return false;
  }

  // The distance between rows of the destination. For a whole page that is its
  // width; a viewport (part of a buffer) also has to step over the rest of the
  // buffer's row and any surface padding.
  const int dest_width = view.Get_Width() + view.Get_XAdd() + view.Get_Pitch();

  // The frame is drawn at the offset stored in the animation file. The offsets
  // are read as signed so that a negative one fails the bounds check below
  // instead of wrapping to a large positive offset.
  const int x_pixel = static_cast<int16_t>(sys_header->pixel_x);
  const int y_pixel = static_cast<int16_t>(sys_header->pixel_y);

  // Check to see if we are using a buffer inside of the animation buffer or if
  // it is being drawn directly to the destination page or buffer.
  if (sys_header->flags & WSA_TARGET_IN_BUFFER) {
    // Get a pointer to the frame in animation buffer.
    frame_buffer = sys_header->target_buffer;
    direct_to_dest = false;
  } else {
    // Deltas are clipped to the end of the view's pixels but not to its edges,
    // so a frame that sticks out would wrap onto the next row. Buffer_To_Page()
    // clips the buffered case instead.
    if (x_pixel < 0 || y_pixel < 0 ||
        x_pixel + sys_header->pixel_width > view.Get_Width() ||
        y_pixel + sys_header->pixel_height > view.Get_Height()) {
      view.Unlock();
      return false;
    }
    frame_buffer = view.Get_Pixels().subspan(
        base::ToSize((y_pixel * dest_width) + x_pixel));
    direct_to_dest = true;
  }
  // If current_frame is equal to total_frames, then no animations have taken
  // place, so frame 0, which Open_Animation() left uncompressed in the delta
  // buffer, must be applied to the frame_buffer/page if it exists.
  if (std::cmp_equal(sys_header->current_frame, total_frames)) {
    // Call apply delta telling it whether to copy or to xor depending on if the
    // target is a page or a buffer.

    if (!(sys_header->flags & WSA_FRAME_0_ON_PAGE)) {
      if (direct_to_dest) {
        // The last parameter says whether to copy or to XOR.  If the first
        // frame is a DELTA, then it must be XOR'd onto the picture already
        // there; otherwise it replaces whatever is there. `dest_width` is the
        // full stride, not the gap between the end of one row and the next.
        Apply_XOR_Delta_To_Page_Or_Viewport(
            frame_buffer, sys_header->delta_buffer, sys_header->pixel_width,
            dest_width,
            sys_header->flags & WSA_FRAME_0_IS_DELTA ? DO_XOR : DO_COPY);
      } else {
        // The target buffer starts out zeroed (black), so an XOR onto it is a
        // copy. WSA_FRAME_0_IS_DELTA has no effect here: the picture is not in
        // the target buffer to be XORed with.
        Apply_XOR_Delta(frame_buffer, sys_header->delta_buffer);
      }
    }
    sys_header->current_frame = 0;
  }

  // Get the current frame. XOR deltas undo themselves, so the requested frame
  // can be reached by stepping in either direction, and through the loop delta
  // if there is one. Pick whichever route applies the fewest deltas.
  int curr_frame = sys_header->current_frame;

  // Get absolute distance from our current frame to the target frame, which is
  // the cost of the route that does not wrap.
  const int distance = std::abs(curr_frame - frame_number);

  // Direction to search for the desired frame: 1 is right, towards higher frame
  // numbers, and -1 is left. Assume we are searching right.
  int search_dir = 1;

  if (frame_number > curr_frame) {
    // Calculate the number of frames to search if we go left and wrap from
    // frame 0 round to the last frame.
    search_frames = total_frames - frame_number + curr_frame;

    // Is wrapping faster than going right? If no looping is allowed, are they
    // trying to do it anyway?
    if (search_frames < distance && !(sys_header->flags & WSA_LINEAR_ONLY)) {
      search_dir = -1;  // Yes, so go left
    } else {
      search_frames = distance;
    }
  } else {
    // Calculate the number of frames to search if we go right and wrap from the
    // last frame round to frame 0.
    search_frames = total_frames - curr_frame + frame_number;

    // Is going straight left at least as fast as wrapping? Or are they trying
    // to loop when they should not?
    if (search_frames >= distance || sys_header->flags & WSA_LINEAR_ONLY) {
      search_dir = -1;  // Yes, so go left
      search_frames = distance;
    }
  }

  // Delta N turns frame N - 1 into frame N, and also back again. Going right,
  // step first and then apply the delta of the frame arrived at; going left,
  // apply the delta of the frame being left and then step.
  //
  // A delta that cannot be loaded leaves the picture alone, so stop there with
  // `shown_frame` still naming the frame the picture holds.
  int shown_frame = curr_frame;
  bool reached = true;
  if (search_dir > 0) {
    for (int loop = 0; loop < search_frames; loop++) {
      // Move the logical frame number ordinally right
      curr_frame += search_dir;

      if (!Apply_Delta(sys_header, curr_frame, frame_buffer, dest_width)) {
        reached = false;
        break;
      }

      // Adjust the current frame number, taking into consideration that we
      // could have wrapped: delta total_frames is the loop delta, which has
      // just produced frame 0.
      if (curr_frame == total_frames) {
        curr_frame = 0;
      }
      shown_frame = curr_frame;
    }
  } else {
    for (int loop = 0; loop < search_frames; loop++) {
      // If we are going backwards and we are on frame 0, the delta to get
      // to the last frame is the n + 1 delta (wrap delta). The step below then
      // lands on frame total_frames - 1.
      if (curr_frame == 0) {
        curr_frame = total_frames;
      }

      if (!Apply_Delta(sys_header, curr_frame, frame_buffer, dest_width)) {
        reached = false;
        break;
      }

      curr_frame += search_dir;
      shown_frame = curr_frame;
    }
  }

  sys_header->current_frame = static_cast<uint16_t>(shown_frame);

  // If we did this all in a hidden buffer, then copy it to the desired page or
  // viewport.
  if (sys_header->flags & WSA_TARGET_IN_BUFFER) {
    Buffer_To_Page(x_pixel, y_pixel, sys_header->pixel_width,
                   sys_header->pixel_height, frame_buffer, view);
  }

  view.Unlock();
  return reached;
}

int Get_Animation_Frame_Count(void* handle) {
  if (!handle) {
    return 0;
  }
  auto* sys_header = static_cast<SysAnimHeaderType*>(handle);
  // Read as signed, so an Amiga animation (high bit of total_frames set; see
  // WSA_FileHeaderType) reports a negative count.
  return static_cast<int16_t>(sys_header->total_frames);
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
void DecodeDelta(std::span<uint8_t> target, std::span<const std::byte> delta,
                 size_t width, size_t stride, bool copy) {
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

unsigned int Apply_XOR_Delta(std::span<uint8_t> target,
                             std::span<const std::byte> delta) {
  // One row as wide as the whole target makes the decoder treat it as a
  // contiguous run of pixels.
  DecodeDelta(target, delta, target.size(), target.size(), false);
  return 0;
}

unsigned int Apply_XOR_Delta(std::span<uint8_t> target,
                             std::span<const uint8_t> delta) {
  DecodeDelta(target, std::as_bytes(delta), target.size(), target.size(),
              false);
  return 0;
}

void Apply_XOR_Delta_To_Page_Or_Viewport(std::span<uint8_t> target,
                                         std::span<const uint8_t> delta,
                                         int width, int nextrow, int copy) {
  if (width <= 0 || nextrow <= 0) {
    return;
  }
  DecodeDelta(target, std::as_bytes(delta), base::ToSize(width),
              base::ToSize(nextrow), copy != DO_XOR);
}

static int64_t Get_Resident_Frame_Offset(std::span<const uint8_t> file_buffer,
                                         int frame) {
  // The first two table entries are always read, to size frame 0, and then the
  // entry for `frame` itself.
  if (frame < 0 || file_buffer.size() < 8 ||
      base::ToSize(frame) >= file_buffer.size() / sizeof(uint32_t)) {
    return 0;
  }
  uint32_t frame0_size = 0;
  const auto first = port::ReadUnaligned<uint32_t>(std::as_bytes(file_buffer));
  if (first) {
    frame0_size = port::ReadUnaligned<uint32_t>(
                      std::as_bytes(file_buffer.subspan(sizeof(uint32_t)))) -
                  first;
  } else {
    frame0_size = 0;
  }

  // The table holds file positions, worked out as if the file had no palette.
  // The file buffer starts at the table and leaves frame 0 out, so take off
  // the header and frame 0 to get a position in the buffer.
  const auto offset = port::ReadUnaligned<uint32_t>(std::as_bytes(
      file_buffer.subspan(base::ToSize(frame) * sizeof(uint32_t))));
  if (offset) {
    return offset - (frame0_size + kWsaFileHeaderSize);
  }
  return 0L;
}

static int64_t Get_File_Frame_Offset(int file_handle, int frame,
                                     int palette_adjust) {
  uint32_t offset = 0;

  SeekFileHandle(file_handle, (frame * 4) + kWsaFileHeaderSize, SEEK_SET);

  // A zero entry means there is no such delta; it must stay zero rather than
  // pick up the palette size, or callers could not tell.
  if (ReadFileHandle(file_handle, base::ObjectBytes(offset)) !=
          sizeof(uint32_t) ||
      offset == 0) {
    return 0;
  }
  // The table's offsets are worked out as if the file had no palette.
  return int64_t{offset} + palette_adjust;
}

static bool Apply_Delta(const SysAnimHeaderType* sys_header, int curr_frame,
                        std::span<uint8_t> dest_ptr, int dest_w) {
  int64_t frame_data_size = 0;
  int64_t frame_offset = 0;

  const int palette_adjust = sys_header->flags & WSA_PALETTE_PRESENT ? 768 : 0;
  auto delta_back = sys_header->delta_buffer;

  if (sys_header->flags & WSA_RESIDENT) {
    // Get the offset of the given frame in the resident file and its size,
    // which is (frame + 1 offset) - (offset). Point at the delta data, figure
    // the offset to load it into the end of the delta buffer, and copy it
    // there; see Open_Animation() for why the end.
    frame_offset =
        Get_Resident_Frame_Offset(sys_header->file_buffer, curr_frame);
    frame_data_size =
        Get_Resident_Frame_Offset(sys_header->file_buffer, curr_frame + 1) -
        frame_offset;

    // A corrupt offset table must not copy from outside the loaded file data
    // or past the delta buffer, which holds largest_frame_size bytes. A zero
    // offset for either frame (no such delta) also ends up here.
    if (frame_offset < 0 || frame_data_size <= 0 ||
        std::cmp_greater(frame_data_size, sys_header->largest_frame_size) ||
        std::cmp_greater(frame_offset + frame_data_size,
                         sys_header->file_buffer.size())) {
      return false;
    }

    const auto data = sys_header->file_buffer.subspan(
        base::ToSize(frame_offset), base::ToSize(frame_data_size));
    delta_back = delta_back.subspan(
        base::ToSize(sys_header->largest_frame_size - frame_data_size));

    base::CopyBytes(std::as_writable_bytes(delta_back), std::as_bytes(data),
                    frame_data_size);

  } else if (sys_header->flags & WSA_FILE) {
    // The file is not in RAM, so go to the file on disk, which is still open.
    // Get the offset of the given frame and its size, which is
    // (frame + 1 offset) - (offset), and return if Get_File_Frame_Offset()
    // failed. Seek to the delta data, figure the offset to load it into the end
    // of the delta buffer and read it in, returning if the correct amount was
    // not read. The original asked "need error handling????" at both returns;
    // the caller still ignores them.
    const int file_handle = sys_header->file_handle;
    SeekFileHandle(file_handle, 0L, SEEK_SET);

    frame_offset =
        Get_File_Frame_Offset(file_handle, curr_frame, palette_adjust);
    frame_data_size =
        Get_File_Frame_Offset(file_handle, curr_frame + 1, palette_adjust) -
        frame_offset;

    // A corrupt offset table must not size a read past the delta buffer,
    // which holds largest_frame_size bytes.
    if (!frame_offset || frame_data_size <= 0 ||
        std::cmp_greater(frame_data_size, sys_header->largest_frame_size)) {
      return false;
    }

    SeekFileHandle(file_handle, static_cast<int32_t>(frame_offset), SEEK_SET);
    delta_back = delta_back.subspan(
        base::ToSize(sys_header->largest_frame_size - frame_data_size));

    if (ReadFileHandle(file_handle, std::as_writable_bytes(delta_back)) !=
        static_cast<int>(frame_data_size)) {
      return false;
    }
  }

  // Uncompress data at end of delta buffer to the beginning of delta buffer,
  // then apply the XOR delta to the target buffer or straight to the view.
  LCW_Uncompress(delta_back, sys_header->delta_buffer);

  if (sys_header->flags & WSA_TARGET_IN_BUFFER) {
    Apply_XOR_Delta(dest_ptr, sys_header->delta_buffer);
  } else {
    Apply_XOR_Delta_To_Page_Or_Viewport(dest_ptr, sys_header->delta_buffer,
                                        sys_header->pixel_width, dest_w,
                                        DO_XOR);
  }

  return true;
}
