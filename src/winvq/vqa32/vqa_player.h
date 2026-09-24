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
#include <memory>
#include <span>
#include <string_view>

#include "base/numeric.h"

// File: the public interface of the VQA movie player - VqaPlayer, the
// VqaConfig a movie is opened with, and the codes and flags that go with them.
// The player's internal state is in vqa_player_state.h.
//
// Playing a movie is one blocking call: VqaPlayer::Play(kVqaModeRun) loads,
// decodes and paces the frames itself. Each decoded frame lands in an image
// buffer and is handed to the client through VqaConfig::frame_callback, which
// puts it on screen; the sound is fed to an SDL audio callback the client
// provides. The DOS video modes are gone, so the player only ever decodes into
// a buffer, and only when kVqaDrawToBuffer is set.
//
// Originally written by Bill Randolph and Denzil E. Long, Jr. at Westwood
// Studios, April 1995.

// Playback modes, the argument to VqaPlayer::Play().
// Play the movie through to the end.
constexpr int kVqaModeRun = 0;
// Load and draw at most one frame, then return.
constexpr int kVqaModeWalk = 1;
// Suspend playback and its audio.
constexpr int kVqaModePause = 2;
// End playback, as if the movie had finished.
constexpr int kVqaModeStop = 3;

// Clocks the player can pace frames by (VqaConfig::clock_source). The audio
// clock, the amount of sound played so far, works only while sound plays;
// otherwise the player falls back to the system clock.
// Audio if playing, else the system clock.
constexpr int kVqaClockDefault = -1;
constexpr int kVqaClockSystem = 1;
constexpr int kVqaClockAudio = 3;

// Resolution of the player's clock: frame times are in ticks of 1/60 second.
constexpr int kVqaTicksPerSecond = 60;

// Result codes. The entry points return 0 or one of these; the ones from
// kVqaNoBuffer on are states of the loader and drawer, not failures.
constexpr int32_t kVqaOk = 0;
// Normal end of the movie, or stopped early.
constexpr int32_t kVqaEndOfMovie = -1;
constexpr int32_t kVqaErrorOpen = -2;
// A read error, or a chunk that does not fit its buffer.
constexpr int32_t kVqaErrorRead = -3;
constexpr int32_t kVqaErrorSeek = -5;
// Not a valid VQA file.
constexpr int32_t kVqaErrorNotVqa = -6;
constexpr int32_t kVqaErrorNoMemory = -7;
// No frame buffer free to load or ready to draw.
constexpr int32_t kVqaNoBuffer = -8;
// Not time for the next frame yet.
constexpr int32_t kVqaNotTime = -9;
// Waiting on the audio or the page flip.
constexpr int32_t kVqaSleeping = -10;
constexpr int32_t kVqaErrorAudio = -12;
constexpr int32_t kVqaPaused = -13;

// The event passed to VqaConfig::event_handler.
constexpr uint32_t kVqaEventSync = base::Bit<uint32_t>(1);

// VqaConfig: how a movie is played. Start from SetVqaConfigDefaults() and
// change what differs; VqaPlayer::Open() copies it, and fills in the -1
// defaults from the movie's header in its copy.
struct VqaConfig {
  // Called with the image buffer and frame number after each frame is
  // decoded, and with nullptr for each frame skipped to keep up. A nonzero
  // return stops the movie. nullptr = none.
  int32_t (*frame_callback)(unsigned char* screen, int32_t framenum){};
  // Called with kVqaEventSync each time the player finds it is too early for
  // the next frame, so the client can present or wait instead of the player
  // spinning. nullptr = none.
  int32_t (*event_handler)(uint32_t event, void* buffer, int32_t nbytes){};
  // The caller's buffer to decode into, image_width x image_height bytes. Empty
  // = the player allocates one the size of the movie when kVqaDrawToBuffer is
  // set.
  std::span<unsigned char> image_buffer;
  // Size of image_buffer in pixels; -1 = the movie's frame size.
  int32_t image_width{};
  int32_t image_height{};
  // Gap in pixels between the image and the buffer corner that the
  // kVqaDrawOriginMask bits of draw_flags name. Both -1 = center the image.
  int32_t margin_x{}, margin_y{};
  // Frames per second the movie is loaded at; -1 = the movie's rate.
  int32_t frame_rate{};
  // Frames per second to draw; -1 or 0 = the movie's rate. When it differs
  // from frame_rate, the drawer paces itself by the time since the last frame
  // drawn rather than by the frame numbers.
  int32_t draw_rate{};
  // kVqaClock* clock to pace playback by.
  int32_t clock_source{};
  uint32_t draw_flags{};    // kVqaDraw* bits
  uint32_t option_flags{};  // kVqaOption* bits
  // Frames decoded ahead of the one on screen, and codebooks kept for them.
  // Each must be at least 1, or Open() fails with kVqaErrorNoMemory.
  int32_t frame_buffer_count{};
  int32_t codebook_buffer_count{};
  // The SDL device audio_callback runs on. The player locks it while touching
  // state the callback shares.
  uint32_t audio_device_id{};  // SDL_AudioDeviceID
  // The client's callback slot, which its SDL audio callback calls through.
  // While a movie with sound is open the player installs its mixer there,
  // and clears the slot when the movie closes.
  void (**audio_callback)(uint8_t*, int){};
  // The device's output format; the movie's sound is converted to it.
  // Required when kVqaOptionAudio is set.
  void* audio_spec{};  // pointer to an SDL_AudioSpec
  // The caller's audio ring buffer. Empty = the player allocates
  // audio_buffer_bytes bytes.
  std::span<unsigned char> audio_buffer;
  // Size of the audio ring in bytes. -1 = as many audio_block_bytes blocks as
  // fit in 1.5 seconds of the movie's sound; 0 = no ring, so no sound.
  int32_t audio_buffer_bytes{};
  // Size in bytes of one audio block, the unit the audio ring is filled and
  // played in. Must be positive when a movie with sound plays with audio on.
  int32_t audio_block_bytes{};
};

// Drawing flags (VqaConfig::draw_flags).
// Decode into the image buffer; nothing is drawn without it.
constexpr uint32_t kVqaDrawToBuffer = base::Bit<uint32_t>(0);
// Load only; frames are discarded undrawn.
constexpr uint32_t kVqaDrawNothing = base::Bit<uint32_t>(1);
// Never skip frames to catch up. The audio callback clears it when the sound
// runs dry.
constexpr uint32_t kVqaDrawNoSkip = base::Bit<uint32_t>(2);
// Two bits naming the buffer corner the margins are measured from.
constexpr uint32_t kVqaDrawOriginMask = 3U << 4;
constexpr uint32_t kVqaDrawTopLeft = 0U << 4;
constexpr uint32_t kVqaDrawTopRight = 1U << 4;
constexpr uint32_t kVqaDrawBottomRight = 2U << 4;
constexpr uint32_t kVqaDrawBottomLeft = 3U << 4;

// Player options (VqaConfig::option_flags).
// Play the sound track. Cleared by Open() when the movie has none.
constexpr uint32_t kVqaOptionAudio = base::Bit<uint32_t>(0);
// Draw every frame as soon as it is loaded, ignoring the clock.
constexpr uint32_t kVqaOptionStep = base::Bit<uint32_t>(1);
// Seeking does not restore the palette.
constexpr uint32_t kVqaOptionPaletteOff = base::Bit<uint32_t>(3);
// Passed on to QueueVqaPalette().
constexpr uint32_t kVqaOptionSlowPalette = base::Bit<uint32_t>(4);
// Use the alternate sound track, if there is one.
constexpr uint32_t kVqaOptionAltAudio = base::Bit<uint32_t>(6);

// The player's internal state; defined in vqa_player_state.h.
struct VqaPlayerState;

// Abstract file source the player reads movies through (see vqaio.h).
class VqaIo;

// Plays VQA movies.
//
// Example:
//   VqaPlayer player;
//   GameFileVqaIo io;  // any VqaIo implementation
//   player.SetIo(&io);
//   if (player.Open("INTRO.VQA", &AnimControl) == 0) {
//     player.Play(kVqaModeRun);
//     player.Close();
//   }
//
// The player owns its internal state but never the io object. Destroying
// the player closes any movie still open.
class VqaPlayer {
 public:
  VqaPlayer();
  ~VqaPlayer();
  VqaPlayer(const VqaPlayer&) = delete;
  VqaPlayer& operator=(const VqaPlayer&) = delete;
  VqaPlayer(VqaPlayer&&) = delete;
  VqaPlayer& operator=(VqaPlayer&&) = delete;

  // Installs the file source used by Open()/Play(). Non-owning: io must stay
  // alive while a movie opened through it is open. Survives Close(), so one
  // player can open several movies in sequence.
  void SetIo(VqaIo* io);

  // Opens a movie for playback and preloads frame_buffer_count frames. config
  // may be nullptr to use defaults; the configuration is copied. No movie may
  // be open already. Returns 0 on success or a kVqaError* code, with the player
  // closed again on failure.
  int Open(std::string_view filename, VqaConfig* config);

  // Closes the movie, if one is open, and makes the player reusable.
  void Close();

  // Runs playback of the open movie in the given kVqaMode* mode. kVqaModeRun
  // blocks until the movie ends or frame_callback stops it, and returns
  // kVqaEndOfMovie. kVqaModeWalk returns after one frame: the number of the
  // frame drawn, 0, or a state such as kVqaNotTime. A read error while loading
  // ends the movie as if it were the last frame.
  int Play(int mode);

  // Repositions the open movie to the given frame, reloading the codebooks
  // from the start of the previous group and, unless kVqaOptionPaletteOff is
  // set, the palette in force; the frame offsets come from the FINF table.
  // frame counts from the start of the movie. Returns the frame number seeked
  // to, or a negative kVqaError* code.
  int SeekFrame(int frame);

 private:
  std::unique_ptr<VqaPlayerState> impl_;
};

// Fills config with the defaults: a 320x200 buffer, centered image, the
// movie's frame rate, 6 frame and 3 codebook buffers, audio on with an audio
// ring sized from the movie, and no frames decoded until the caller sets
// kVqaDrawToBuffer.
void SetVqaConfigDefaults(VqaConfig* config);

// Pause and resume the playing movie's sound, for when the game window loses
// and regains focus. The movie's clock follows the sound, so the frames wait
// too. Both do nothing when no movie sound is playing.
void PauseVqaAudio();
void ResumeVqaAudio();

// Supplied by the game: queue a palette change for the next frame.
void QueueVqaPalette(std::span<uint8_t> palette, int32_t numbytes,
                     uint32_t slowpal);

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQA_PLAYER_H_
