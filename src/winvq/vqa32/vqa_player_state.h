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

#ifndef CNC_RED_ALERT_WINVQ_VQA32_VQA_PLAYER_STATE_H_
#define CNC_RED_ALERT_WINVQ_VQA32_VQA_PLAYER_STATE_H_

// File: the VQA player's internal state, VqaPlayerState, and the functions the
// player's source files share. Only vqa32 and its tests include it.
//
// The player is two cooperating parts that take turns in PlayVqa()'s loop on
// one thread. The loader reads frames and their codebooks from the file into
// a FrameRing; the drawer decodes a loaded frame into the image buffer when
// the clock says it is due, shows it and hands its buffer back to the loader.
// The sound runs separately, on the SDL audio thread, from a ring the loader
// fills.
//
// Originally written by Denzil E. Long, Jr. and Bill Randolph at Westwood
// Studios, August 1995.

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"
#include "winvq/vqa32/adpcm_decoders.h"
#include "winvq/vqa32/chunk_reader.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/lcw_buffer.h"
#include "winvq/vqa32/vq_decoder.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player.h"
#include "winvq/vqa32/vqaio.h"

// VqaLoader: the loader's position in the file and in the buffer rings.
struct VqaLoader {
  // Index in the ring of the codebook the partial codebooks of the current
  // group are collected into, to become the next group's codebook.
  int partial_codebook;
  // Index of the last complete codebook, used by the frames being loaded.
  int full_codebook;
  // Partial codebooks collected into partial_codebook so far, and their total
  // size in bytes (compressed or not).
  int32_t partial_count;
  int32_t partial_bytes;
  // Where compressed pieces collect in partial_codebook, estimated from the
  // group's first piece.
  int32_t partial_offset;
  // Number of the next frame to load; the movie is loaded when it reaches
  // the header's frame count.
  int32_t next_frame_number;
  // The chunk being loaded, kept so a loader woken from kMovieLoaderAsleep
  // resumes inside it instead of reading a new one.
  Chunk chunk;
};

// VqaDrawer: where and when the drawer decodes frames.
struct VqaDrawer {
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
  // The palette of the last frame skipped with one, set with the next frame
  // drawn while kDrawerPalettePending is set, and its size in bytes. At most
  // 256 colors, which is why the loader rejects larger palettes.
  int32_t saved_palette_bytes;
  std::array<unsigned char, 768> saved_palette;
  // The image size in blocks, the geometry the decoder walks.
  int32_t blocks_per_row;
  int32_t block_rows;
  // Number of the last frame selected for drawing. SelectFrameToDraw() draws
  // regardless of the clock once frame_rate / 5 frames have passed since, so
  // at least 5 frames a second reach the screen.
  int32_t last_selected_frame;
  // Number of the last frame drawn, which PlayVqa() returns in walk mode.
  int32_t last_drawn_frame;
};

// Drawer flag: a skipped frame's palette is pending.
constexpr uint32_t kDrawerPalettePending = base::Bit<uint32_t>(0);

// VqaAudio: the sound ring and the state shared with the SDL audio callback.
//
// The ring is block_count blocks of config.audio_block_bytes bytes. The
// loader decompresses each frame's sound chunk into staging, and
// CopyStagedAudio() moves it into the ring at write_offset, marking the blocks
// it filled in block_loaded. The callback, on the audio thread, plays
// play_block and frees it once the next block is loaded; if it is not, it
// plays the block again. The loader sleeps (kVqaSleeping) while the
// block it would overwrite is still unplayed. Code on the main thread holds the
// SDL device lock while changing what the callback reads.
struct VqaAudio {
  // One flag per ring block: 1 = holds unplayed sound, 0 = free to fill.
  std::vector<int16_t> block_loaded;
  // Staging for one frame's decompressed sound, staging_capacity bytes.
  std::vector<unsigned char> staging;
  // The ring, config.audio_buffer_bytes bytes; empty without sound.
  std::vector<unsigned char> ring;
  // Byte offset in the ring where the loader writes next.
  int32_t write_offset = 0;
  int32_t block_count = 0;
  // The block being played.
  int32_t play_block = 0;
  // Bytes in staging waiting for CopyStagedAudio(), 0 when it is empty.
  int32_t staged_bytes = 0;
  int32_t staging_capacity = 0;
  uint32_t flags = 0;  // kAudio* bits
  // Format of the track being played, the primary or the alternate one.
  int sample_rate = 0;
  int channels = 0;
  int bits_per_sample = 0;  // 8 or 16
  int32_t bytes_per_second = 0;
  // Decoder state for SND2 (IMA ADPCM) chunks, carried from chunk to chunk.
  ImaAdpcmDecoder adpcm;
  // Blocks handed to SDL since the sound started (a replayed block counts
  // only after the movie has loaded completely). ReadMovieClock() derives the
  // movie clock from it.
  int blocks_played = 0;
};

// Audio flags.
// The SDL stream and callback are installed.
constexpr uint32_t kAudioOpen = base::Bit<uint32_t>(0);
// The callback is playing the ring.
constexpr uint32_t kAudioPlaying = base::Bit<uint32_t>(6);

// VqaMovie: everything a movie needs while it is open. Allocated by OpenVqa()
// once the header is read and freed by CloseVqa().
struct VqaMovie {
  explicit VqaMovie(FrameRing frame_ring) : ring(std::move(frame_ring)) {}

  // The shape of the blocks frames are decoded from; nullopt when nothing is
  // decoded, because kVqaDrawToBuffer is clear or the block size has no
  // decoder.
  std::optional<BlockShape> block_shape;

  // The loaded frames and their codebooks.
  FrameRing ring;

  // The image buffer, when the player allocated it.
  std::vector<unsigned char> image_storage;

  VqaAudio audio;
  VqaLoader loader{};
  VqaDrawer drawer{};
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
// The loader stopped inside a sound chunk until the audio ring has room; see
// chunk_header.
constexpr uint32_t kMovieLoaderAsleep = base::Bit<uint32_t>(2);
// The drawer and the loader have finished; kMovieDone is both.
constexpr uint32_t kMovieDrawerDone = base::Bit<uint32_t>(3);
constexpr uint32_t kMovieLoaderDone = base::Bit<uint32_t>(4);
constexpr uint32_t kMovieDone = kMovieDrawerDone | kMovieLoaderDone;
// PlayVqa() has configured the drawer and started the clock and sound.
constexpr uint32_t kMovieStarted = base::Bit<uint32_t>(5);
constexpr uint32_t kMoviePaused = base::Bit<uint32_t>(6);

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

// The player entry points behind VqaPlayer's Open(), Close() and Play(); see
// vqa_player.h for what they do. OpenVqa() and CloseVqa() are
// also the allocation and release of state->movie.
int32_t OpenVqa(VqaPlayerState* state, std::string_view filename,
                VqaConfig* config);
void CloseVqa(VqaPlayerState* state);
int32_t PlayVqa(VqaPlayerState* state, int32_t mode);

// Loads the next frame into the loader's frame buffer, collecting its
// codebook and sound on the way. Returns 0 when a frame was loaded, or
// kVqaNoBuffer (no free buffer), kVqaSleeping (waiting on the audio
// ring; call again to resume), kVqaEndOfMovie, or a read or seek error.
int32_t LoadNextFrame(VqaPlayerState* state);

// Places the image in the image buffer from config.margin_x/margin_y and the
// origin flags, and picks the decoder for the movie's block size. Runs once,
// when playback starts.
void ConfigureDrawer(VqaPlayerState* state);

// Decodes the drawer's next frame into the image buffer if it is due, hands it
// to the frame callback and frees its buffer for the loader. Returns 0 when a
// frame was drawn; kVqaNotTime or kVqaNoBuffer when none was; or
// kVqaEndOfMovie when the frame callback asked to stop.
int32_t DrawNextFrame(VqaPlayerState* state);

// Makes the movie clock read now_ticks (kVqaTicksPerSecond), and picks what it
// runs on: the sound played so far when clock_source is kVqaClockDefault or
// kVqaClockAudio and sound is playing, else the system clock. PlayVqa() calls
// it when the movie starts and, with the time the pause began, when it
// resumes, so a pause does not count.
void SetMovieClock(const VqaPlayerState* state, int64_t now_ticks,
                   int clock_source);
// Returns the movie clock in kVqaTicksPerSecond, from the source and offset
// the last SetMovieClock() set.
int64_t ReadMovieClock(const VqaPlayerState* state);

// Sound output. The sound system plays one movie at a time; the functions
// below keep its state in audio.cc, shared by every VqaPlayerState.
//
// Creates the SDL stream converting the movie's sound to config.audio_spec
// and installs the player's mixer in config.audio_callback. Returns 0, or -1
// with nothing installed when SDL cannot convert to that spec.
int32_t OpenMovieAudio(VqaPlayerState* state);
// Stops the sound and, when this is the last open movie, removes the mixer
// and frees the stream.
void CloseMovieAudio(VqaPlayerState* state);
// Starts the mixer playing the movie's audio ring from play_block, with the
// audio clock back at zero. Returns 0, or -1 if a movie's sound is already
// playing.
int32_t StartMovieAudio(VqaPlayerState* state);
// Stops the mixer playing the ring; the sound already converted is kept for a
// restart.
void StopMovieAudio(const VqaPlayerState* state);
// Moves the staged sound (staged_bytes of staging) into the ring at
// write_offset, wrapping at its end, and marks the blocks it completes as
// loaded. Returns 0, also when there is nothing to move or the sound is off,
// or kVqaSleeping, with the sound still staged, when the blocks it would
// overwrite have not played yet.
int32_t CopyStagedAudio(VqaPlayerState* state);

// Set once the loader has read the whole movie; from then on the audio
// callback counts a replayed block towards the clock, so the last frames
// still come due after the sound runs out. Written by OpenVqa() and
// PlayVqa() on the main thread and read on the audio thread.
extern std::atomic<bool> vqa_movie_loaded;

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQA_PLAYER_STATE_H_
