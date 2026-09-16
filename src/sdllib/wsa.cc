#include "sdllib/wsa.h"

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

#include "absl/strings/str_format.h"
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

//
// WSA animation header allocation type.
// If we need more then 8 flags for the flags variable, we can combine
// USER_ALLOCATED with SYS_ALLOCATED  and combine FILE with RESIDENT.
//
#define WSA_USER_ALLOCATED 0x01U
#define WSA_SYS_ALLOCATED 0x02U
#define WSA_FILE 0x04U
#define WSA_RESIDENT 0x08U
#define WSA_TARGET_IN_BUFFER 0x10U
#define WSA_LINEAR_ONLY 0x20U
#define WSA_FRAME_0_ON_PAGE 0x40U
#define WSA_PALETTE_PRESENT 0x100U
#define WSA_FRAME_0_IS_DELTA 0x200U

// These are used to call Apply_XOR_Delta_To_Page_Or_Viewport() to setup flags
// parameter.  If These change, make sure and change their values in lp_asm.asm.
#define DO_XOR 0x0
#define DO_COPY 0x01

typedef struct {
  uint16_t current_frame;
  uint16_t total_frames;
  uint16_t pixel_x;
  uint16_t pixel_y;
  uint16_t pixel_width;
  uint16_t pixel_height;
  uint16_t largest_frame_size;
  char* delta_buffer;
  char* file_buffer;
  char file_name[13];
  uint16_t flags;
  // New fields that animate does not know about below this point. SEE
  // kExtraBytesAnimateDoesNotKnowAbout
  int16_t file_handle;
  uint32_t anim_mem_size;
} SysAnimHeaderType;

// NOTE:"THIS IS A BAD THING. SINCE sizeof(SysAnimHeaderType) CHANGED, THE
// ANIMATE.EXE UTILITY DID NOT KNOW I UPDATED IT, IT ADDS IT TO
// largest_frame_size BEFORE SAVING IT TO THE FILE.  THIS MEANS I HAVE TO ADD
// THESE charS ON NOW FOR IT TO WORK.
constexpr int kExtraBytesAnimateDoesNotKnowAbout =
    int{sizeof(SysAnimHeaderType) - 37};

//
// Header structure for the file.
// NOTE:  The 'total_frames' field is used to differentiate between Amiga and
// IBM animations.  Amiga animations have the HIGH bit set.
//

#pragma pack(push, 1)
typedef struct {
  uint16_t total_frames;
  uint16_t pixel_x;
  uint16_t pixel_y;
  uint16_t pixel_width;
  uint16_t pixel_height;
  uint16_t largest_frame_size;
  uint16_t flags;
  uint32_t frame0_offset;
  uint32_t frame0_end;
  /* unsigned long data_seek_offset, unsigned short frame_size ... */
} WSA_FileHeaderType;

#pragma pack(pop)

constexpr int kWsaFileHeaderSize{sizeof(WSA_FileHeaderType) -
                                 (2 * sizeof(uint32_t))};

static int64_t Get_Resident_Frame_Offset(const char* file_buffer, int frame);
static int64_t Get_File_Frame_Offset(int file_handle, int frame,
                                     int palette_adjust);
static bool Apply_Delta(const SysAnimHeaderType* sys_header, int curr_frame,
                        void* dest_ptr, int dest_w);

void* Open_Animation(const char* file_name, char* user_buffer,
                     int32_t user_buffer_size, WSAOpenType user_flags,
                     unsigned char* palette) {
  int palette_adjust = 0;
  int frame0_size = 0;
  base::ssize target_buffer_size = 0;
  WSA_FileHeaderType file_header = {};

  /*======================================================================*/
  /* Open the file to get the header information
   */
  /*======================================================================*/

  uint16_t anim_flags = 0;
  const int fh = OpenFileHandle(file_name, FileAccess::kRead);
  if (fh == kInvalidHandle) {
    return nullptr;
  }
  ReadFileHandle(fh, &file_header, sizeof(WSA_FileHeaderType));

  /*======================================================================*/
  /* If the file has an attached palette then if we have a valid palette
   */
  /*		pointer we need to read it in.
   */
  /*======================================================================*/

  if (file_header.flags & 1) {
    anim_flags |= WSA_PALETTE_PRESENT;
    palette_adjust = 768;

    if (palette != nullptr) {
      SeekFileHandle(
          fh, static_cast<int32_t>(sizeof(uint32_t) * file_header.total_frames),
          SEEK_CUR);
      ReadFileHandle(fh, palette, 768L);
    }

  } else {
    palette_adjust = 0;
  }

  // Check for flag from ANIMATE indicating that this animation was
  // created from a .LBM and a .ANM.  These means that the first
  // frame is a XOR Delta from a picture, not black.
  if (file_header.flags & 2) {
    anim_flags |= WSA_FRAME_0_IS_DELTA;
  }

  // Get the total file size minus the size of the first frame and the size
  // of the file header.  These will not be read in to save even more space.
  base::ssize file_buffer_size = SeekFileHandle(fh, 0, SEEK_END);

  if (file_header.frame0_offset) {
    const auto tlong = static_cast<int32_t>(file_header.frame0_end -
                                            file_header.frame0_offset);
    frame0_size = static_cast<uint16_t>(tlong);
  } else {
    anim_flags |= WSA_FRAME_0_ON_PAGE;
    frame0_size = 0;
  }

  file_buffer_size -= palette_adjust + frame0_size + kWsaFileHeaderSize;

  // We need to determine the buffer sizes required for the animation.  At a
  // minimum, we need a target buffer for the uncompressed frame and a delta
  // buffer for the delta data.  We may be able to make the file resident,
  // so we will determine the file size.
  //
  // If the target buffer is in the user buffer
  // Then figure its size
  // and set the allocation flag
  // Else size is zero.
  //
  if (user_flags & WSA_OPEN_DIRECT) {
    target_buffer_size = 0L;
  } else {
    anim_flags |= WSA_TARGET_IN_BUFFER;
    target_buffer_size =
        base::ssize{file_header.pixel_width} * file_header.pixel_height;
  }

  // NOTE:"THIS IS A BAD THING. SINCE sizeof(SysAnimHeaderType) CHANGED, THE
  // ANIMATE.EXE UTILITY DID NOT KNOW I UPDATED IT, IT ADDS IT TO
  // largest_frame_size BEFORE SAVING IT TO THE FILE.  THIS MEANS I HAVE TO ADD
  // THESE charS ON NOW FOR IT TO WORK.
  const base::ssize delta_buffer_size =
      base::ssize{file_header.largest_frame_size} +
      kExtraBytesAnimateDoesNotKnowAbout;

  // Frame 0 is read into the last largest_frame_size - 37 bytes of the delta
  // buffer. A corrupt header whose frame 0 is bigger than that, or whose
  // largest_frame_size is too small to include ANIMATE's 37 header bytes,
  // would write outside it.
  const base::ssize frame_capacity =
      delta_buffer_size - base::ssize{sizeof(SysAnimHeaderType)};
  if (frame_capacity < 0 || frame0_size > frame_capacity) {
    CloseFileHandle(fh);
    return nullptr;
  }
  const base::ssize min_buffer_size = target_buffer_size + delta_buffer_size;
  const base::ssize max_buffer_size = min_buffer_size + file_buffer_size;

  // check to see if buffer size is big enough for at least min required
  if (user_buffer &&
      std::bit_cast<uintptr_t>(user_buffer) % alignof(SysAnimHeaderType) != 0) {
    CloseFileHandle(fh);
    return nullptr;
  }

  if (user_buffer && user_buffer_size < min_buffer_size) {
    CloseFileHandle(fh);
    return nullptr;
  }

  // A buffer was not passed in, so do allocations
  if (user_buffer == nullptr) {
    // If the user wants it from the disk, or specified a buffer less than
    // the max needed, give them the min. Otherwise (no buffer size, or
    // enough for the max configuration) allocate what we need.
    if ((user_flags & WSA_OPEN_FROM_DISK) ||
        (user_buffer_size != 0 && user_buffer_size < max_buffer_size)) {
      user_buffer_size = static_cast<int32_t>(min_buffer_size);
    } else {
      user_buffer_size = static_cast<int32_t>(max_buffer_size);
    }

    // Check to see if enough RAM available for buffer_size.
    if (user_buffer_size > Ram_Free(MEM_NORMAL)) {
      // If not enough room for even the min, return no buffer.

      if (min_buffer_size > Ram_Free(MEM_NORMAL)) {
        CloseFileHandle(fh);
        return nullptr;
      }

      // Else make buffer size the min and allocate it.
      user_buffer_size = static_cast<int32_t>(min_buffer_size);
    }

    // allocate buffer needed
    user_buffer = new char[base::ToSize(user_buffer_size)]();

    anim_flags |= WSA_SYS_ALLOCATED;
  } else {
    // Check to see if the user_buffer_size should be min or max.
    if (user_flags & WSA_OPEN_FROM_DISK || user_buffer_size < max_buffer_size) {
      user_buffer_size = static_cast<int32_t>(min_buffer_size);
    } else {
      user_buffer_size = static_cast<int32_t>(max_buffer_size);
    }
    anim_flags |= WSA_USER_ALLOCATED;
  }

  // Set the pointers to the RAM buffers
  char* sys_anim_header_buffer = user_buffer;
  char* target_buffer = static_cast<char*>(
      Add_Long_To_Pointer(sys_anim_header_buffer, sizeof(SysAnimHeaderType)));
  char* delta_buffer = static_cast<char*>(
      Add_Long_To_Pointer(target_buffer, target_buffer_size));

  //	Clear target buffer if it is in the user buffer.
  if (target_buffer_size) {
    // The 16-bit DOS build clamped this to unsigned short, which left most of
    // any frame over 65535 pixels (e.g. 320x240) uncleared.
    memset(target_buffer, 0, base::ToSize(target_buffer_size));
  }

  // Poke data into the system animation header (start of user_buffer)
  // current_frame is set to total_frames so that Animate_Frame() knows that
  // it needs to clear the target buffer.

  // Allocated storage is aligned; caller-provided storage was checked above.
  auto* sys_header =
      port::AlignedObject<SysAnimHeaderType>(sys_anim_header_buffer);
  sys_header->current_frame = sys_header->total_frames =
      file_header.total_frames;
  sys_header->pixel_x = file_header.pixel_x;
  sys_header->pixel_y = file_header.pixel_y;
  sys_header->pixel_width = file_header.pixel_width;
  sys_header->pixel_height = file_header.pixel_height;
  sys_header->anim_mem_size = static_cast<std::uint32_t>(user_buffer_size);
  sys_header->delta_buffer = delta_buffer;
  sys_header->largest_frame_size = static_cast<uint16_t>(
      delta_buffer_size - base::ssize{sizeof(SysAnimHeaderType)});

  absl::SNPrintF(sys_header->file_name, sizeof(sys_header->file_name), "%s",
                 file_name);

  // Figure how much room the frame offsets take up in the file.
  // Add 2 - one for the wrap around and one for the final end offset.
  const int offsets_size = (file_header.total_frames + 2) * 4;

  // Can the user_buffer_size handle the maximum case buffer?
  if (user_buffer_size == max_buffer_size) {
    //
    //	set the file buffer pointer,
    // Skip over the header information.
    // Read in the offsets.
    // Skip over the first frame.
    // Read in remaining frames.
    //

    sys_header->file_buffer = static_cast<char*>(
        Add_Long_To_Pointer(delta_buffer, sys_header->largest_frame_size));
    SeekFileHandle(fh, kWsaFileHeaderSize, SEEK_SET);
    ReadFileHandle(fh, sys_header->file_buffer, offsets_size);
    SeekFileHandle(fh, frame0_size + palette_adjust, SEEK_CUR);
    ReadFileHandle(fh, sys_header->file_buffer + offsets_size,
                   static_cast<int32_t>(file_buffer_size - offsets_size));

    //
    // Find out if there is an ending value for the last frame.
    // If there is not, then this animation will not be able to
    // loop back to the beginning.
    //
    if (Get_Resident_Frame_Offset(sys_header->file_buffer,
                                  sys_header->total_frames + 1)) {
      anim_flags |= WSA_RESIDENT;
    } else {
      anim_flags |= WSA_LINEAR_ONLY | WSA_RESIDENT;
    }
  } else {  // buffer cannot handle max_size of buffer

    if (Get_File_Frame_Offset(fh, sys_header->total_frames + 1,
                              palette_adjust)) {
      anim_flags |= WSA_FILE;
    } else {
      anim_flags |= WSA_LINEAR_ONLY | WSA_FILE;
    }
    ////
    sys_header->file_buffer = nullptr;
  }

  // Figure where to back load frame 0 into the delta buffer.
  char* delta_back = static_cast<char*>(Add_Long_To_Pointer(
      delta_buffer, sys_header->largest_frame_size - frame0_size));

  // Read the first frame into the delta buffer and uncompress it.
  // Then close it.
  SeekFileHandle(fh, kWsaFileHeaderSize + offsets_size + palette_adjust,
                 SEEK_SET);
  ReadFileHandle(fh, delta_back, frame0_size);

  // We do not use the file handle when it is in RAM.
  if (anim_flags & WSA_RESIDENT) {
    sys_header->file_handle = static_cast<int16_t>(-1);
    CloseFileHandle(fh);
  } else {
    sys_header->file_handle = static_cast<int16_t>(fh);
  }

  LCW_Uncompress(delta_back, delta_buffer, sys_header->largest_frame_size);

  // Finally set the flags,
  sys_header->flags = anim_flags;

  // return valid handle
  return user_buffer;
}

void Close_Animation(void* handle) {

  // Assign our local system header pointer to the beginning of the handle space
  auto* sys_header = static_cast<SysAnimHeaderType*>(handle);

  // Close the WSA file in it was disk based.
  if (sys_header->flags & WSA_FILE) {
    CloseFileHandle(sys_header->file_handle);
  }

  // Check to see if the buffer was allocated OR the programmer provided the
  // buffer
  if (handle && sys_header->flags & WSA_SYS_ALLOCATED) {
    Free(handle);
  }
}

bool Animate_Frame(void* handle, GraphicViewPortClass& view, int frame_number,
                   int x_pixel, int y_pixel, WSAType /*flags_and_prio*/,
                   void* /*magic_cols*/, void* /*magic*/) {
  int search_frames = 0;            // How many frames to search.
  uint8_t* frame_buffer = nullptr;  // our destination.
  bool direct_to_dest = false;      // are we going directly to the destination?

  // Assign local pointer to the beginning of the buffer where the system
  // information resides
  auto* sys_header = static_cast<SysAnimHeaderType*>(
      handle);  // fix up the void pointer past in.

  // Get the total number of frames
  const int total_frames =
      sys_header->total_frames;  // number of frames in anim.

  // Are the animation handle and the frame number valid?
  if (!handle || total_frames <= frame_number) {
    return false;
  }

  if (!view.Lock()) {
    return false;
  }

  // Decide if we are going to a page or a viewport (part of a buffer).
  const int dest_width =
      view.Get_Width() + view.Get_XAdd() +
      view.Get_Pitch();  // the width of the destination buffer or page.

  //
  // adjust x_pixel and y_pixel by system pixel_x and pixel_y respectively.
  //
  x_pixel += static_cast<int16_t>(sys_header->pixel_x);
  y_pixel += static_cast<int16_t>(sys_header->pixel_y);

  //
  // Check to see if we are using a buffer inside of the animation buffer or if
  // it is being drawn directly to the destination page or buffer.
  //
  if (sys_header->flags & WSA_TARGET_IN_BUFFER) {
    // Get a pointer to the frame in animation buffer.
    frame_buffer = static_cast<uint8_t*>(
        Add_Long_To_Pointer(sys_header, sizeof(SysAnimHeaderType)));
    direct_to_dest = false;
  } else {
    frame_buffer = view.Get_Offset();
    frame_buffer += (y_pixel * dest_width) + x_pixel;
    direct_to_dest = true;
  }
  //
  // If current_frame is equal to tatal_frames, then no animations have taken
  // place so must uncompress frame 0 in delta buffer to the frame_buffer/page
  // if it exists.
  //
  if (std::cmp_equal(sys_header->current_frame, total_frames)) {
    // Call apply delta telling it wether to copy or to xor depending on if the
    // target is a page or a buffer.

    if (!(sys_header->flags & WSA_FRAME_0_ON_PAGE)) {
      if (direct_to_dest) {
        // The last parameter says weather to copy or to XOR.  If  the
        // first frame is a DELTA, then it must be XOR'd.  A true is
        // copy while false is XOR.

        Apply_XOR_Delta_To_Page_Or_Viewport(
            frame_buffer, sys_header->delta_buffer, sys_header->pixel_width,
            dest_width,  // dest_width - sys_header->pixel_width,
            sys_header->flags & WSA_FRAME_0_IS_DELTA ? DO_XOR : DO_COPY);
      } else {
        Apply_XOR_Delta(frame_buffer, sys_header->delta_buffer);
      }
    }
    sys_header->current_frame = 0;
  }

  //
  // Get the current frame
  // If no looping aloud, are the trying to do it anyways?
  //
  int curr_frame = sys_header->current_frame;  // current frame we are on.

  // Get absoulte distance from our current frame to the target frame
  const int distance =
      std::abs(curr_frame - frame_number);  // distance to desired frame.

  // Assume we are searching right
  int search_dir = 1;  // direcion to search for desired frame.

  // Calculate the number of frames to search if we go right and wrap

  if (frame_number > curr_frame) {
    search_frames = total_frames - frame_number + curr_frame;

    // Is going right faster than going backwards?
    // Or are they trying to loop when the should not?
    if (search_frames < distance && !(sys_header->flags & WSA_LINEAR_ONLY)) {
      search_dir = -1;  // No, so go left
    } else {
      search_frames = distance;
    }
  } else {
    search_frames = total_frames - curr_frame + frame_number;

    // Is going right faster than going backwards?
    // Or are they trying to loop when the should not?
    if (search_frames >= distance || sys_header->flags & WSA_LINEAR_ONLY) {
      search_dir = -1;  // No, so go left
      search_frames = distance;
    }
  }

  // Take care of the case when we are searching right (possibly right)

  if (search_dir > 0) {
    for (int loop = 0; loop < search_frames; loop++) {
      // Move the logical frame number ordinally right
      curr_frame += search_dir;

      Apply_Delta(sys_header, curr_frame, frame_buffer, dest_width);

      // Adjust the current frame number, taking into consideration that we
      // could have wrapped

      if (curr_frame == total_frames) {
        curr_frame = 0;
      }
    }
  } else {
    for (int loop = 0; loop < search_frames; loop++) {
      // If we are going backwards and we are on frame 0, the delta to get
      // to the last frame is the n + 1 delta (wrap delta)

      if (curr_frame == 0) {
        curr_frame = total_frames;
      }

      Apply_Delta(sys_header, curr_frame, frame_buffer, dest_width);

      curr_frame += search_dir;
    }
  }

  sys_header->current_frame = static_cast<uint16_t>(frame_number);

  // If we did this all in a hidden buffer, then copy it to the desired page or
  // viewport.
  if (sys_header->flags & WSA_TARGET_IN_BUFFER) {
    Buffer_To_Page(x_pixel, y_pixel, sys_header->pixel_width,
                   sys_header->pixel_height, frame_buffer, view);
  }

  view.Unlock();
  return true;
}

int Get_Animation_Frame_Count(void* handle) {

  if (!handle) {
    return 0;
  }
  auto* sys_header = static_cast<SysAnimHeaderType*>(handle);
  return static_cast<int16_t>(sys_header->total_frames);
}

unsigned int Apply_XOR_Delta(void* target, const void* delta) {
  auto* source_ptr = static_cast<uint8_t*>(target);
  const auto* udelta = static_cast<const uint8_t*>(delta);

  // top_loop
  while (true) {
    uint8_t b = *udelta++;

    if (b == 0) {
      // SHORTRUN
      int count = *udelta++;      // get count
      const uint8_t xor_b = *udelta++;  // get XOR byte
      do {
        *source_ptr =
            static_cast<uint8_t>(*source_ptr ^ xor_b);  // XOR that byte
        source_ptr++;
      } while (--count);
    } else if (b & 0x80) {
      // By now, we know it must be a LONGDUMP, SHORTSKIP, LONGRUN, or LONGSKIP
      b &= 0x7F;
      if (b == 0) {
        // get word code
        const uint32_t code = udelta[0] | uint32_t{udelta[1]} << 8;
        udelta += 2;

        if (!code) {
          return 0;  // long count of zero means stop
        }

        if (code & 0x8000) {
          if (code & 0x4000) {
            // LONGRUN
            int count = static_cast<int>(code & 0x3FFF);
            const uint8_t xor_b = *udelta++;  // get XOR byte
            do {
              *source_ptr =
                  static_cast<uint8_t>(*source_ptr ^ xor_b);  // XOR that byte
              source_ptr++;
            } while (--count);
          } else {
            // LONGDUMP
            int count = static_cast<int>(code & 0x7FFF);
            do {
              const uint8_t xor_b = *udelta++;  // get delta XOR byte
              *source_ptr = static_cast<uint8_t>(
                  *source_ptr ^ xor_b);  // xor that byte on the dest
              source_ptr++;
            } while (--count);
          }
        } else {  // LONGSKIP
          source_ptr += code;
        }
      } else {  // SHORTSKIP
        source_ptr += b;
      }
    } else {
      // SHORTDUMP
      int count = b;

      do {
        const uint8_t xor_b = *udelta++;  // get delta XOR byte
        *source_ptr = static_cast<uint8_t>(*source_ptr ^
                                           xor_b);  // xor that byte on the dest
        source_ptr++;
      } while (--count);
    }
  }
}

void Apply_XOR_Delta_To_Page_Or_Viewport(void* target, void* delta, int width,
                                         int nextrow, int copy) {
  auto* source_ptr = static_cast<uint8_t*>(target);
  const auto* udelta = static_cast<uint8_t*>(delta);

  int x = 0;

  if (copy == DO_XOR) {
    // mostly duplicated from Apply_XOR_Delta
    // except for the row checks
    while (true) {
      uint8_t b = *udelta++;

      if (b == 0) {
        // SHORTRUN
        int count = *udelta++;      // get count
        const uint8_t xor_b = *udelta++;  // get XOR byte
        do {
          *source_ptr ^= xor_b;  // XOR that byte
          source_ptr++;

          if (++x == width) {
            // final column, go to next row
            x = 0;
            source_ptr = source_ptr - width + nextrow;
          }
        } while (--count);
      } else if (b & 0x80) {
        // By now, we know it must be a LONGDUMP, SHORTSKIP, LONGRUN, or
        // LONGSKIP
        b &= 0x7F;
        if (b == 0) {
          // get word code
          const uint32_t code = udelta[0] | uint32_t{udelta[1]} << 8;
          udelta += 2;

          if (!code) {
            return;  // long count of zero means stop
          }

          if (code & 0x8000) {
            if (code & 0x4000) {
              // LONGRUN
              int count = static_cast<int>(code & 0x3FFF);
              const uint8_t xor_b = *udelta++;  // get XOR byte
              do {
                *source_ptr ^= xor_b;  // XOR that byte
                source_ptr++;

                if (++x == width) {
                  // final column, go to next row
                  x = 0;
                  source_ptr = source_ptr - width + nextrow;
                }
              } while (--count);
            } else {
              // LONGDUMP
              int count = static_cast<int>(code & 0x7FFF);
              do {
                const uint8_t xor_b = *udelta++;  // get delta XOR byte
                *source_ptr ^= xor_b;       // xor that byte on the dest
                source_ptr++;

                if (++x == width) {
                  // final column, go to next row
                  x = 0;
                  source_ptr = source_ptr - width + nextrow;
                }
              } while (--count);
            }
          } else {
            // LONGSKIP
            source_ptr -= x;  // go back to beginning or row.
            x += static_cast<int>(code);
            while (x >= width) {
              x -= width;
              source_ptr += nextrow;
            }
            source_ptr += x;  // get to correct position in row.
          }
        } else  // SHORTSKIP
        {
          source_ptr -= x;  // go back to beginning or row.
          x += b;
          while (x >= width) {
            x -= width;
            source_ptr += nextrow;
          }
          source_ptr += x;  // get to correct position in row.
        }
      } else {
        // SHORTDUMP
        int count = b;

        do {
          const uint8_t xor_b = *udelta++;  // get delta XOR byte
          *source_ptr ^= xor_b;       // xor that byte on the dest
          source_ptr++;
          if (++x == width) {
            // final column, go to next row
            x = 0;
            source_ptr = source_ptr - width + nextrow;
          }
        } while (--count);
      }
    }
  } else  // copy
  {
    // same thing without xor...
    while (true) {
      uint8_t b = *udelta++;

      if (b == 0) {
        // SHORTRUN
        int count = *udelta++;      // get count
        const uint8_t xor_b = *udelta++;  // get byte
        do {
          *source_ptr = xor_b;  // store that byte
          source_ptr++;

          if (++x == width) {
            // final column, go to next row
            x = 0;
            source_ptr = source_ptr - width + nextrow;
          }
        } while (--count);
      } else if (b & 0x80) {
        // By now, we know it must be a LONGDUMP, SHORTSKIP, LONGRUN, or
        // LONGSKIP
        b &= 0x7F;
        if (b == 0) {
          // get word code
          const uint32_t code = udelta[0] | uint32_t{udelta[1]} << 8;
          udelta += 2;

          if (!code) {
            return;  // long count of zero means stop
          }

          if (code & 0x8000) {
            if (code & 0x4000) {
              // LONGRUN
              int count = static_cast<int>(code & 0x3FFF);
              const uint8_t xor_b = *udelta++;  // get byte
              do {
                *source_ptr = xor_b;  // store that byte
                source_ptr++;

                if (++x == width) {
                  // final column, go to next row
                  x = 0;
                  source_ptr = source_ptr - width + nextrow;
                }
              } while (--count);
            } else {
              // LONGDUMP
              int count = static_cast<int>(code & 0x7FFF);
              do {
                const uint8_t xor_b = *udelta++;  // get delta byte
                *source_ptr = xor_b;        // store that byte
                source_ptr++;

                if (++x == width) {
                  // final column, go to next row
                  x = 0;
                  source_ptr = source_ptr - width + nextrow;
                }
              } while (--count);
            }
          } else {
            // LONGSKIP
            source_ptr -= x;  // go back to beginning or row.
            x += static_cast<int>(code);
            while (x >= width) {
              x -= width;
              source_ptr += nextrow;
            }
            source_ptr += x;  // get to correct position in row.
          }
        } else  // SHORTSKIP
        {
          source_ptr -= x;  // go back to beginning or row.
          x += b;
          while (x >= width) {
            x -= width;
            source_ptr += nextrow;
          }
          source_ptr += x;  // get to correct position in row.
        }
      } else {
        // SHORTDUMP
        int count = b;

        do {
          const uint8_t xor_b = *udelta++;  // get delta byte
          *source_ptr = xor_b;        // store that byte
          source_ptr++;
          if (++x == width) {
            // final column, go to next row
            x = 0;
            source_ptr = source_ptr - width + nextrow;
          }
        } while (--count);
      }
    }
  }
}

static int64_t Get_Resident_Frame_Offset(const char* file_buffer, int frame) {
  uint32_t frame0_size = 0;
  const auto first = port::ReadUnaligned<uint32_t>(file_buffer);
  if (first) {
    frame0_size =
        port::ReadUnaligned<uint32_t>(file_buffer + sizeof(uint32_t)) - first;
  } else {
    frame0_size = 0;
  }

  const auto offset = port::ReadUnaligned<uint32_t>(
      file_buffer + (base::ToSize(frame) * sizeof(uint32_t)));
  if (offset) {
    return offset - (frame0_size + kWsaFileHeaderSize);
  }
  return 0L;
}

static int64_t Get_File_Frame_Offset(int file_handle, int frame,
                                     int palette_adjust) {
  uint32_t offset = 0;

  SeekFileHandle(file_handle, (frame * 4) + kWsaFileHeaderSize, SEEK_SET);

  if (ReadFileHandle(file_handle, &offset, sizeof(uint32_t)) !=
      sizeof(uint32_t)) {
    offset = 0L;
  }
  offset += static_cast<uint32_t>(palette_adjust);
  return offset;
}

static bool Apply_Delta(const SysAnimHeaderType* sys_header, int curr_frame,
                        void* dest_ptr, int dest_w) {
  int64_t frame_data_size = 0;
  int64_t frame_offset = 0;

  const int palette_adjust = sys_header->flags & WSA_PALETTE_PRESENT ? 768 : 0;
  char* delta_back = sys_header->delta_buffer;

  if (sys_header->flags & WSA_RESIDENT) {
    // Get offset of the given frame in the resident file
    // Get the size of the frame <- (frame+1 offset) - (offset)
    // Point at the delta data
    // figure offset to load data into end of delta buffer
    // copy it into buffer

    frame_offset =
        Get_Resident_Frame_Offset(sys_header->file_buffer, curr_frame);
    frame_data_size =
        Get_Resident_Frame_Offset(sys_header->file_buffer, curr_frame + 1) -
        frame_offset;

    // A corrupt offset table must not copy from outside the loaded file data
    // or past the delta buffer, which holds largest_frame_size bytes.
    const auto* const anim_end = static_cast<const char*>(
        Add_Long_To_Pointer(sys_header, sys_header->anim_mem_size));
    if (frame_offset < 0 || frame_data_size <= 0 ||
        std::cmp_greater(frame_data_size, sys_header->largest_frame_size) ||
        frame_offset + frame_data_size > anim_end - sys_header->file_buffer) {
      return false;
    }

    char* data_ptr = static_cast<char*>(
        Add_Long_To_Pointer(sys_header->file_buffer, frame_offset));
    delta_back = static_cast<char*>(Add_Long_To_Pointer(
        delta_back, sys_header->largest_frame_size - frame_data_size));

    Mem_Copy(data_ptr, delta_back, base::ToSize(frame_data_size));

  } else if (sys_header->flags & WSA_FILE) {
    //	Open up file because not file not in RAM.
    // Get offset of the given frame in the file on disk
    // Get the size of the frame <- (frame+1 offset) - (offset)
    // Return if Get_.._offset() failed.  -- need error handling????
    //	Seek to delta data.
    // figure offset to load data into end of delta buffer
    //	Read it into buffer -- Return if correct amount not read.-- errors??

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
    delta_back = static_cast<char*>(Add_Long_To_Pointer(
        delta_back, sys_header->largest_frame_size - frame_data_size));

    if (ReadFileHandle(file_handle, delta_back,
                       static_cast<int32_t>(frame_data_size)) !=
        static_cast<int>(frame_data_size)) {
      return false;
    }
  }

  // Uncompress data at end of delta buffer to the beginning of delta buffer.
  // Find start of target buffer.
  // Apply the XOR delta.

  LCW_Uncompress(delta_back, sys_header->delta_buffer,
                 sys_header->largest_frame_size);

  if (sys_header->flags & WSA_TARGET_IN_BUFFER) {
    Apply_XOR_Delta(dest_ptr, sys_header->delta_buffer);
  } else {
    Apply_XOR_Delta_To_Page_Or_Viewport(dest_ptr, sys_header->delta_buffer,
                                        sys_header->pixel_width, dest_w,
                                        DO_XOR);
  }

  return true;
}
