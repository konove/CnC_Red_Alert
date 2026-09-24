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
#ifndef CNC_RED_ALERT_WINVQ_VQA32_VQA_PLAYER_H_
#define CNC_RED_ALERT_WINVQ_VQA32_VQA_PLAYER_H_

#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

// File: the public interface of the VQA movie player - VqaPlayer, the
// VqaClient that shows its frames, and the options it is opened with.
//
// A movie needs three things from whoever plays it: a VqaIo to read the file
// through (vqaio.h), a VqaClient to show the frames, and optionally a
// VqaAudioDevice to play the sound on (vqa_audio_device.h). The player decodes
// each frame into an image of its own, the movie's size, and hands the client
// a view of it; placing and scaling it is the client's business.
//
// Originally written by Bill Randolph and Denzil E. Long, Jr. at Westwood
// Studios, April 1995.

class Movie;
class VqaAudioDevice;
class VqaIo;

// Why VqaPlayer::Open() failed.
enum class VqaError {
  kOpen,      // The file could not be opened.
  kRead,      // A read failed, a chunk did not fit its buffer, or the movie
              // ends before its last frame.
  kSeek,      // A chunk could not be skipped.
  kNotVqa,    // Not a VQA movie, or a malformed header.
  kNoMemory,  // VqaOptions asks for no frame or codebook buffers.
  kAudio,     // The sound cannot be played on the device given.
};

// A decoded frame, valid only during the VqaClient call it is passed to.
struct VqaFrameView {
  // Number of the frame in the movie.
  int frame_number = 0;
  // The movie's frame size in pixels.
  int width = 0;
  int height = 0;
  // width * height 8-bit palette indexes, row by row.
  std::span<const uint8_t> pixels;
  // The palette to show the frame with, as 8-bit R,G,B triplets of up to 256
  // colors; empty when the palette is unchanged. A skipped frame's palette is
  // carried over to the next frame shown.
  std::span<const uint8_t> palette;
};

// Shows a movie's frames. Implemented by whoever plays the movie.
//
// Example:
//   class Screen final : public VqaClient {
//     bool OnFrame(const VqaFrameView& frame) override {
//       Blit(frame.pixels, frame.width, frame.height);
//       return !EscapePressed();
//     }
//   };
class VqaClient {
 public:
  VqaClient() = default;
  virtual ~VqaClient() = default;
  VqaClient(const VqaClient&) = delete;
  VqaClient& operator=(const VqaClient&) = delete;
  VqaClient(VqaClient&&) = delete;
  VqaClient& operator=(VqaClient&&) = delete;

  // Shows a frame. Returns false to stop the movie.
  virtual bool OnFrame(const VqaFrameView& frame) = 0;
  // A frame was dropped because playback ran late. Returns false to stop the
  // movie.
  virtual bool OnFrameSkipped(int /*frame_number*/) { return true; }
  // It is too early for the next frame: present, or wait, rather than let the
  // player spin.
  virtual void OnIdle() {}
};

// How a movie plays. The defaults suit the games.
struct VqaOptions {
  // Drop late frames to catch up. Off, frames are only dropped once the sound
  // has run dry, which means the machine cannot keep up.
  bool skip_late_frames = false;
  // Frames decoded ahead of the one on screen, and codebooks kept for them.
  // Each must be at least 1.
  int frame_buffers = 6;
  int codebook_buffers = 3;
  // Size of the sound ring in bytes, rounded down to whole audio blocks;
  // nullopt = as many blocks as fit in 1.5 seconds of the movie's sound. Less
  // than one block plays the movie without sound.
  std::optional<int> audio_ring_bytes;
  // Size in bytes of one audio block, the unit the ring is filled and played
  // in. Must be positive for a movie with sound.
  int audio_block_bytes = 2048;
};

// What VqaPlayer::Step() did.
enum class VqaStepResult {
  kFrameShown,  // A frame was shown.
  kWaiting,     // Nothing to show yet.
  kEnded,       // The movie has ended, or the client stopped it.
};

// Plays a VQA movie. Open() reads the header and preloads the first frames;
// Run() plays the movie to the end. Destroying the player closes the movie.
//
// Example:
//   GameFileVqaIo io;
//   Screen screen;
//   if (auto player = VqaPlayer::Open(io, "INTRO.VQA", screen, &audio)) {
//     player->Run();
//   }
class VqaPlayer {
 public:
  // Opens the named movie through io, to be shown by client and heard on
  // audio, or silently when audio is nullptr. io, client and audio must
  // outlive the player.
  static std::expected<VqaPlayer, VqaError> Open(
      VqaIo& io, std::string_view name, VqaClient& client,
      VqaAudioDevice* audio, const VqaOptions& options = {});

  ~VqaPlayer();
  VqaPlayer(VqaPlayer&& other) noexcept;
  VqaPlayer& operator=(VqaPlayer&& other) noexcept;
  VqaPlayer(const VqaPlayer&) = delete;
  VqaPlayer& operator=(const VqaPlayer&) = delete;

  // Plays the movie until it ends or the client stops it.
  void Run();
  // Gives the loader and the drawer one turn each and returns what came of
  // it; for a caller with its own loop. The sound plays on between calls.
  VqaStepResult Step();
  // Number of the last frame shown; 0 before the first.
  [[nodiscard]] int last_frame_shown() const;

 private:
  explicit VqaPlayer(std::unique_ptr<Movie> movie);

  std::unique_ptr<Movie> movie_;
};

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQA_PLAYER_H_
