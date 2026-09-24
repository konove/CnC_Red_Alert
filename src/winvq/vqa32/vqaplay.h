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
#ifndef CNC_RED_ALERT_WINVQ_VQA32_VQAPLAY_H_
#define CNC_RED_ALERT_WINVQ_VQA32_VQAPLAY_H_

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

// File: the public interface of the VQA movie player - VqaPlayer, the
// VqaConfig a movie is opened with, and the codes and flags that go with them.
// The player's internal state is in vqaplayp.h.
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

// Build switches of the original DOS library. Nothing in vqa32 tests them any
// more; only the standalone player in vplay32, which is not built, reads
// VQAAUDIO_ON, VQAVESA_ON and VQAWOOFER_ON.
#define VQASTANDALONE 0  // Stand alone player
#define VQAVOC_ON 0      // Enable VOC file override
#define VQAAUDIO_ON 1    // Audio playback enable/disable
#define VQAVIDEO_ON 0    // Video manager enable/disable
#define VQAMCGA_ON 0     // MCGA enable/disable
#define VQAXMODE_ON 0    // Xmode enable/disable
#define VQAVESA_ON 0     // VESA enable/disable
#define VQABLOCK_2X2 0   // 2x2 block decode enable/disable
#define VQABLOCK_2X3 0   // 2x3 block decode enable/disable
#define VQABLOCK_4X2 1   // 4x2 block decode enable/disable
#define VQABLOCK_4X4 1   // 4x4 block decode enable/disable
#define VQAWOOFER_ON 0   // Subwoofer track enable/disable

// Playback modes, the argument to VqaPlayer::Play().
constexpr int kVqaModeRun = 0;  // Play the movie through to the end.
constexpr int kVqaModeWalk =
    1;  // Load and draw at most one frame, then return.
constexpr int kVqaModePause = 2;  // Suspend playback and its audio.
constexpr int kVqaModeStop = 3;   // End playback, as if the movie had finished.

// Clocks the player can pace frames by (VqaConfig::clock_source). The audio
// clock is used only while sound plays and the interrupt clock never starts
// on this port, so both fall back to the system clock ("DOS").
constexpr int kVqaClockDefault =
    -1;                             // Audio if playing, else the system clock.
constexpr int kVqaClockSystem = 1;  // System clock
constexpr int kVqaClockInterrupt = 2;  // Timer interrupt tick count
constexpr int kVqaClockAudio = 3;      // Bytes of audio played so far

// Resolution of the player's clock: frame times, VQAStatistics::start_time and
// end_time are in ticks of 1/60 second.
constexpr int kVqaTicksPerSecond = 60;

// Error and status codes. The entry points return 0 or one of these; the ones
// from kVqaNoBuffer on are states of the loader and drawer, not failures.
constexpr int32_t kVqaOk = 0;  // No error
constexpr int32_t kVqaEndOfMovie =
    -1;  // Normal end of the movie, or stopped early
constexpr int32_t kVqaErrorOpen = -2;  // Unable to open
constexpr int32_t kVqaErrorRead =
    -3;                        // Read error, or a chunk that does not fit
#define VQAERR_WRITE (-4)      // Write error
constexpr int32_t kVqaErrorSeek = -5;      // Seek error
constexpr int32_t kVqaErrorNotVqa = -6;    // Not a valid VQA file.
constexpr int32_t kVqaErrorNoMemory = -7;  // Unable to allocate memory
constexpr int32_t kVqaNoBuffer =
    -8;  // No frame buffer free to load or ready to draw
constexpr int32_t kVqaNotTime = -9;    // Not time for the next frame yet
constexpr int32_t kVqaSleeping = -10;  // Waiting on the audio or the page flip
#define VQAERR_VIDEO (-11)     // Video related error.
constexpr int32_t kVqaErrorAudio = -12;  // Audio related error.
constexpr int32_t kVqaPaused = -13;      // In paused state.

// Events passed to VqaConfig::event_handler. Only kVqaEventSync is sent.
#define VQAEVENT_PALETTE (1 << 0)
constexpr uint32_t kVqaEventSync = 1U << 1;

// VqaConfig: how a movie is played. Start from SetVqaConfigDefaults() and
// change what differs; VqaPlayer::Open() copies it, and fills in the -1
// defaults from the movie's header in its copy.
//
// Fields marked "unused" are left from the DOS library and read by nothing.
struct VqaConfig {
  // Called with the image buffer and frame number after each frame is
  // decoded, and with nullptr for each frame skipped to keep up. A nonzero
  // return stops the movie. nullptr = none.
  int32_t (*frame_callback)(unsigned char* screen, int32_t framenum){};
  // Called with kVqaEventSync each time the player finds it is too early for
  // the next frame, so the client can present or wait instead of the player
  // spinning. nullptr = none.
  int32_t (*event_handler)(uint32_t event, void* buffer, int32_t nbytes){};
  // kVqaEvent* bits the client wants. Unused: event_handler gets every event.
  uint32_t NotifyFlags{};
  // DOS video mode (vqm32/video.h). Unused.
  int32_t Vmode{};
  // Vertical blank bit polarity for the DOS page flip. Copied into VqaMovie
  // and otherwise unused.
  int32_t VBIBit{};
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
  // Name of a VOC file to play instead of the movie's sound. Unused.
  char* VocFile{};
  // The caller's audio ring buffer. Empty = the player allocates
  // audio_buffer_bytes bytes.
  std::span<unsigned char> audio_buffer;
  // Size of the audio ring in bytes. -1 = as many audio_block_bytes blocks as
  // fit in 1.5 seconds of the movie's sound; 0 = no ring, so no sound.
  int32_t audio_buffer_bytes{};
  // Playback rate in samples per second. Unused.
  int32_t AudioRate{};
  // Playback volume, 0x00FF by default. Unused.
  int32_t Volume{};
  // Size in bytes of one audio block, the unit the audio ring is filled and
  // played in. Must be positive when a movie with sound plays with audio on.
  int32_t audio_block_bytes{};
  // HMI sound driver setup: handle, card ID, port, IRQ and DMA channel, -1
  // meaning autodetect. Unused.
  int32_t DigiHandle{};
  int32_t DigiCard{};
  int32_t DigiPort{};
  int32_t DigiIRQ{};
  int32_t DigiDMA{};
  // Preferred language. Unused.
  int32_t Language{};
  // Fonts for subtitle captions and, in C&C, E.V.A. text. Unused.
  char* CapFont{};
  char* EVAFont{};  // For C&C Only
};

// Drawing flags (VqaConfig::draw_flags): the VQACFGB_* bit numbers and the
// kVqaDraw* masks built from them.

// Decode into the image buffer; nothing is drawn without it.
#define VQACFGB_BUFFER 0
#define VQACFGB_NODRAW 1  // Load only; frames are discarded undrawn.
// Never skip frames to catch up. The audio callback clears it when the sound
// runs dry.
#define VQACFGB_NOSKIP 2
#define VQACFGB_VRAMCB 3   // XMode VRAM copy enable
#define VQACFGB_ORIGIN 4  // Two bits: the corner the margins are measured from.
#define VQACFGB_SCALEX2 6  // Scale X2 enable (VESA 320x200 to 640x400)
#define VQACFGB_WOOFER 7   // Subwoofer track
constexpr uint32_t kVqaDrawToBuffer = 1U << VQACFGB_BUFFER;
constexpr uint32_t kVqaDrawNothing = 1U << VQACFGB_NODRAW;
constexpr uint32_t kVqaDrawNoSkip = 1U << VQACFGB_NOSKIP;
#define VQACFGF_VRAMCB (1U << VQACFGB_VRAMCB)
constexpr uint32_t kVqaDrawOriginMask = 3U << VQACFGB_ORIGIN;
constexpr uint32_t kVqaDrawTopLeft = 0U << VQACFGB_ORIGIN;
constexpr uint32_t kVqaDrawTopRight = 1U << VQACFGB_ORIGIN;
constexpr uint32_t kVqaDrawBottomRight = 2U << VQACFGB_ORIGIN;
constexpr uint32_t kVqaDrawBottomLeft = 3U << VQACFGB_ORIGIN;
#define VQACFGF_SCALEX2 (1U << VQACFGB_SCALEX2)
#define VQACFGF_WOOFER (1U << VQACFGB_WOOFER)

// Player options (VqaConfig::option_flags).

// Play the sound track. Cleared by Open() when the movie has none.
#define VQAOPTB_AUDIO 0
// Draw every frame as soon as it is loaded, ignoring the clock.
#define VQAOPTB_STEP 1
#define VQAOPTB_UNUSED2 2   // Retired: mono debug output enable.
#define VQAOPTB_PALOFF 3    // Seeking does not restore the palette.
#define VQAOPTB_SLOWPAL 4   // Passed on to QueueVqaPalette().
#define VQAOPTB_HMIINIT 5   // HMI already initialized by client.
#define VQAOPTB_ALTAUDIO 6  // Use the alternate sound track, if there is one.
#define VQAOPTB_CAPTIONS 7  // Show captions. Unused.
#define VQAOPTB_EVA 8       // Show EVA text (For C&C only). Unused.
constexpr uint32_t kVqaOptionAudio = 1U << VQAOPTB_AUDIO;
constexpr uint32_t kVqaOptionStep = 1U << VQAOPTB_STEP;
constexpr uint32_t kVqaOptionPaletteOff = 1U << VQAOPTB_PALOFF;
constexpr uint32_t kVqaOptionSlowPalette = 1U << VQAOPTB_SLOWPAL;
#define VQAOPTF_HMIINIT (1U << VQAOPTB_HMIINIT)
constexpr uint32_t kVqaOptionAltAudio = 1U << VQAOPTB_ALTAUDIO;
#define VQAOPTF_CAPTIONS (1U << VQAOPTB_CAPTIONS)
#define VQAOPTF_EVA (1U << VQAOPTB_EVA)  // For C&C only

// VQAInfo: what VqaPlayer::GetInfo() reports about the open movie.
struct VQAInfo {
  // Number of frames, lowered by SetStop().
  int32_t NumFrames;
  // Frame size in pixels.
  int32_t image_width;
  int32_t image_height;
  // The buffer frames are decoded into; empty when there is none.
  std::span<unsigned char> image_buffer;
};

// VQAStatistics: what VqaPlayer::GetStats() reports about the playback so far.
struct VQAStatistics {
  // Clock readings (kVqaTicksPerSecond) when playback started and when it ended
  // or was last paused.
  int64_t start_time;
  int64_t end_time;
  int32_t FramesLoaded;
  int32_t FramesDrawn;
  // Frames dropped to keep up with the clock.
  int32_t FramesSkipped;
  // Size in bytes of the largest frame loaded.
  int32_t max_frame_bytes;
  // Number of sample bytes played. Nothing counts them, so this is always 0.
  int64_t SamplesPlayed;
  // Bytes the player allocated for the movie.
  int32_t allocated_bytes;
};

// The player's internal state; defined in vqaplayp.h.
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
  // fromwhere is ignored: frame is always counted from the start. Returns
  // the frame number seeked to, or a negative kVqaError* code.
  int SeekFrame(int frame, int fromwhere);

  // Shortens the open movie to its first `frame` frames. Returns the previous
  // frame count, or -1 (changing nothing) if frame is not between 1 and that
  // count.
  int SetStop(int frame);

  // Retrieve information/statistics about the open movie.
  void GetInfo(VQAInfo* info) const;
  void GetStats(VQAStatistics* stats) const;

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

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQAPLAY_H_
