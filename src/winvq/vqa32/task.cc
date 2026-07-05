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

/****************************************************************************
 *
 *        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
 *
 *----------------------------------------------------------------------------
 *
 * PROJECT
 *     VQA player library. (32-Bit protected mode)
 *
 * FILE
 *     task.c
 *
 * DESCRIPTION
 *     Loading and drawing delegation
 *
 * PROGRAMMER
 *     Bill Randolph
 *     Denzil E. Long, Jr.
 *
 * DATE
 *     July 25, 1995
 *
 *----------------------------------------------------------------------------
 *
 * PUBLIC
 *     VqaPlayer    - RAII facade over the player entry points.
 *
 * PRIVATE
 *     VQA_Play     - Play the VQA movie.
 *     VQA_SetStop  - Set the frame the player should stop on.
 *     VQA_GetInfo  - Get VQA movie information.
 *     VQA_GetStats - Get VQA movie statistics.
 *     User_Update  - Page flip routine called by the task interrupt.
 *
 ****************************************************************************/

#include <cstdint>

#include "winvq/vqa32/vqafile.h"
#include "winvq/vqa32/vqaplay.h"
#include "winvq/vqa32/vqaplayp.h"
#include "winvq/vqm32/font.h"

/*---------------------------------------------------------------------------
 * PRIVATE DECLARATIONS
 *-------------------------------------------------------------------------*/

/* Externals */
extern "C" {
extern int __cdecl Check_Key();
extern int __cdecl Get_Key();
}

/* VqaPlayer: thin RAII facade over the internal player entry points. */

VqaPlayer::VqaPlayer() : impl_(std::make_unique<VQAHandle>()) {}

VqaPlayer::~VqaPlayer() {
  /* Only an open movie needs shutdown; Close() on a never-opened player
   * would touch the (possibly absent) io object.
   */
  if (impl_->data != nullptr) {
    Close();
  }
}

void VqaPlayer::SetIo(VqaIo* io) { impl_->io = io; }

int VqaPlayer::Open(const char* filename, VQAConfig* config) {
  return static_cast<int>(VQA_Open(impl_.get(), filename, config));
}

void VqaPlayer::Close() { VQA_Close(impl_.get()); }

int VqaPlayer::Play(int mode) {
  return static_cast<int>(VQA_Play(impl_.get(), mode));
}

int VqaPlayer::SeekFrame(int frame, int fromwhere) {
  return static_cast<int>(VQA_SeekFrame(impl_.get(), frame, fromwhere));
}

int VqaPlayer::SetStop(int frame) {
  return static_cast<int>(VQA_SetStop(impl_.get(), frame));
}

void VqaPlayer::GetInfo(VQAInfo* info) const { VQA_GetInfo(impl_.get(), info); }

void VqaPlayer::GetStats(VQAStatistics* stats) const {
  VQA_GetStats(impl_.get(), stats);
}

int VQAMovieDone;
/****************************************************************************
 *
 * NAME
 *     VQA_Play - Play the VQA movie.
 *
 * SYNOPSIS
 *     Error = VQA_Play(VQA, Mode)
 *
 *     long VQA_Play(VQAHandle *, long);
 *
 * FUNCTION
 *     Playback the movie associated with the specified VQAHandle.
 *
 * INPUTS
 *     VQA  - Pointer to handle of movie to play.
 *     Mode - Playback mode.
 *              VQAMODE_RUN   - Run the movie until completion.
 *              VQAMODE_WALK  - Walk the movie frame by frame.
 *              VQAMODE_PAUSE - Pause the movie.
 *              VQAMODE_STOP  - Stop the movie (Shutdown).
 *
 * RESULT
 *     Error - 0 if successful, or error code.
 *
 ****************************************************************************/

long VQA_Play(VQAHandle* vqa, long mode) {
  VQAData* vqabuf;
  VQAConfig* config;
  VQADrawer* drawer;
  int64_t rc = 0;

#ifdef _WIN32
  /*
  ** Save the process priority level then bump it up
  */
  DWORD process_priority = GetPriorityClass(GetCurrentProcess());
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif  // _WIN32

  /* Dereference commonly used data members for quick access. */
  vqabuf = vqa->data;
  drawer = &vqabuf->Drawer;
  config = &vqa->config;

  /* One time player priming. */
  if ((vqabuf->Flags & VQADATF_PRIMED) == 0) {
    /* Init the Drawer's configuration */
    VQA_Configure_Drawer(vqa);

    /* If audio enabled & loaded, start playing */
    if ((config->OptionFlags & VQAOPTF_AUDIO) != 0 &&
        vqabuf->Audio.IsLoaded[0] != 0) {
      VQA_StartAudio(vqa);
    }

    /* Initialize the timer */
    auto i =
        vqabuf->Drawer.CurFrame->FrameNum * VQA_TIMETICKS / config->DrawRate;

    VQA_SetTimer(vqa, i, config->TimerMethod);
    vqabuf->StartTime = VQA_GetTime(vqa);

    /* Priming is complete. */
    vqabuf->Flags |= VQADATF_PRIMED;
  }

  /* Main Player Loop */
  switch (mode) {
    case VQAMODE_PAUSE:
      if ((vqabuf->Flags & VQADATF_PAUSED) == 0) {
        vqabuf->Flags |= VQADATF_PAUSED;
        vqabuf->EndTime = VQA_GetTime(vqa);

        /* Stop the audio while the movie is paused. */
        if ((vqabuf->Audio.Flags & VQAAUDF_ISPLAYING) != 0) {
          VQA_StopAudio(vqa);
        }
      }

      rc = VQAERR_PAUSED;
      break;

    case VQAMODE_RUN:
    case VQAMODE_WALK:
    default:

      /* Start up the movie if is it currently paused. */
      if ((vqabuf->Flags & VQADATF_PAUSED) != 0) {
        vqabuf->Flags &= ~VQADATF_PAUSED;

        /* Start the audio if it was previously on. */
        if ((config->OptionFlags & VQAOPTF_AUDIO) != 0) {
          if (VQA_StartAudio(vqa) != 0) {
            /* Stop audio, if it's playing. */
            VQA_StopAudio(vqa);
#ifdef _WIN32
            /*
            ** Restore the process priority level
            */
            SetPriorityClass(GetCurrentProcess(), process_priority);
#endif  // _WIN32
            return VQAERR_EOF;
          }
        }

        VQA_SetTimer(vqa, vqabuf->EndTime, config->TimerMethod);
      }

      /* Load, Draw, Load, Draw, Load, Draw ... */
      while ((vqabuf->Flags & (VQADATF_DDONE | VQADATF_LDONE)) !=
             (VQADATF_DDONE | VQADATF_LDONE)) {
        /* Load a frame */
        if ((vqabuf->Flags & VQADATF_LDONE) == 0) {
          rc = VQA_LoadFrame(vqa);
          if (rc == 0) {
            vqabuf->LoadedFrames++;
          } else {
            if (rc != VQAERR_NOBUFFER && rc != VQAERR_SLEEPING) {
              vqabuf->Flags |= VQADATF_LDONE;
              rc = 0;
            }
          }
        } else {
          VQAMovieDone++;
        }

        /* Draw a frame */
        if ((config->DrawFlags & VQACFGF_NODRAW) == 0) {
          rc = (*vqabuf->Draw_Frame)(vqa);
          if (rc == 0) {
            vqabuf->DrawnFrames++;
            rc = vqabuf->Drawer.LastFrameNum;
            if (User_Update(vqa) != 0) {
              vqabuf->Flags |= VQADATF_DDONE | VQADATF_LDONE;
            }
          } else {
            if (rc == VQAERR_EOF) {
              break;
            }
            if ((vqabuf->Flags & VQADATF_LDONE) != 0 && rc == VQAERR_NOBUFFER) {
              vqabuf->Flags |= VQADATF_DDONE;
            }

            if (rc == VQAERR_NOT_TIME && config->EventHandler != nullptr) {
              // zzz
              config->EventHandler(VQAEVENT_SYNC, nullptr, 0);
            }
          }
        } else {
          vqabuf->Flags |= VQADATF_DDONE;
          drawer->CurFrame->Flags = 0L;
          drawer->CurFrame = drawer->CurFrame->Next;
        }

        if (mode == VQAMODE_WALK) {
          break;
        }
      }
      break;
  }

  /* If the movie is finished or we are requested to stop then shutdown. */
  if ((vqabuf->Flags & (VQADATF_DDONE | VQADATF_LDONE)) ==
          (VQADATF_DDONE | VQADATF_LDONE) ||
      mode == VQAMODE_STOP) {
    /* Record the end time; must be done before stopping audio, since we're
     * getting the elapsed time from the audio DMA position.
     */
    vqabuf->EndTime = VQA_GetTime(vqa);

    /* Movie is finished. */
    rc = VQAERR_EOF;
  }

  /* Stop audio, if it's playing. */
  if ((vqabuf->Audio.Flags & VQAAUDF_ISPLAYING) != 0) {
    VQA_StopAudio(vqa);
  }

#ifdef _WIN32
  /*
  ** Restore the process priority level
  */
  SetPriorityClass(GetCurrentProcess(), process_priority);
#endif  // _WIN32

  return rc;
}

/****************************************************************************
 *
 * NAME
 *     VQA_SetStop - Set the frame the player should stop on.
 *
 * SYNOPSIS
 *     OldStop = VQA_SetStop(VQA, Frame)
 *
 *     long = VQA_SetStop(VQAHandle *, long);
 *
 * FUNCTION
 *     Set the frame that the player should stop on. This function will only
 *     work on movies that are already open.
 *
 * INPUTS
 *     VQA   - VQAHandle of movie to set the stop frame for.
 *     Frame - Frame number to stop on.
 *
 * RESULT
 *     OldStop - Previous stop frame. (-1 = invalid stop frame)
 *
 ****************************************************************************/

auto VQA_SetStop(VQAHandle* vqa, int64_t stop) -> int64_t {
  int64_t oldstop = -1;

  /* Get a local pointer to the header. */
  auto* header = &vqa->header;

  if (stop > 0 && header->Frames >= stop) {
    oldstop = header->Frames;
    header->Frames = stop;
  }

  return oldstop;
}

/****************************************************************************
 *
 * NAME
 *     VQA_GetInfo - Get VQA movie information.
 *
 * SYNOPSIS
 *     VQA_GetInfo(VQA, Info)
 *
 *     void VQA_GetInfo(VQAHandle *, VQAInfo *);
 *
 * FUNCTION
 *     Retrieve information about the opened movie.
 *
 * INPUTS
 *     VQA  - Pointer to VQAHandle of opened movie.
 *     Info - Pointer to VQAInfo structure to fill.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

void VQA_GetInfo(VQAHandle* vqa, VQAInfo* info) {
  auto* header = &vqa->header;

  info->NumFrames = header->Frames;
  info->ImageHeight = header->ImageHeight;
  info->ImageWidth = header->ImageWidth;
  info->ImageBuf = vqa->data->Drawer.ImageBuf;
}

/****************************************************************************
 *
 * NAME
 *     VQA_GetStats - Get VQA movie statistics.
 *
 * SYNOPSIS
 *     VQA_GetStats(VQA, Stats)
 *
 *     void VQA_GetStats(VQAHandle *, VQAStatistics *);
 *
 * FUNCTION
 *     Retrieve the statistics for the VQA movie.
 *
 * INPUTS
 *     VQA   - Handle of VQA movie to get statistics for.
 *     Stats - Pointer to VQAStatistics to fill.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

void VQA_GetStats(VQAHandle* vqa, VQAStatistics* stats) {
  VQAData* vqabuf;

  /* Dereference VQAData structure from VQAHandle */
  vqabuf = vqa->data;

  stats->MemUsed = vqabuf->MemUsed;
  stats->StartTime = vqabuf->StartTime;
  stats->EndTime = vqabuf->EndTime;
  stats->FramesLoaded = vqabuf->LoadedFrames;
  stats->FramesDrawn = vqabuf->DrawnFrames;
  stats->FramesSkipped = vqabuf->Drawer.NumSkipped;
  stats->MaxFrameSize = vqabuf->Loader.MaxFrameSize;
  stats->SamplesPlayed = vqabuf->Audio.SamplesPlayed;
}

/****************************************************************************
 *
 * NAME
 *     User_Update - Page flip routine called by the task interrupt.
 *
 * SYNOPSIS
 *     User_Update(VQA)
 *
 *     long User_Update(VQAHandle *);
 *
 * FUNCTION
 *
 * INPUTS
 *     VQA - Handle of VQA movie.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

int64_t User_Update(VQAHandle* vqa) {
  auto* vqabuf = vqa->data;

  if ((vqabuf->Flags & VQADATF_UPDATE) != 0) {
    // Update data for mono output
    vqabuf->Flipper.LastFrameNum = vqabuf->Flipper.CurFrame->FrameNum;

    // Mark the frame as loadable
    vqabuf->Flipper.CurFrame->Flags = 0;
    vqabuf->Flags &= ~VQADATF_UPDATE;
  }

  return 0;
}

void VQA_Dummy() { Set_Font(nullptr); }
