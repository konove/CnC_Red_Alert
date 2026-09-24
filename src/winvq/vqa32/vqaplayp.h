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

// File: the VQA player's internal state, VQAHandle, and the functions the
// player's source files share. Only vqa32 and its tests include it.
//
// The player is three cooperating parts that take turns in VQA_Play()'s loop
// on one thread. The loader reads frames from the file into a ring of frame
// buffers and their codebooks into a ring of codebook buffers; the drawer
// decodes a loaded frame into the image buffer when the clock says it is due;
// the "flipper" (User_Update) then hands the frame's buffer back to the
// loader. Each part keeps its own position in the rings, so a frame buffer's
// Flags are how they tell each other it is full or free. The sound runs
// separately, on the SDL audio thread, from a ring the loader fills.
//
// Originally written by Denzil E. Long, Jr. and Bill Randolph at Westwood
// Studios, August 1995.

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqaio.h"
#include "winvq/vqa32/vqaplay.h"
#include "winvq/vqm32/soscomp.h"
#include "winvq/vqm32/video.h"

// Identification of the original library. Unused.
#define VQA_VERSION "2.42"
#define VQA_DATE __DATE__ " " __TIME__

#define VQA_IDSTRING "VQA32 " VQA_VERSION " (" VQA_DATE ")"
#define VQA_REQUIRES "VQM32 2.12 or better."

// Packs a block width and height into the dimension code the drawer switches
// on to pick an UnVQ routine. Only 4x2 and 4x4 blocks have one.
constexpr uint32_t BLOCK_DIM(uint32_t a, uint32_t b) {
  return ((a & 0xFF) << 8) | (b & 0xFF);
}
#define BLOCK_2X2 BLOCK_DIM(2, 2)
#define BLOCK_2X3 BLOCK_DIM(2, 3)
#define BLOCK_4X2 BLOCK_DIM(4, 2)
#define BLOCK_4X4 BLOCK_DIM(4, 4)

// Limits of the DOS library. Unused: the buffer counts come from VQAConfig.
#define VQA_MAX_CBBUFS 10     // Maximum number of codebook buffers
#define VQA_MAX_FRAMEBUFS 30  // Maximum number of frame buffers

// Vector pointer value that marked a masked-out block. Unused.
#define VQA_MASK_POINTER 0x8000

// ChunkHeader: the 8 bytes in front of every IFF chunk, read raw. id compares
// against the ID_ and kChunk constants as read; size is big-endian and only
// usable through ChunkSize() in loader.cc. The payload that follows is padded
// to an even length.
struct ChunkHeader {
  uint32_t id;
  uint32_t size;
};

// ZAPHeader: the header of a SND1 (Westwood ADPCM) sound chunk. Equal sizes
// mean the sound is stored uncompressed.
struct ZAPHeader {
  uint16_t UnCompSize;  // Bytes of sound after decompression
  uint16_t CompSize;    // Bytes of sound in the chunk
};

// VQACBNode: one buffer in the ring of codebooks. A codebook is the table of
// pixel blocks a frame's vector pointers index into, and serves every frame
// of a group. The loader assembles the next group's codebook from the
// partial codebooks in the current group's frames, so the ring needs at
// least that one codebook ahead of the one in use.
//
// Compressed data is loaded at the end of the buffer (CBOffset) and
// decompressed in place towards the start by the drawer when the first frame
// using it is drawn; uncompressed data is loaded at the start.
struct VQACBNode {
  // Max_CB_Size bytes.
  std::vector<unsigned char> BufferStorage;
  unsigned char* Buffer =
      nullptr;  // Points into BufferStorage for compatibility
  VQACBNode* Next = nullptr;
  uint32_t Flags = 0;  // VQACBF_* bits
  // Where the compressed data starts in BufferStorage.
  int32_t CBOffset = 0;
};

// VQACBNode flags, set by the loader for each new codebook.
#define VQACBB_DOWNLOADED 0  // Download codebook to VRAM (XMODE VRAM)
#define VQACBB_CBCOMP 1      // Codebook is still compressed
#define VQACBF_DOWNLOADED (1U << VQACBB_DOWNLOADED)
#define VQACBF_CBCOMP (1U << VQACBB_CBCOMP)

// VQAFrameNode: one buffer in the ring of loaded frames. The loader fills it
// and sets VQAFRMF_LOADED; the drawer decodes it; the flipper or a skip
// clears Flags to hand it back to the loader. Compressed data sits at the end
// of its buffer and is decompressed in place just before the frame is drawn,
// as in VQACBNode.
struct VQAFrameNode {
  // The frame's vector pointers, one per block, Max_Ptr_Size bytes.
  std::vector<unsigned char> PointersStorage;
  // The frame's palette as 8-bit R,G,B triplets, Max_Pal_Size bytes. Holds
  // a stale palette when the frame has none (VQAFRMF_PALETTE clear).
  std::vector<unsigned char> PaletteStorage;
  unsigned char* Pointers = nullptr;  // Points into PointersStorage
  // The codebook the frame's pointers index into.
  VQACBNode* Codebook = nullptr;
  unsigned char* Palette = nullptr;  // Points into PaletteStorage
  VQAFrameNode* Next = nullptr;
  uint32_t Flags = 0;  // VQAFRMF_* bits
  // Number of the frame in the movie.
  int32_t FrameNum = 0;
  // Where the compressed pointers and palette start in their buffers.
  int32_t PtrOffset = 0;
  int32_t PalOffset = 0;
  // Bytes of palette, meaningful when VQAFRMF_PALETTE is set.
  int32_t PaletteSize = 0;
};

// VQAFrameNode flags. All clear means the buffer is free for the loader.
#define VQAFRMB_LOADED 0   // Loaded and waiting to be drawn
#define VQAFRMB_KEY 1      // Key frame: never skipped
#define VQAFRMB_PALETTE 2  // Carries a palette that must be set
#define VQAFRMB_PALCOMP 3  // Palette is still compressed
#define VQAFRMB_PTRCOMP 4  // Vector pointer data is still compressed
#define VQAFRMF_LOADED (1U << VQAFRMB_LOADED)
#define VQAFRMF_KEY (1U << VQAFRMB_KEY)
#define VQAFRMF_PALETTE (1U << VQAFRMB_PALETTE)
#define VQAFRMF_PALCOMP (1U << VQAFRMB_PALCOMP)
#define VQAFRMF_PTRCOMP (1U << VQAFRMB_PTRCOMP)

// VQALoader: the loader's position in the file and in the buffer rings.
struct VQALoader {
  // The codebook node the partial codebooks of the current group are
  // collected into, to become the next group's codebook.
  VQACBNode* CurCB;
  // The last complete codebook, used by the frames being loaded.
  VQACBNode* FullCB;
  // The frame node the next frame is loaded into.
  VQAFrameNode* CurFrame;
  // Partial codebooks collected into CurCB so far, and their total size in
  // bytes (compressed or not).
  int32_t NumPartialCB;
  int32_t PartialCBSize;
  // Number of the next frame to load; the movie is loaded when it reaches
  // the header's frame count.
  int32_t CurFrameNum;
  // First frame of the last codebook group. Set, never read.
  int32_t LastCBFrame;
  // Number of frames loaded when the last one finished. Set, never read.
  int32_t LastFrameNum;
  // Times the loader found no free frame buffer. Never read.
  int32_t WaitsOnDrawer;
  // Never set or read.
  int32_t WaitsOnAudio;
  // Bytes of chunks read for the frame being loaded, and the largest total
  // of a whole frame after the first, which also carries the movie's
  // opening codebook (reported by VQA_GetStats()).
  int32_t FrameSize;
  int32_t MaxFrameSize;
  // Header of the chunk being loaded, kept so a loader woken from
  // VQADATF_LSLEEP resumes inside it instead of reading a new one.
  ChunkHeader CurChunkHdr;
};

// VQADrawer: where and when the drawer decodes frames.
struct VQADrawer {
  // The next frame to draw.
  VQAFrameNode* CurFrame;
  uint32_t Flags;  // VQADRWF_* bits
  // The DOS video mode's description. Unused.
  DisplayInfo* Display;
  // The buffer frames are decoded into, ImageWidth x ImageHeight pixels:
  // the caller's, the player's own, or empty when neither was provided.
  std::span<unsigned char> ImageBuf;
  int32_t ImageWidth;
  int32_t ImageHeight;
  // Inclusive image corners in buffer pixels. X1,Y1 is the corner anchored
  // by the VQACFGF_ORIGIN flags, X2,Y2 the opposite one.
  int32_t X1, Y1, X2, Y2;
  // Index in ImageBuf of the image's top-left pixel.
  int32_t ScreenOffset;
  // Size in bytes of Palette_24's contents; 0 until the loader has seen the
  // movie's first palette.
  int32_t CurPalSize;
  // The movie's first palette, copied there by the loader, and later the
  // palette of a frame skipped with VQADRWF_SETPAL. At most 256 colors, which
  // is why the loader rejects larger palettes.
  unsigned char Palette_24[768];
  // 15-bit version of Palette_24, for 32K-color modes. Unused.
  unsigned char Palette_15[512];
  // The image size in blocks, the geometry UnVQ walks.
  int32_t BlocksPerRow;
  int32_t NumRows;
  // BlocksPerRow * NumRows. Set, never read.
  int32_t NumBlocks;
  // A rectangle of blocks the DOS drawer left undrawn. Unused.
  int32_t MaskStart;
  int32_t MaskWidth;
  int32_t MaskHeight;
  // When the frame-skipping path last passed a frame; starts one second in
  // the past so the first frame is never early.
  int64_t LastTime;  // In VQA_TIMETICKS, as returned by VQA_GetTime().
  // Number of the last frame selected for drawing. Select_Frame() draws
  // regardless of the clock once FrameRate / 5 frames have passed since, so
  // at least 5 frames a second reach the screen.
  int32_t LastFrame;
  // Number of the last frame drawn, which VQA_Play() returns in walk mode.
  int32_t LastFrameNum;
  // Never set or read.
  int32_t DesiredFrame;
  // Frames skipped to keep up with the clock.
  int32_t NumSkipped;
  // Times the drawer waited for a page flip and for a loaded frame. Never
  // read.
  int32_t WaitsOnFlipper;
  int32_t WaitsOnLoader;
};

// Drawer flags.
#define VQADRWB_SETPAL 0  // A skipped frame's palette is pending.
#define VQADRWF_SETPAL (1U << VQADRWB_SETPAL)

// VQAFlipper: the frame the drawer finished, which User_Update() releases.
// The name is from DOS, where this step showed the frame by flipping video
// pages; now the client's DrawerCallback has already shown it.
struct VQAFlipper {
  // The frame drawn last; valid while VQADATF_UPDATE is set.
  VQAFrameNode* CurFrame;
  // Number of the last frame released. Never read.
  int32_t LastFrameNum;
};

// VQAAudio: the sound ring and the state shared with the SDL audio callback.
//
// The ring (Buffer) is NumAudBlocks blocks of config.HMIBufSize bytes. The
// loader decompresses each frame's sound chunk into TempBuf, and CopyAudio()
// moves it into the ring at AudBufPos, marking the blocks it filled in
// IsLoadedStorage. The callback, on the audio thread, plays CurBlock from
// PlayPosition and frees it once the next block is loaded; if it is not, it
// plays the block again. The loader sleeps (VQAERR_SLEEPING) while the block
// it would overwrite is still unplayed. Code on the main thread holds the
// SDL device lock while changing what the callback reads.
struct VQAAudio {
  // The ring, when the player allocated it.
  std::vector<unsigned char> BufferStorage;
  // One flag per ring block: 1 = holds unplayed sound, 0 = free to fill.
  std::vector<int16_t> IsLoadedStorage;
  // Staging for one frame's decompressed sound, TempBufSize bytes.
  std::vector<unsigned char> TempBufStorage;
  std::span<unsigned char> Buffer;  // BufferStorage or caller-owned span
  // Byte offset in the ring where the loader writes next.
  int32_t AudBufPos = 0;
  int16_t* IsLoaded = nullptr;  // Points into IsLoadedStorage
  int32_t NumAudBlocks = 0;
  // The block being played, and the one after it (callback scratch).
  int32_t CurBlock = 0;
  int32_t NextBlock = 0;
  unsigned char* TempBuf = nullptr;  // Points into TempBufStorage
  // Bytes in TempBuf waiting for CopyAudio(), 0 when it is empty.
  int32_t TempBufLen = 0;
  int32_t TempBufSize = 0;
  uint32_t Flags = 0;  // VQAAUDF_* bits
  // Byte offset of CurBlock in the ring.
  int32_t PlayPosition = 0;
  // Never set, so VQA_GetStats() always reports 0.
  int64_t SamplesPlayed = 0;
  // Times the callback found the next block empty and replayed the current
  // one. Never read.
  int32_t NumSkipped = 0;
  // Format of the track being played, the primary or the alternate one.
  uint16_t SampleRate = 0;
  unsigned char Channels = 0;
  unsigned char BitsPerSample = 0;  // 8 or 16
  int32_t BytesPerSec = 0;
  // Decoder state for SND2 (IMA ADPCM) chunks, carried from chunk to chunk.
  SosCompressInfo ADPCM_Info = {};
  // Blocks handed to SDL since the sound started (a replayed block counts
  // only after the movie has loaded completely). VQA_GetTime() derives the
  // movie clock from it.
  int ChunksMovedToAudioBuffer = 0;
};

// Audio flags. The two-bit fields hold an HMI_* state; DIGIINIT is set while
// the SDL stream and callback are installed. The rest come from the DOS
// library: TIMERINIT is only ever cleared, HMITIMER is set only by
// VQA_StartTimerInt(), which nothing calls, and the page locks are unused.
#define VQAAUDB_DIGIINIT 0    // Sound output initialized (2 bits)
#define VQAAUDB_TIMERINIT 2   // HMI timer system initialized (2 bits)
#define VQAAUDB_HMITIMER 4    // HMI timer callback initialized (2 bits)
#define VQAAUDB_ISPLAYING 6   // The callback is playing the ring.
#define VQAAUDB_MEMLOCKED 30  // Audio memory page locked.
#define VQAAUDB_MODLOCKED 31  // Audio module page locked.

#define VQAAUDF_DIGIINIT (3U << VQAAUDB_DIGIINIT)
#define VQAAUDF_TIMERINIT (3U << VQAAUDB_TIMERINIT)
#define VQAAUDF_HMITIMER (3U << VQAAUDB_HMITIMER)
#define VQAAUDF_ISPLAYING (1U << VQAAUDB_ISPLAYING)
#define VQAAUDF_MEMLOCKED (1U << VQAAUDB_MEMLOCKED)
#define VQAAUDF_MODLOCKED (1U << VQAAUDB_MODLOCKED)

// States of the two-bit audio flag fields.
#define HMI_UNINIT 0U   // Not initialized
#define HMI_VQAINIT 1U  // Initialized by the player
#define HMI_APPINIT 2U  // Initialized by the application

// VQAData: everything a movie needs while it is open. Allocated by VQA_Open()
// once the header is read and freed by VQA_Close().
struct VQAData {
  // Draws the drawer's next frame if it is due; DrawFrame_Buffer, the only
  // draw routine left.
  int32_t (*Draw_Frame)(VQAHandle* vqa) = nullptr;

  // Decodes a frame into the image buffer: the routine for the movie's block
  // size, or one that does nothing when VQACFGF_BUFFER is clear or the block
  // size has no routine.
  void (*UnVQ)(std::span<const unsigned char> codebook,
               std::span<const unsigned char> pointers,
               std::span<unsigned char> buffer, int blocksperrow, int numrows,
               int bufwidth) = nullptr;

  // RAII storage for nodes - these vectors own the node objects
  std::vector<std::unique_ptr<VQACBNode>> CBNodes;
  std::vector<std::unique_ptr<VQAFrameNode>> FrameNodes;

  // The image buffer, when the player allocated it.
  std::vector<unsigned char> ImageBufStorage;
  // One FINF entry per header frame: 4 flag bits on top of the halved file
  // offset (see FrameHasPalette and FrameByteOffset in vqa_format.h).
  std::vector<uint32_t> FoffStorage;

  VQAFrameNode* FrameData = nullptr;  // Points to first node in FrameNodes
  VQACBNode* CBData = nullptr;        // Points to first node in CBNodes
  VQAAudio Audio;
  VQALoader Loader{};
  VQADrawer Drawer{};
  VQAFlipper Flipper{};
  uint32_t Flags = 0;        // VQADATF_* bits
  uint32_t* Foff = nullptr;  // Points into FoffStorage
  // Copy of VQAConfig::VBIBit. Unused.
  int32_t VBIBit = 0;
  // Buffer sizes in bytes for one codebook, palette and set of vector
  // pointers, computed from the header with slack for compressed data loaded
  // at the end of the buffer.
  int32_t Max_CB_Size = 0;
  int32_t Max_Pal_Size = 0;
  int32_t Max_Ptr_Size = 0;
  // Frames loaded and drawn so far, for VQA_GetStats().
  int32_t LoadedFrames = 0;
  int32_t DrawnFrames = 0;
  // Clock readings (VQA_TIMETICKS) when playback started and when it ended
  // or was last paused. EndTime is where a resumed movie restarts the clock.
  int64_t StartTime = 0;
  int64_t EndTime = 0;
  // Bytes allocated for the movie, for VQA_GetStats().
  int32_t MemUsed = 0;
};

// VQAData flags.

// A drawn frame waits for User_Update() to release it; no other frame is drawn
// until then.
#define VQADATB_UPDATE 0
// The drawer has a frame ready and is waiting on VQADATF_UPDATE, so it does not
// select another.
#define VQADATB_DSLEEP 1
// The loader stopped inside a sound chunk until the audio ring has room; see
// CurChunkHdr.
#define VQADATB_LSLEEP 2
#define VQADATB_DDONE 3  // The drawer has finished. Set when done.
#define VQADATB_LDONE 4  // The loader has finished. Set when done.
// VQA_Play() has configured the drawer and started the clock and sound.
#define VQADATB_PRIMED 5
#define VQADATB_PAUSED 6  // The player is paused.
#define VQADATF_UPDATE (1U << VQADATB_UPDATE)
#define VQADATF_DSLEEP (1U << VQADATB_DSLEEP)
#define VQADATF_LSLEEP (1U << VQADATB_LSLEEP)
#define VQADATF_DDONE (1U << VQADATB_DDONE)
#define VQADATF_LDONE (1U << VQADATB_LDONE)
#define VQADATF_PRIMED (1U << VQADATB_PRIMED)
#define VQADATF_PAUSED (1U << VQADATB_PAUSED)

// VQAHandle: the player state behind VqaPlayer, which owns one; tests use one
// directly to reach the internals. It outlives the movies opened on it.
struct VQAHandle {
  VQAHandle() = default;
  ~VQAHandle() = default;
  VQAHandle(const VQAHandle&) = delete;
  VQAHandle& operator=(const VQAHandle&) = delete;
  VQAHandle(VQAHandle&&) = delete;
  VQAHandle& operator=(VQAHandle&&) = delete;

  // Clears playback state so the handle can be reopened. io is intentionally
  // preserved across reset.
  void Reset() {
    data = nullptr;
    config = {};
    header = {};
  }

  // The file source installed by VqaPlayer::SetIo(). Not owned.
  VqaIo* io = nullptr;
  // The open movie's buffers, owned: allocated by VQA_Open() and deleted by
  // VQA_Close(). nullptr while no movie is open.
  VQAData* data = nullptr;
  // The copy of the caller's configuration, with the -1 defaults resolved
  // from the header.
  VQAConfig config{};
  // The open movie's VQHD header. SetStop() lowers frame_count.
  VqaHeader header{};
};

// The player entry points behind VqaPlayer's methods of the same names; see
// vqaplay.h for what they do. VQA_Open() and VQA_Close() are also the
// allocation and release of vqa->data.
int32_t VQA_Open(VQAHandle* vqa, const char* filename, VQAConfig* config);
void VQA_Close(VQAHandle* vqa);
int32_t VQA_Play(VQAHandle* vqa, int32_t mode);
int32_t VQA_SeekFrame(VQAHandle* vqa, int32_t frame, int32_t fromwhere);
int64_t VQA_SetStop(VQAHandle* vqa, int64_t stop);
void VQA_GetInfo(VQAHandle* vqa, VQAInfo* info);
void VQA_GetStats(const VQAHandle* vqa, VQAStatistics* stats);

// Loads the next frame into the loader's frame buffer, collecting its
// codebook and sound on the way. Returns 0 when a frame was loaded, or
// VQAERR_NOBUFFER (no free buffer), VQAERR_SLEEPING (waiting on the audio
// ring; call again to resume), VQAERR_EOF, or a read or seek error.
int32_t VQA_LoadFrame(VQAHandle* vqa);

// Places the image in the image buffer from config.X1/Y1 and the origin
// flags, and picks the decoder for the movie's block size. Runs once, when
// playback starts.
void VQA_Configure_Drawer(VQAHandle* vqap);

// The page flip: once the drawer has drawn a frame (VQADATF_UPDATE), frees
// that frame's buffer for the loader. Always returns 0.
int64_t User_Update(const VQAHandle* vqa);

// The DOS timer interrupt. VQA_StartTimerInt() and VQA_TimerMethod() have no
// callers; VQA_StopTimerInt() only runs at close.
int32_t VQA_StartTimerInt(const VQAHandle* vqap, int32_t init);
void VQA_StopTimerInt(VQAHandle* vqap);

// Sets the movie clock to time (VQA_TIMETICKS), using the VQA_TMETHOD_*
// method, or the best one available, as its source.
void VQA_SetTimer(VQAHandle* vqap, int64_t time, int method);
// Returns the movie clock in VQA_TIMETICKS.
int64_t VQA_GetTime(VQAHandle* vqap);
int32_t VQA_TimerMethod();

// Sound output. VQA_OpenAudio() installs the SDL stream and callback,
// VQA_StartAudio() and VQA_StopAudio() start and stop the callback playing
// the ring, VQA_CloseAudio() removes it all. CopyAudio() moves the staged
// sound into the ring, or returns VQAERR_SLEEPING if the ring has no room.
int32_t VQA_OpenAudio(VQAHandle* vqap);
void VQA_CloseAudio(VQAHandle* vqap);
int32_t VQA_StartAudio(VQAHandle* vqap);
void VQA_StopAudio(const VQAHandle* vqap);
int32_t CopyAudio(VQAHandle* vqap);

// Nonzero once the loader has read the whole movie; from then on the audio
// callback counts a replayed block towards the clock, so the last frames
// still come due after the sound runs out. Written by VQA_Open() and
// VQA_Play() on the main thread and read on the audio thread, unsynchronized.
extern int VQAMovieDone;

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQAPLAYP_H_
