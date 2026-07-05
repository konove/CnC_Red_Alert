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
/****************************************************************************
 *
 *         C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
 *
 *----------------------------------------------------------------------------
 *
 * PROJECT
 *     VQA player library. (32-Bit protected mode)
 *
 * FILE
 *     vqaplayp.h
 *
 * DESCRIPTION
 *     VQAPlay private library definitions.
 *
 * PROGRAMMER
 *     Denzil E. Long, Jr.
 *     Bill Randolph
 *
 * DATE
 *     August 21, 1995
 *
 ****************************************************************************/
#include <cstdint>
#include <memory>
#include <vector>

#include "winvq/vqa32/vqafile.h"
#include "winvq/vqa32/vqaio.h"
#include "winvq/vqa32/vqaplay.h"
#include "winvq/vqm32/soscomp.h"
#include "winvq/vqm32/video.h"

/*---------------------------------------------------------------------------
 * GENERAL CONSTANT DEFINITIONS
 *-------------------------------------------------------------------------*/

/* Internal library version. */
#define VQA_VERSION "2.42"
#define VQA_DATE __DATE__ " " __TIME__

#define VQA_IDSTRING "VQA32 " VQA_VERSION " (" VQA_DATE ")"
#define VQA_REQUIRES "VQM32 2.12 or better."

/* Block dimensions macro and identifiers. */
#define BLOCK_DIM(a, b) (((a & 0xFF) << 8) | (b & 0xFF))
#define BLOCK_2X2 BLOCK_DIM(2, 2)
#define BLOCK_2X3 BLOCK_DIM(2, 3)
#define BLOCK_4X2 BLOCK_DIM(4, 2)
#define BLOCK_4X4 BLOCK_DIM(4, 4)

/* Memory limits */
#define VQA_MAX_CBBUFS 10    /* Maximum number of codebook buffers */
#define VQA_MAX_FRAMEBUFS 30 /* Maximum number of frame buffers */

/* Special Constants */
#define VQA_MASK_POINTER 0x8000 /* Pointer value to use for masking. */

/*---------------------------------------------------------------------------
 * STRUCTURES AND RELATED DEFINITIONS
 *-------------------------------------------------------------------------*/

/* ChunkHeader: IFF chunk identifier header.
 *
 * id   - 4 Byte chunk id.
 * size - Size of chunk.
 */
typedef struct _ChunkHeader {
  uint32_t id;
  uint32_t size;
} ChunkHeader;

/* ZAPHeader: ZAP audio compression header. NOTE: If the uncompressed size
 *            and the compressed size are equal then the audio frame is RAW
 *            (NOT COMPRESSED).
 *
 * UnCompSize - Uncompressed size in bytes.
 * CompSize   - Compressed size in bytes.
 */
typedef struct _ZAPHeader {
  unsigned short UnCompSize;
  unsigned short CompSize;
} ZAPHeader;

/* VQACBNode: A circular list of codebook buffers, used by the load task.
 *            If the data is compressed, it is loaded into the end of the
 *            buffer and the compression flags is set. Otherwise the data
 *            is loaded into the start of the buffer.
 *            (Make sure this structure's size is always DWORD aligned.)
 *
 * Buffer   - Codebook data buffer.
 * Next     - Pointer to next VQACBNode in the codebook list.
 * Flags    - Used by the drawer to tell if certain operations have been
 *            performed on this codebook, such as downloading to VRAM,
 *            or pre-scaling it. This field is cleared by the Loader when a
 *            new codebook is loaded.
 * CBOffset - Offset into the buffer of the compressed data.
 */
struct VQACBNode {
  std::vector<unsigned char> BufferStorage;
  unsigned char* Buffer =
      nullptr;  // Points into BufferStorage for compatibility
  VQACBNode* Next = nullptr;
  unsigned long Flags = 0;
  unsigned long CBOffset = 0;
};

/* VQACBNode flags */
#define VQACBB_DOWNLOADED 0 /* Download codebook to VRAM (XMODE VRAM) */
#define VQACBB_CBCOMP 1     /* Codebook is compressed */
#define VQACBF_DOWNLOADED (1 << VQACBB_DOWNLOADED)
#define VQACBF_CBCOMP (1 << VQACBB_CBCOMP)

/* VQAFrameNode: A circular list of frame buffers, filled in by the load
 *               task. If the data is compressed, it is loaded into the end
 *               of the buffer and the compress flag is set. Otherwise, it's
 *               loaded into the start of the buffer.
 *               (Make sure this structure's size is always DWORD aligned.)
 *
 * Pointers    - The vector pointer data buffer.
 * Codebook    - Pointer to VQACBNode list entry for this frame.
 * Palette     - The array of palette colors (R,G,B).
 * Next        - Pointer to the next entry in the Frame Buffer List.
 * Flags       - Inter-process communication flags for this frame (see below)
 *               set by Loader, cleared by Flipper.
 * FrameNum    - Number of this frame in the animation.
 * PtrOffset   - Offset into buffer of the compressed vector pointer data.
 * PalOffset   - Offset into buffer of the compressed palette data.
 * PaletteSize - Size of the palette for this frame (in bytes).
 */
struct VQAFrameNode {
  std::vector<unsigned char> PointersStorage;
  std::vector<unsigned char> PaletteStorage;
  unsigned char* Pointers = nullptr;  // Points into PointersStorage
  VQACBNode* Codebook = nullptr;
  unsigned char* Palette = nullptr;  // Points into PaletteStorage
  VQAFrameNode* Next = nullptr;
  unsigned long Flags = 0;
  long FrameNum = 0;
  long PtrOffset = 0;
  long PalOffset = 0;
  long PaletteSize = 0;
};

/* FrameNode flags */
#define VQAFRMB_LOADED 0  /* Frame loaded */
#define VQAFRMB_KEY 1     /* Key Frame (must be drawn) */
#define VQAFRMB_PALETTE 2 /* Palette needs set */
#define VQAFRMB_PALCOMP 3 /* Palette is compressed */
#define VQAFRMB_PTRCOMP 4 /* Vector pointer data is compressed */
#define VQAFRMF_LOADED (1 << VQAFRMB_LOADED)
#define VQAFRMF_KEY (1 << VQAFRMB_KEY)
#define VQAFRMF_PALETTE (1 << VQAFRMB_PALETTE)
#define VQAFRMF_PALCOMP (1 << VQAFRMB_PALCOMP)
#define VQAFRMF_PTRCOMP (1 << VQAFRMB_PTRCOMP)

/* VQALoader: Data needed exclusively by the Loader.
 *            (Make sure this structure's size is always DWORD aligned.)
 *
 * CurCB         - Pointer to the current codebook node to load data into.
 * FullCB        - Pointer to the last fully-loaded codebook node.
 * CurFrame      - Pointer to the current frame node to load data into.
 * NumPartialCB  - Number of partial codebooks accumulated.
 * PartialCBSize - Size of partial codebook (LCW'd or not), in bytes
 * CurFrameNum   - The number of the frame being loaded by the Loader.
 * LastCBFrame   - Last frame in the animation that contains a partial CB
 * LastFrameNum  - Number of the last loaded frame
 * WaitsOnDrawer - Number of wait states Loader hits waiting on the Drawer
 * WaitsOnAudio  - Number of wait states Loader hits waiting on HMI
 * FrameSize     - Size of the last frame in bytes.
 * MaxFrameSize  - Size of the largest frame in the animation.
 * CurChunkHdr   - Chunk header of the chunk currently being processed.
 */
typedef struct _VQALoader {
  VQACBNode* CurCB;
  VQACBNode* FullCB;
  VQAFrameNode* CurFrame;
  long NumPartialCB;
  long PartialCBSize;
  long CurFrameNum;
  long LastCBFrame;
  long LastFrameNum;
  long WaitsOnDrawer;
  long WaitsOnAudio;
  long FrameSize;
  long MaxFrameSize;
  ChunkHeader CurChunkHdr;
} VQALoader;

/* VQADrawer: Data needed exclusively by the Drawer.
 *            (Make sure this structure's size is always DWORD aligned.)
 *
 * CurFrame       - Pointer to the current frame to draw.
 * Flags          - Flags for the draw routines (IE: VQADRWF_SETPAL)
 * Display        - Pointer to DisplayInfo structure for active video mode.
 * ImageBuf       - Buffer to un-vq into, must be DWORD aligned.
 * ImageWidth     - Width of Image buffer (in pixels).
 * ImageHeight    - Height of Image buffer (in pixels).
 * X1,Y1,X2,Y2    - Coordinates of image corners (in pixels).
 * ScreenOffset   - Offset into screen memory, for centering small images.
 * CurPalSize     - Size of the current palette in bytes.
 * Palette_24     - Copy of the last-loaded palette
 * Palette_15     - 15-bit version of Palette_24, for 32K-color modes
 * BlocksPerRow   - # of VQ blocks per row for this resolution/block width.
 * NumRows        - # of rows of VQ blocks for this resolution/block height.
 * NumBlocks      - Total number of blocks in the image.
 * MaskStart      - Pointer index of start of mask rectangle.
 * MaskWidth      - Width of mask rectangle, in blocks.
 * MaskHeight     - Height of mask rectangle, in blocks.
 * LastTime       - The time when that last frame was drawn.
 * LastFrame      - The number of the last frame selected.
 * LastFrameNum   - Number of the last frame drawn.
 * DesiredFrame   - The number of the frame that should be drawn.
 * NumSkipped     - Number of frames skipped.
 * WaitsOnFlipper - Number of wait states Drawer hits waiting on the Flipper.
 * WaitsOnLoader  - Number of wait states Drawer hits waiting on the Loader.
 */
typedef struct _VQADrawer {
  VQAFrameNode* CurFrame;
  unsigned long Flags;
  DisplayInfo* Display;
  unsigned char* ImageBuf;
  long ImageWidth;
  long ImageHeight;
  long X1, Y1, X2, Y2;
  long ScreenOffset;
  long CurPalSize;
  unsigned char Palette_24[768];
  unsigned char Palette_15[512];
  long BlocksPerRow;
  long NumRows;
  long NumBlocks;
  long MaskStart;
  long MaskWidth;
  long MaskHeight;
  long LastTime;
  long LastFrame;
  long LastFrameNum;
  long DesiredFrame;
  long NumSkipped;
  long WaitsOnFlipper;
  long WaitsOnLoader;
} VQADrawer;

/* Drawer flags */
#define VQADRWB_SETPAL 0 /* Set palette */
#define VQADRWF_SETPAL (1 << VQADRWB_SETPAL)

/* VQAFlipper: Data needed exclusively by the page-flipper.
 *             (Make sure this structure's size is always DWORD aligned.)
 *
 * CurFrame     - Pointer to current flipper frame.
 * LastFrameNum - Number of last flipped frame
 * pad          - DWORD alignment padding.
 */
typedef struct _VQAFlipper {
  VQAFrameNode* CurFrame;
  long LastFrameNum;
} VQAFlipper;

/* VQAAudio: Data needed exclusively by audio playback.
 *           (Make sure this structure's size is always DWORD aligned.)
 *
 * Buffer         - Pointer to the audio buffer.
 * AudBufPos      - Current audio buffer position, for copying data in buffer.
 * IsLoaded       - Inter-process communication flag:
 *                  0 = is loadable, 1 = is not. Loader sets it when it
 *                  loads, audio callback clears it when it plays one.
 * NumAudBlocks   - Number of HMI blocks in the audio buffer.
 * CurBlock       - Current audio block
 * NextBlock      - Next audio block
 * TempBuf        - Pointer to temp buffer for loading/decompressing audio
 *                  data.
 * TempBufLen     - Number of bytes loaded into temp buffer.
 * TempBufSize    - Size of temp buffer in bytes.
 * Flags          - Various audio flags. (See below)
 * PlayPosition   - HMI's current buffer position.
 * SamplesPlayed  - Total samples played.
 * NumSkipped     - Count of buffers missed.
 * SampleRate     - Recorded sampling rate of the track.
 * Channels       - Number of channels in the track.
 * BitsPerSample  - Bit resolution size of sample (8,16)
 * BytesPerSec    - Recorded data transfer for one second.
 * ADPCM_Info     - ADPCM decompression information structure.
 * DigiHandle     - HMI digital device handle.
 * SampleHandle   - HMI sample handle.
 * DigiTimer      - HMI digital fill handler timer handle.
 * sSOSSampleData - HMI sample structure.
 * DigiCaps       - HMI sound card digital capabilities.
 * DigiHardware   - HMI sound card digital hardware settings.
 * sSOSInitDriver - HMI driver initialization structure.
 * TimerHandle		- Handle to Windows multi-media timer
 * SoundTimerHandle- Handle to extra Windows mm timer req. for direct sound
 * SecondaryBufferPtr - Pointer to out direct sound secondary buffer
 * DSBuffFormat		- WAVEFORMATEX structure for direct sound
 * BufferDesc		- Description structure for setting up direct sound
 * buffers SecondaryBufferSize - length in bytes of our secondary sound buffer
 * EndLastAudioChunk - Offset into secondary buffer of the end of the last chunk
 * of audio copied in ChunksMovedToAudioBuffer - Total number of HMIBufSize
 * chunks moved into the secondary buffer LastChunkPosition - Offset position of
 * last chunk copied to secondary buffer CreatedSoundObject - True if we had to
 * create our own direct sound object CreatedSoundBuffer - True if we had to
 * create out own direct sound primary buffer
 */
struct VQAAudio {
  std::vector<unsigned char> BufferStorage;
  std::vector<short> IsLoadedStorage;
  std::vector<unsigned char> TempBufStorage;
  unsigned char* Buffer = nullptr;  // Points into BufferStorage
  unsigned long AudBufPos = 0;
  short* IsLoaded = nullptr;  // Points into IsLoadedStorage
  unsigned long NumAudBlocks = 0;
  unsigned long CurBlock = 0;
  unsigned long NextBlock = 0;
  unsigned char* TempBuf = nullptr;  // Points into TempBufStorage
  unsigned long TempBufLen = 0;
  unsigned long TempBufSize = 0;
  unsigned long Flags = 0;
  unsigned long PlayPosition = 0;
  unsigned long SamplesPlayed = 0;
  unsigned long NumSkipped = 0;
  unsigned short SampleRate = 0;
  unsigned char Channels = 0;
  unsigned char BitsPerSample = 0;
  unsigned long BytesPerSec = 0;
  SosCompressInfo ADPCM_Info = {};
  unsigned ChunksMovedToAudioBuffer = 0;
};

/* Audio flags. */
#define VQAAUDB_DIGIINIT 0   /* HMI digital driver initialized (2 bits) */
#define VQAAUDB_TIMERINIT 2  /* HMI timer system initialized (2 bits) */
#define VQAAUDB_HMITIMER 4   /* HMI timer callback initialized (2 bits) */
#define VQAAUDB_ISPLAYING 6  /* Audio playing flag. */
#define VQAAUDB_MEMLOCKED 30 /* Audio memory page locked. */
#define VQAAUDB_MODLOCKED 31 /* Audio module page locked. */

#define VQAAUDF_DIGIINIT (3 << VQAAUDB_DIGIINIT)
#define VQAAUDF_TIMERINIT (3 << VQAAUDB_TIMERINIT)
#define VQAAUDF_HMITIMER (3 << VQAAUDB_HMITIMER)
#define VQAAUDF_ISPLAYING (1 << VQAAUDB_ISPLAYING)
#define VQAAUDF_MEMLOCKED (1 << VQAAUDB_MEMLOCKED)
#define VQAAUDF_MODLOCKED (1 << VQAAUDB_MODLOCKED)

/* HMI device initialization conditions. (DIGIINIT, TIMERINIT, HMITIMER) */
#define HMI_UNINIT 0  /* Unitialize state. */
#define HMI_VQAINIT 1 /* VQA initialized */
#define HMI_APPINIT 2 /* Application initialized */

/* VQAData: This stucture contains all the data used for playing a VQA.
 *
 * Draw_Frame   - Pointer to the draw-frame routine for this video mode.
 * Page_Flip    - Pointer to the page flip function for this video mode.
 * UnVQ         - Pointer to the UnVQ routine for this vid mode & blk size.
 * FrameData    - Frame buffer circular list head.
 * CBData       - Codebook circular list head.
 * Audio        - Audio buffer
 * Loader       - Loader's data
 * Drawer       - Drawer's data
 * Flipper      - Flipper's data
 * Flags        - Flags used by the player.
 * Foff         - Pointer to frame offset table.
 * VBIBit       - Vertical blank bit polarity.
 * Max_CB_Size  - Maximum size of an uncompressed codebook
 * Max_Pal_Size - Maximum size of an uncompressed palette
 * Max_Ptr_Size - Maximum size of uncompressed pointer data
 * LoadedFrames - Number of frames loaded
 * DrawnFrames  - Number of frames drawn
 * StartTime    - Start time in VQA time ticks
 * EndTime      - Stop time in VQA time ticks
 * MemUsed      - Number of bytes allocated by VQA_AllocBuffers
 */
struct VQAData {
  long (*Draw_Frame)(VQAHandle* vqa) = nullptr;

  void __cdecl (*UnVQ)(const unsigned char* codebook,
                       const unsigned char* pointers, unsigned char* buffer,
                       unsigned long blocksperrow, unsigned long numrows,
                       unsigned long bufwidth) = nullptr;

  // RAII storage for nodes - these vectors own the node objects
  std::vector<std::unique_ptr<VQACBNode>> CBNodes;
  std::vector<std::unique_ptr<VQAFrameNode>> FrameNodes;

  // RAII storage for buffers
  std::vector<unsigned char> ImageBufStorage;
  std::vector<long> FoffStorage;

  VQAFrameNode* FrameData = nullptr;  // Points to first node in FrameNodes
  VQACBNode* CBData = nullptr;        // Points to first node in CBNodes
  VQAAudio Audio;
  VQALoader Loader;
  VQADrawer Drawer;
  VQAFlipper Flipper;
  unsigned long Flags = 0;
  long* Foff = nullptr;  // Points into FoffStorage
  long VBIBit = 0;
  long Max_CB_Size = 0;
  long Max_Pal_Size = 0;
  long Max_Ptr_Size = 0;
  long LoadedFrames = 0;
  long DrawnFrames = 0;
  long StartTime = 0;
  long EndTime = 0;
  long MemUsed = 0;
};

/* VQAData flags */
#define VQADATB_UPDATE 0 /* Update the display. */
#define VQADATB_DSLEEP 1 /* Drawer sleep state. */
#define VQADATB_LSLEEP 2 /* Loader sleep state. */
#define VQADATB_DDONE 3  /* Drawer done flag. (0 = done) */
#define VQADATB_LDONE 4  /* Loader done flag. (0 = done) */
#define VQADATB_PRIMED 5 /* Buffers are primed. */
#define VQADATB_PAUSED 6 /* The player is paused. */
#define VQADATF_UPDATE (1 << VQADATB_UPDATE)
#define VQADATF_DSLEEP (1 << VQADATB_DSLEEP)
#define VQADATF_LSLEEP (1 << VQADATB_LSLEEP)
#define VQADATF_DDONE (1 << VQADATB_DDONE)
#define VQADATF_LDONE (1 << VQADATB_LDONE)
#define VQADATF_PRIMED (1 << VQADATB_PRIMED)
#define VQADATF_PAUSED (1 << VQADATB_PAUSED)

/* VQAHandle: Full definition of the player state behind the opaque handle
 *            declared in vqaplay.h. Allocated by VQA_Alloc() and freed by
 *            VQA_Free(); clients only ever hold a pointer.
 *
 * io     - File source installed via VQA_SetIo(). Not owned.
 * data   - Pointer to internal data buffers.
 * config - Configuration structure.
 * header - VQA header structure.
 */
struct VQAHandle {
  VQAHandle() = default;
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

  VqaIo* io = nullptr;
  VQAData* data = nullptr;
  VQAConfig config{};
  VQAHeader header{};
};

/*---------------------------------------------------------------------------
 * FUNCTION PROTOTYPES
 *-------------------------------------------------------------------------*/

/* Loader/Drawer system. */
long VQA_LoadFrame(VQAHandle* vqa);
void VQA_Configure_Drawer(VQAHandle* vqap);
int64_t User_Update(VQAHandle* vqa);

/* Timer system. */
long VQA_StartTimerInt(VQAHandle* vqap, long init);
void VQA_StopTimerInt(VQAHandle* vqap);
void VQA_SetTimer(VQAHandle* vqap, long time, long method);
int64_t VQA_GetTime(VQAHandle* vqap);
long VQA_TimerMethod();

/* Audio system. */
long VQA_OpenAudio(VQAHandle* vqap, void* window);
void VQA_CloseAudio(VQAHandle* vqap);
long VQA_StartAudio(VQAHandle* vqap);
void VQA_StopAudio(VQAHandle* vqap);
long CopyAudio(VQAHandle* vqap);

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQAPLAYP_H_
