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

// File: the public interface of the VQA movie player - VqaPlayer, the
// VQAConfig a movie is opened with, and the codes and flags that go with them.
// The player's internal state is in vqaplayp.h.
//
// Playing a movie is one blocking call: VqaPlayer::Play(VQAMODE_RUN) loads,
// decodes and paces the frames itself. Each decoded frame lands in an image
// buffer and is handed to the client through VQAConfig::DrawerCallback, which
// puts it on screen; the sound is fed to an SDL audio callback the client
// provides. The DOS video modes are gone, so the player only ever decodes into
// a buffer, and only when VQACFGF_BUFFER is set.
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
#define VQAMODE_RUN 0    // Play the movie through to the end.
#define VQAMODE_WALK 1   // Load and draw at most one frame, then return.
#define VQAMODE_PAUSE 2  // Suspend playback and its audio.
#define VQAMODE_STOP 3   // End playback, as if the movie had finished.

// Clocks the player can pace frames by (VQAConfig::TimerMethod). The audio
// clock is used only while sound plays and the interrupt clock never starts
// on this port, so both fall back to the system clock ("DOS").
#define VQA_TMETHOD_DEFAULT (-1)  // Audio if playing, else the system clock.
#define VQA_TMETHOD_DOS 1         // System clock
#define VQA_TMETHOD_INT 2         // Timer interrupt tick count
#define VQA_TMETHOD_AUDIO 3       // Bytes of audio played so far

// Resolution of the player's clock: frame times, VQAStatistics::StartTime and
// EndTime are in ticks of 1/60 second.
#define VQA_TIMETICKS 60

// Error and status codes. The entry points return 0 or one of these; the ones
// from VQAERR_NOBUFFER on are states of the loader and drawer, not failures.
#define VQAERR_NONE 0          // No error
#define VQAERR_EOF (-1)        // Normal end of the movie, or stopped early
#define VQAERR_OPEN (-2)       // Unable to open
#define VQAERR_READ (-3)       // Read error, or a chunk that does not fit
#define VQAERR_WRITE (-4)      // Write error
#define VQAERR_SEEK (-5)       // Seek error
#define VQAERR_NOTVQA (-6)     // Not a valid VQA file.
#define VQAERR_NOMEM (-7)      // Unable to allocate memory
#define VQAERR_NOBUFFER (-8)   // No frame buffer free to load or ready to draw
#define VQAERR_NOT_TIME (-9)   // Not time for the next frame yet
#define VQAERR_SLEEPING (-10)  // Waiting on the audio or the page flip
#define VQAERR_VIDEO (-11)     // Video related error.
#define VQAERR_AUDIO (-12)     // Audio related error.
#define VQAERR_PAUSED (-13)    // In paused state.

// Events passed to VQAConfig::EventHandler. Only VQAEVENT_SYNC is sent.
#define VQAEVENT_PALETTE (1 << 0)
#define VQAEVENT_SYNC (1 << 1)

// VQAConfig: how a movie is played. Start from VQA_DefaultConfig() and change
// what differs; VqaPlayer::Open() copies it, and fills in the -1 defaults
// from the movie's header in its copy.
//
// Fields marked "unused" are left from the DOS library and read by nothing.
struct VQAConfig {
  // Called with the image buffer and frame number after each frame is
  // decoded, and with nullptr for each frame skipped to keep up. A nonzero
  // return stops the movie. nullptr = none.
  int32_t (*DrawerCallback)(unsigned char* screen, int32_t framenum){};
  // Called with VQAEVENT_SYNC each time the player finds it is too early for
  // the next frame, so the client can present or wait instead of the player
  // spinning. nullptr = none.
  int32_t (*EventHandler)(uint32_t event, void* buffer, int32_t nbytes){};
  // VQAEVENT_* bits the client wants. Unused: EventHandler gets every event.
  uint32_t NotifyFlags{};
  // DOS video mode (vqm32/video.h). Unused.
  int32_t Vmode{};
  // Vertical blank bit polarity for the DOS page flip. Copied into VQAData
  // and otherwise unused.
  int32_t VBIBit{};
  // The caller's buffer to decode into, ImageWidth x ImageHeight bytes. Empty
  // = the player allocates one the size of the movie when VQACFGF_BUFFER is
  // set.
  std::span<unsigned char> ImageBuf;
  // Size of ImageBuf in pixels; -1 = the movie's frame size.
  int32_t ImageWidth{};
  int32_t ImageHeight{};
  // Gap in pixels between the image and the buffer corner that the
  // VQACFGF_ORIGIN bits of DrawFlags name. Both -1 = center the image.
  int32_t X1{}, Y1{};
  // Frames per second the movie is loaded at; -1 = the movie's rate.
  int32_t FrameRate{};
  // Frames per second to draw; -1 or 0 = the movie's rate. When it differs
  // from FrameRate, the drawer paces itself by the time since the last frame
  // drawn rather than by the frame numbers.
  int32_t DrawRate{};
  // VQA_TMETHOD_* clock to pace playback by.
  int32_t TimerMethod{};
  uint32_t DrawFlags{};    // VQACFGF_* bits
  uint32_t OptionFlags{};  // VQAOPTF_* bits
  // Frames decoded ahead of the one on screen, and codebooks kept for them.
  // Each must be at least 1, or Open() fails with VQAERR_NOMEM.
  int32_t NumFrameBufs{};
  int32_t NumCBBufs{};
  // The SDL device AudioCallback runs on. The player locks it while touching
  // state the callback shares.
  uint32_t AudioDeviceID{};  // SDL_AudioDeviceID
  // The client's callback slot, which its SDL audio callback calls through.
  // While a movie with sound is open the player installs its mixer there,
  // and clears the slot when the movie closes.
  void (**AudioCallback)(uint8_t*, int){};
  // The device's output format; the movie's sound is converted to it.
  // Required when VQAOPTF_AUDIO is set.
  void* AudioSpec{};  // pointer to an SDL_AudioSpec
  // Name of a VOC file to play instead of the movie's sound. Unused.
  char* VocFile{};
  // The caller's audio ring buffer. Empty = the player allocates
  // AudioBufSize bytes.
  std::span<unsigned char> AudioBuf;
  // Size of the audio ring in bytes. -1 = as many HMIBufSize blocks as fit
  // in 1.5 seconds of the movie's sound; 0 = no ring, so no sound.
  int32_t AudioBufSize{};
  // Playback rate in samples per second. Unused.
  int32_t AudioRate{};
  // Playback volume, 0x00FF by default. Unused.
  int32_t Volume{};
  // Size in bytes of one audio block, the unit the audio ring is filled and
  // played in. Must be positive when a movie with sound plays with audio on.
  int32_t HMIBufSize{};
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

// Drawing flags (VQAConfig::DrawFlags): the VQACFGB_* bit numbers and the
// VQACFGF_* masks built from them.

// Decode into the image buffer; nothing is drawn without it.
#define VQACFGB_BUFFER 0
#define VQACFGB_NODRAW 1  // Load only; frames are discarded undrawn.
// Never skip frames to catch up. The audio callback clears it when the sound
// runs dry.
#define VQACFGB_NOSKIP 2
#define VQACFGB_VRAMCB 3   // XMode VRAM copy enable
#define VQACFGB_ORIGIN 4   // Two bits: the corner X1,Y1 are measured from.
#define VQACFGB_SCALEX2 6  // Scale X2 enable (VESA 320x200 to 640x400)
#define VQACFGB_WOOFER 7   // Subwoofer track
#define VQACFGF_BUFFER (1U << VQACFGB_BUFFER)
#define VQACFGF_NODRAW (1U << VQACFGB_NODRAW)
#define VQACFGF_NOSKIP (1U << VQACFGB_NOSKIP)
#define VQACFGF_VRAMCB (1U << VQACFGB_VRAMCB)
#define VQACFGF_ORIGIN (3U << VQACFGB_ORIGIN)
#define VQACFGF_TOPLEFT (0U << VQACFGB_ORIGIN)
#define VQACFGF_TOPRIGHT (1U << VQACFGB_ORIGIN)
#define VQACFGF_BOTRIGHT (2U << VQACFGB_ORIGIN)
#define VQACFGF_BOTLEFT (3U << VQACFGB_ORIGIN)
#define VQACFGF_SCALEX2 (1U << VQACFGB_SCALEX2)
#define VQACFGF_WOOFER (1U << VQACFGB_WOOFER)

// Player options (VQAConfig::OptionFlags).

// Play the sound track. Cleared by Open() when the movie has none.
#define VQAOPTB_AUDIO 0
// Draw every frame as soon as it is loaded, ignoring the clock.
#define VQAOPTB_STEP 1
#define VQAOPTB_UNUSED2 2   // Retired: mono debug output enable.
#define VQAOPTB_PALOFF 3    // Seeking does not restore the palette.
#define VQAOPTB_SLOWPAL 4   // Passed on to Flag_To_Set_Palette().
#define VQAOPTB_HMIINIT 5   // HMI already initialized by client.
#define VQAOPTB_ALTAUDIO 6  // Use the alternate sound track, if there is one.
#define VQAOPTB_CAPTIONS 7  // Show captions. Unused.
#define VQAOPTB_EVA 8       // Show EVA text (For C&C only). Unused.
#define VQAOPTF_AUDIO (1U << VQAOPTB_AUDIO)
#define VQAOPTF_STEP (1U << VQAOPTB_STEP)
#define VQAOPTF_PALOFF (1U << VQAOPTB_PALOFF)
#define VQAOPTF_SLOWPAL (1U << VQAOPTB_SLOWPAL)
#define VQAOPTF_HMIINIT (1U << VQAOPTB_HMIINIT)
#define VQAOPTF_ALTAUDIO (1U << VQAOPTB_ALTAUDIO)
#define VQAOPTF_CAPTIONS (1U << VQAOPTB_CAPTIONS)
#define VQAOPTF_EVA (1U << VQAOPTB_EVA)  // For C&C only

// VQAInfo: what VqaPlayer::GetInfo() reports about the open movie.
struct VQAInfo {
  // Number of frames, lowered by SetStop().
  int32_t NumFrames;
  // Frame size in pixels.
  int32_t ImageWidth;
  int32_t ImageHeight;
  // The buffer frames are decoded into; nullptr when there is none.
  unsigned char* ImageBuf;
};

// VQAStatistics: what VqaPlayer::GetStats() reports about the playback so far.
struct VQAStatistics {
  // Clock readings (VQA_TIMETICKS) when playback started and when it ended or
  // was last paused.
  int64_t StartTime;
  int64_t EndTime;
  int32_t FramesLoaded;
  int32_t FramesDrawn;
  // Frames dropped to keep up with the clock.
  int32_t FramesSkipped;
  // Size in bytes of the largest frame loaded.
  int32_t MaxFrameSize;
  // Number of sample bytes played. Nothing counts them, so this is always 0.
  int64_t SamplesPlayed;
  // Bytes the player allocated for the movie.
  int32_t MemUsed;
};

// The player's internal state; defined in vqaplayp.h.
struct VQAHandle;

// Abstract file source the player reads movies through (see vqaio.h).
class VqaIo;

// Plays VQA movies.
//
// Example:
//   VqaPlayer player;
//   GameFileVqaIo io;  // any VqaIo implementation
//   player.SetIo(&io);
//   if (player.Open("INTRO.VQA", &AnimControl) == 0) {
//     player.Play(VQAMODE_RUN);
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

  // Opens a movie for playback and preloads NumFrameBufs frames. config may
  // be nullptr to use defaults; the configuration is copied. No movie may be
  // open already. Returns 0 on success or a VQAERR_* code, with the player
  // closed again on failure.
  int Open(const char* filename, VQAConfig* config);

  // Closes the movie, if one is open, and makes the player reusable.
  void Close();

  // Runs playback of the open movie in the given VQAMODE_* mode. VQAMODE_RUN
  // blocks until the movie ends or DrawerCallback stops it, and returns
  // VQAERR_EOF. VQAMODE_WALK returns after one frame: the number of the frame
  // drawn, 0, or a VQAERR_* state such as VQAERR_NOT_TIME. A read error while
  // loading ends the movie as if it were the last frame.
  int Play(int mode);

  // Repositions the open movie to the given frame, reloading the codebooks
  // from the start of the previous group and, unless VQAOPTF_PALOFF is set,
  // the palette in force; the frame offsets come from the FINF table.
  // fromwhere is ignored: frame is always counted from the start. Returns
  // the frame number seeked to, or a negative VQAERR_* code.
  int SeekFrame(int frame, int fromwhere);

  // Shortens the open movie to its first `frame` frames. Returns the previous
  // frame count, or -1 (changing nothing) if frame is not between 1 and that
  // count.
  int SetStop(int frame);

  // Retrieve information/statistics about the open movie.
  void GetInfo(VQAInfo* info) const;
  void GetStats(VQAStatistics* stats) const;

 private:
  std::unique_ptr<VQAHandle> impl_;
};

// Fills config with the defaults: a 320x200 buffer, centered image, the
// movie's frame rate, 6 frame and 3 codebook buffers, audio on with an audio
// ring sized from the movie, and no frames decoded until the caller sets
// VQACFGF_BUFFER.
void VQA_DefaultConfig(VQAConfig* config);

// Pause and resume the playing movie's sound, for when the game window loses
// and regains focus. The movie's clock follows the sound, so the frames wait
// too. Both do nothing when no movie sound is playing.
void VQA_PauseAudio();
void VQA_ResumeAudio();

// Supplied by the game: queue a palette change for the next frame.
void Flag_To_Set_Palette(std::span<uint8_t> palette, int32_t numbytes,
                         uint32_t slowpal);

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQAPLAY_H_
