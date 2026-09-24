/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CNC_RED_ALERT_WINVQ_VQA32_VQAPLAYP_H_
#define CNC_RED_ALERT_WINVQ_VQA32_VQAPLAYP_H_

// File: the VQA player's internal state, VqaPlayerState, and the functions the
// player's source files share. Only vqa32 and its tests include it.
//
// The player is three cooperating parts that take turns in PlayVqa()'s loop
// on one thread. The loader reads frames from the file into a ring of frame
// buffers and their codebooks into a ring of codebook buffers; the drawer
// decodes a loaded frame into the image buffer when the clock says it is due;
// the "flipper" (ReleaseDrawnFrame) then hands the frame's buffer back to the
// loader. Each part keeps its own position in the rings, so a frame buffer's
// flags are how they tell each other it is full or free. The sound runs
// separately, on the SDL audio thread, from a ring the loader fills.
//
// Originally written by Denzil E. Long, Jr. and Bill Randolph at Westwood
// Studios, August 1995.

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqaio.h"
#include "winvq/vqa32/vqaplay.h"
#include "winvq/vqm32/soscomp.h"

// Packs a block width and height into the dimension code the drawer switches
// on to pick a decoder. Only 4x2 and 4x4 blocks have one.
constexpr uint32_t BlockDimensions(uint32_t a, uint32_t b) {
  return ((a & 0xFF) << 8) | (b & 0xFF);
}
constexpr uint32_t kBlock4x2 = BlockDimensions(4, 2);
constexpr uint32_t kBlock4x4 = BlockDimensions(4, 4);

// ChunkHeader: the 8 bytes in front of every IFF chunk, read raw. id compares
// against the ID_ and kChunk constants as read; size is big-endian and only
// usable through ChunkSize() in loader.cc. The payload that follows is padded
// to an even length.
struct ChunkHeader {
  uint32_t id;
  uint32_t size;
};

// ZapHeader: the header of a SND1 (Westwood ADPCM) sound chunk. Equal sizes
// mean the sound is stored uncompressed.
struct ZapHeader {
  uint16_t UnCompSize;  // Bytes of sound after decompression
  uint16_t CompSize;    // Bytes of sound in the chunk
};

// VqaCodebook: one buffer in the ring of codebooks. A codebook is the table of
// pixel blocks a frame's vector pointers index into, and serves every frame
// of a group. The loader assembles the next group's codebook from the
// partial codebooks in the current group's frames, so the ring needs at
// least that one codebook ahead of the one in use.
//
// Compressed data is loaded at the end of the buffer (compressed_offset) and
// decompressed in place towards the start by the drawer when the first frame
// using it is drawn; uncompressed data is loaded at the start.
struct VqaCodebook {
  // codebook_capacity bytes.
  std::vector<unsigned char> buffer;
  VqaCodebook* next = nullptr;
  uint32_t flags = 0;  // kCodebook* bits
  // Where the compressed data starts in buffer.
  int32_t compressed_offset = 0;
};

// VqaCodebook flag, set by the loader for each new codebook: the codebook is
// still compressed.
constexpr uint32_t kCodebookCompressed = 1U << 1;

// VqaFrame: one buffer in the ring of loaded frames. The loader fills it
// and sets kFrameLoaded; the drawer decodes it; the flipper or a skip
// clears flags to hand it back to the loader. Compressed data sits at the end
// of its buffer and is decompressed in place just before the frame is drawn,
// as in VqaCodebook.
struct VqaFrame {
  // The frame's vector pointers, one per block, pointers_capacity bytes.
  std::vector<unsigned char> pointers;
  // The frame's palette as 8-bit R,G,B triplets, palette_capacity bytes. Holds
  // a stale palette when the frame has none (kFrameHasPalette clear).
  std::vector<unsigned char> palette;
  // The codebook the frame's pointers index into.
  VqaCodebook* codebook = nullptr;
  VqaFrame* next = nullptr;
  uint32_t flags = 0;  // kFrame* bits
  // Number of the frame in the movie.
  int32_t frame_number = 0;
  // Where the compressed pointers and palette start in their buffers.
  int32_t pointers_offset = 0;
  int32_t palette_offset = 0;
  // Bytes of palette, meaningful when kFrameHasPalette is set.
  int32_t palette_bytes = 0;
};

// VqaFrame flags. All clear means the buffer is free for the loader.
// Loaded and waiting to be drawn.
constexpr uint32_t kFrameLoaded = 1U << 0;
// Key frame: never skipped.
constexpr uint32_t kFrameKey = 1U << 1;
// Carries a palette that must be set.
constexpr uint32_t kFrameHasPalette = 1U << 2;
// The palette and the vector pointers are still compressed.
constexpr uint32_t kFramePaletteCompressed = 1U << 3;
constexpr uint32_t kFramePointersCompressed = 1U << 4;

// VqaLoader: the loader's position in the file and in the buffer rings.
struct VqaLoader {
  // The codebook node the partial codebooks of the current group are
  // collected into, to become the next group's codebook.
  VqaCodebook* partial_codebook;
  // The last complete codebook, used by the frames being loaded.
  VqaCodebook* full_codebook;
  // The frame node the next frame is loaded into.
  VqaFrame* current_frame;
  // Partial codebooks collected into partial_codebook so far, and their total
  // size in bytes (compressed or not).
  int32_t partial_count;
  int32_t partial_bytes;
  // Number of the next frame to load; the movie is loaded when it reaches
  // the header's frame count.
  int32_t next_frame_number;
  // Header of the chunk being loaded, kept so a loader woken from
  // kMovieLoaderAsleep resumes inside it instead of reading a new one.
  ChunkHeader chunk_header;
};

// VqaDrawer: where and when the drawer decodes frames.
struct VqaDrawer {
  // The next frame to draw.
  VqaFrame* current_frame;
  uint32_t flags;  // kDrawer* bits
  // The buffer frames are decoded into, image_width x image_height pixels:
  // the caller's, the player's own, or empty when neither was provided.
  std::span<unsigned char> image_buffer;
  int32_t image_width;
  int32_t image_height;
  // Inclusive image corners in buffer pixels. x1,y1 is the corner anchored
  // by the kVqaDrawOriginMask flags, x2,y2 the opposite one.
  int32_t x1, y1, x2, y2;
  // Index in image_buffer of the image's top-left pixel.
  int32_t image_offset;
  // Size in bytes of saved_palette's contents; 0 until the loader has seen the
  // movie's first palette.
  int32_t saved_palette_bytes;
  // The movie's first palette, copied there by the loader, and later the
  // palette of a frame skipped with kDrawerPalettePending. At most 256 colors,
  // which is why the loader rejects larger palettes.
  std::array<unsigned char, 768> saved_palette;
  // The image size in blocks, the geometry decode_frame walks.
  int32_t blocks_per_row;
  int32_t block_rows;
  // When the frame-skipping path last passed a frame; starts one second in
  // the past so the first frame is never early.
  int64_t last_time;  // In kVqaTicksPerSecond, as returned by ReadMovieClock().
  // Number of the last frame selected for drawing. Select_Frame() draws
  // regardless of the clock once frame_rate / 5 frames have passed since, so
  // at least 5 frames a second reach the screen.
  int32_t last_selected_frame;
  // Number of the last frame drawn, which PlayVqa() returns in walk mode.
  int32_t last_drawn_frame;
};

// Drawer flag: a skipped frame's palette is pending.
constexpr uint32_t kDrawerPalettePending = 1U << 0;

// VqaFlipper: the frame the drawer finished, which ReleaseDrawnFrame()
// releases. The name is from DOS, where this step showed the frame by flipping
// video pages; now the client's frame_callback has already shown it.
struct VqaFlipper {
  // The frame drawn last; valid while kMovieAwaitingRelease is set.
  VqaFrame* drawn_frame;
};

// VqaAudio: the sound ring and the state shared with the SDL audio callback.
//
// The ring is block_count blocks of config.audio_block_bytes bytes. The
// loader decompresses each frame's sound chunk into TempBuf, and
// CopyStagedAudio() moves it into the ring at write_offset, marking the blocks
// it filled in block_loaded. The callback, on the audio thread, plays
// play_block from play_offset and frees it once the next block is loaded; if it
// is not, it plays the block again. The loader sleeps (kVqaSleeping) while the
// block it would overwrite is still unplayed. Code on the main thread holds the
// SDL device lock while changing what the callback reads.
struct VqaAudio {
  // The ring, when the player allocated it.
  std::vector<unsigned char> ring_storage;
  // One flag per ring block: 1 = holds unplayed sound, 0 = free to fill.
  std::vector<int16_t> block_loaded;
  // Staging for one frame's decompressed sound, staging_capacity bytes.
  std::vector<unsigned char> staging;
  std::span<unsigned char> ring;  // ring_storage or caller-owned span
  // Byte offset in the ring where the loader writes next.
  int32_t write_offset = 0;
  int32_t block_count = 0;
  // The block being played, and the one after it (callback scratch).
  int32_t play_block = 0;
  int32_t next_block = 0;
  // Bytes in TempBuf waiting for CopyStagedAudio(), 0 when it is empty.
  int32_t staged_bytes = 0;
  int32_t staging_capacity = 0;
  uint32_t flags = 0;  // kAudio* bits
  // Byte offset of play_block in the ring.
  int32_t play_offset = 0;
  // Format of the track being played, the primary or the alternate one.
  int sample_rate = 0;
  int channels = 0;
  int bits_per_sample = 0;  // 8 or 16
  int32_t bytes_per_second = 0;
  // Decoder state for SND2 (IMA ADPCM) chunks, carried from chunk to chunk.
  SosCompressInfo adpcm = {};
  // Blocks handed to SDL since the sound started (a replayed block counts
  // only after the movie has loaded completely). ReadMovieClock() derives the
  // movie clock from it.
  int blocks_played = 0;
};

// Audio flags.
// The SDL stream and callback are installed.
constexpr uint32_t kAudioOpen = 1U << 0;
// The callback is playing the ring.
constexpr uint32_t kAudioPlaying = 1U << 6;

// VqaMovie: everything a movie needs while it is open. Allocated by OpenVqa()
// once the header is read and freed by CloseVqa().
struct VqaMovie {
  // Decodes a frame into the image buffer: the routine for the movie's block
  // size, or one that does nothing when kVqaDrawToBuffer is clear or the block
  // size has no routine.
  void (*decode_frame)(std::span<const unsigned char> codebook,
                       std::span<const unsigned char> pointers,
                       std::span<unsigned char> buffer, int blocksperrow,
                       int numrows, int bufwidth) = nullptr;

  // RAII storage for nodes - these vectors own the node objects
  std::vector<std::unique_ptr<VqaCodebook>> codebooks;
  std::vector<std::unique_ptr<VqaFrame>> frames;

  // The image buffer, when the player allocated it.
  std::vector<unsigned char> image_storage;
  // One FINF entry per header frame: 4 flag bits on top of the halved file
  // offset (see FrameHasPalette and FrameByteOffset in vqa_format.h).
  std::vector<uint32_t> frame_offsets;

  VqaAudio audio;
  VqaLoader loader{};
  VqaDrawer drawer{};
  VqaFlipper flipper{};
  uint32_t flags = 0;        // kMovie* bits
  // Buffer sizes in bytes for one codebook, palette and set of vector
  // pointers, computed from the header with slack for compressed data loaded
  // at the end of the buffer.
  int32_t codebook_capacity = 0;
  int32_t palette_capacity = 0;
  int32_t pointers_capacity = 0;
  // The clock reading (kVqaTicksPerSecond) when playback ended or was last
  // paused, where a resumed movie restarts the clock.
  int64_t end_time = 0;
};

// VqaMovie flags.
// A drawn frame waits for ReleaseDrawnFrame() to release it; no other frame is
// drawn until then.
constexpr uint32_t kMovieAwaitingRelease = 1U << 0;
// The drawer has a frame ready and is waiting on kMovieAwaitingRelease, so it
// does not select another.
constexpr uint32_t kMovieDrawerAsleep = 1U << 1;
// The loader stopped inside a sound chunk until the audio ring has room; see
// chunk_header.
constexpr uint32_t kMovieLoaderAsleep = 1U << 2;
// The drawer and the loader have finished.
constexpr uint32_t kMovieDrawerDone = 1U << 3;
constexpr uint32_t kMovieLoaderDone = 1U << 4;
// PlayVqa() has configured the drawer and started the clock and sound.
constexpr uint32_t kMovieStarted = 1U << 5;
constexpr uint32_t kMoviePaused = 1U << 6;

// VqaPlayerState: the player state behind VqaPlayer, which owns one; tests use
// one directly to reach the internals. It outlives the movies opened on it.
struct VqaPlayerState {
  VqaPlayerState() = default;
  ~VqaPlayerState() = default;
  VqaPlayerState(const VqaPlayerState&) = delete;
  VqaPlayerState& operator=(const VqaPlayerState&) = delete;
  VqaPlayerState(VqaPlayerState&&) = delete;
  VqaPlayerState& operator=(VqaPlayerState&&) = delete;

  // Clears playback state so the player can open another movie. io is
  // intentionally preserved across reset.
  void Reset() {
    movie.reset();
    config = {};
    header = {};
  }

  // The file source installed by VqaPlayer::SetIo(). Not owned.
  VqaIo* io = nullptr;
  // The open movie's buffers: allocated by OpenVqa() and released by
  // CloseVqa(). nullptr while no movie is open.
  std::unique_ptr<VqaMovie> movie;
  // The copy of the caller's configuration, with the -1 defaults resolved
  // from the header.
  VqaConfig config{};
  // The open movie's VQHD header.
  VqaHeader header{};
};

// The player entry points behind VqaPlayer's Open(), Close(), Play() and
// SeekFrame(); see vqaplay.h for what they do. OpenVqa() and CloseVqa() are
// also the allocation and release of state->movie.
int32_t OpenVqa(VqaPlayerState* vqa, std::string_view filename,
                VqaConfig* config);
void CloseVqa(VqaPlayerState* vqa);
int32_t PlayVqa(VqaPlayerState* state, int32_t mode);
int32_t SeekVqaFrame(VqaPlayerState* vqa, int32_t framenum);

// Loads the next frame into the loader's frame buffer, collecting its
// codebook and sound on the way. Returns 0 when a frame was loaded, or
// kVqaNoBuffer (no free buffer), kVqaSleeping (waiting on the audio
// ring; call again to resume), kVqaEndOfMovie, or a read or seek error.
int32_t LoadNextFrame(VqaPlayerState* vqa);

// Places the image in the image buffer from config.margin_x/margin_y and the
// origin flags, and picks the decoder for the movie's block size. Runs once,
// when playback starts.
void ConfigureDrawer(VqaPlayerState* vqap);

// Decodes the drawer's next frame into the image buffer if it is due, hands it
// to the frame callback, and leaves it for ReleaseDrawnFrame(). Returns 0 when
// a frame was drawn; kVqaNotTime, kVqaNoBuffer or kVqaSleeping when none was;
// or kVqaEndOfMovie when the frame callback asked to stop.
int32_t DrawNextFrame(VqaPlayerState* vqa);

// The page flip: once the drawer has drawn a frame (kMovieAwaitingRelease),
// frees that frame's buffer for the loader.
void ReleaseDrawnFrame(const VqaPlayerState* state);

// Sets the movie clock to time (kVqaTicksPerSecond), using the kVqaClock*
// method, or the best one available, as its source.
void SetMovieClock(VqaPlayerState* vqap, int64_t time, int method);
// Returns the movie clock in kVqaTicksPerSecond.
int64_t ReadMovieClock(VqaPlayerState* vqap);

// Sound output. OpenMovieAudio() installs the SDL stream and callback,
// StartMovieAudio() and StopMovieAudio() start and stop the callback playing
// the ring, CloseMovieAudio() removes it all. CopyStagedAudio() moves the
// staged sound into the ring, or returns kVqaSleeping if the ring has no room.
int32_t OpenMovieAudio(VqaPlayerState* vqap);
void CloseMovieAudio(VqaPlayerState* vqap);
int32_t StartMovieAudio(VqaPlayerState* vqap);
void StopMovieAudio(const VqaPlayerState* vqap);
int32_t CopyStagedAudio(VqaPlayerState* vqap);

// Set once the loader has read the whole movie; from then on the audio
// callback counts a replayed block towards the clock, so the last frames
// still come due after the sound runs out. Written by OpenVqa() and
// PlayVqa() on the main thread and read on the audio thread.
extern std::atomic<bool> vqa_movie_loaded;

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQAPLAYP_H_
