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

// File: VqaPlayer, and the playback loop that takes turns between the
// loader and the drawer (VQA_Play), with the small entry points around it.
//
// Originally written by Bill Randolph and Denzil E. Long, Jr. at Westwood
// Studios, July 1995, where the loader and drawer ran as tasks off a timer
// interrupt.

#include "winvq/vqa32/vqaio.h"

#include <span>

#include <cstdint>
#include <memory>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqaplay.h"
#include "winvq/vqa32/vqaplayp.h"

// VqaPlayer is a thin wrapper: each method forwards to the VQA_* entry point
// of the same name on its handle.

VqaPlayer::VqaPlayer() : impl_(std::make_unique<VQAHandle>()) {}

VqaPlayer::~VqaPlayer() {
  // Only an open movie needs shutdown; Close() on a never-opened player
  // would call Close() on an io object that may never have been installed.
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

// Each pass of the loop gives the loader one frame to load and the drawer one
// frame to draw; either may decline (no free buffer, not yet time) and the
// loop comes round again, so a RUN spins until the movie is done. The loader
// runs ahead by up to NumFrameBufs frames, which is what absorbs a slow read.
int32_t VQA_Play(VQAHandle* vqa, int32_t mode) {
  VQAData* vqabuf = nullptr;
  VQAConfig* config = nullptr;
  VQADrawer* drawer = nullptr;
  int32_t rc = 0;

#ifdef _WIN32
  // Run at high priority while the movie plays, so the busy loop below is not
  // starved of the time slices that keep the frames on schedule. Every return
  // below restores the saved level.
  DWORD process_priority = GetPriorityClass(GetCurrentProcess());
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif  // _WIN32

  vqabuf = vqa->data;
  drawer = &vqabuf->Drawer;
  config = &vqa->config;

  // The first call starts playback. The sound starts first so the clock
  // below can run from it, and only if VQA_Open() preloaded some. A movie
  // whose audio ring came out empty (AudioBufSize 0, or -1 when 1.5 seconds
  // of sound is less than one HMIBufSize block) plays silent.
  if ((vqabuf->Flags & VQADATF_PRIMED) == 0) {
    VQA_Configure_Drawer(vqa);

    if ((config->OptionFlags & VQAOPTF_AUDIO) != 0 &&
        !vqabuf->Audio.IsLoadedStorage.empty() &&
        vqabuf->Audio.IsLoadedStorage.front() != 0) {
      VQA_StartAudio(vqa);
    }

    // Set the clock to the time of the first frame loaded, so it is due now.
    const auto i =
        vqabuf->Drawer.CurFrame->FrameNum * VQA_TIMETICKS / config->DrawRate;

    VQA_SetTimer(vqa, i, config->TimerMethod);
    vqabuf->StartTime = VQA_GetTime(vqa);

    vqabuf->Flags |= VQADATF_PRIMED;
  }

  switch (mode) {
    case VQAMODE_PAUSE:
      if ((vqabuf->Flags & VQADATF_PAUSED) == 0) {
        vqabuf->Flags |= VQADATF_PAUSED;
        vqabuf->EndTime = VQA_GetTime(vqa);

        // The clock follows the sound, so stopping it stops the clock too.
        if ((vqabuf->Audio.Flags & VQAAUDF_ISPLAYING) != 0) {
          VQA_StopAudio(vqa);
        }
      }

      rc = VQAERR_PAUSED;
      break;

    // Shut down below without loading or drawing anything more.
    case VQAMODE_STOP:
      break;

    case VQAMODE_RUN:
    case VQAMODE_WALK:
    default:

      // Resume a paused movie: the sound, and the clock from where it
      // stopped.
      if ((vqabuf->Flags & VQADATF_PAUSED) != 0) {
        vqabuf->Flags &= ~VQADATF_PAUSED;

        // VQA_StartAudio() fails only if some movie's sound is already
        // playing, which ends this one.
        if (((config->OptionFlags & VQAOPTF_AUDIO) != 0) &&
            (VQA_StartAudio(vqa) != 0)) {
          VQA_StopAudio(vqa);
#ifdef _WIN32
            SetPriorityClass(GetCurrentProcess(), process_priority);
#endif  // _WIN32
            return VQAERR_EOF;
        }

        VQA_SetTimer(vqa, vqabuf->EndTime, config->TimerMethod);
      }

      // Load, draw, load, draw... until both are done.
      while ((vqabuf->Flags & (VQADATF_DDONE | VQADATF_LDONE)) !=
             (VQADATF_DDONE | VQADATF_LDONE)) {
        if ((vqabuf->Flags & VQADATF_LDONE) == 0) {
          rc = VQA_LoadFrame(vqa);
          if (rc == 0) {
            vqabuf->LoadedFrames++;
          } else {
            // A full ring or a wait on the sound is retried next pass. The
            // end of the file, or any error, ends the loading: the frames
            // already loaded still play.
            if (rc != VQAERR_NOBUFFER && rc != VQAERR_SLEEPING) {
              vqabuf->Flags |= VQADATF_LDONE;
              rc = 0;
            }
          }
        } else {
          VQAMovieDone++;
        }

        if ((config->DrawFlags & VQACFGF_NODRAW) == 0) {
          rc = (*vqabuf->Draw_Frame)(vqa);
          if (rc == 0) {
            vqabuf->DrawnFrames++;
            rc = vqabuf->Drawer.LastFrameNum;
            // The frame is on screen (the DrawerCallback showed it), so its
            // buffer can go back to the loader.
            if (User_Update(vqa) != 0) {
              vqabuf->Flags |= VQADATF_DDONE | VQADATF_LDONE;
            }
          } else {
            // DrawerCallback asked to stop.
            if (rc == VQAERR_EOF) {
              break;
            }
            // Nothing left to draw once nothing more will be loaded.
            if ((vqabuf->Flags & VQADATF_LDONE) != 0 && rc == VQAERR_NOBUFFER) {
              vqabuf->Flags |= VQADATF_DDONE;
            }

            // Too early for the next frame: let the client present or wait
            // instead of the loop spinning.
            if (rc == VQAERR_NOT_TIME && config->EventHandler != nullptr) {
              config->EventHandler(VQAEVENT_SYNC, nullptr, 0);
            }
          }
        } else {
          // Not drawing: discard each frame as soon as it is loaded.
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

  if ((vqabuf->Flags & (VQADATF_DDONE | VQADATF_LDONE)) ==
          (VQADATF_DDONE | VQADATF_LDONE) ||
      mode == VQAMODE_STOP) {
    // Read the clock before stopping the sound, since the clock is the
    // amount of sound played.
    vqabuf->EndTime = VQA_GetTime(vqa);

    rc = VQAERR_EOF;
  }

  // Every return stops the sound, even a walk that will be called again;
  // the next call does not restart it (only a resume from pause does).
  if ((vqabuf->Audio.Flags & VQAAUDF_ISPLAYING) != 0) {
    VQA_StopAudio(vqa);
  }

#ifdef _WIN32
  SetPriorityClass(GetCurrentProcess(), process_priority);
#endif  // _WIN32

  return rc;
}

auto VQA_SetStop(VQAHandle* vqa, int64_t stop) -> int64_t {
  int64_t oldstop = -1;

  auto* header = &vqa->header;

  if (stop > 0 && std::cmp_greater_equal(header->frame_count, stop)) {
    oldstop = header->frame_count;
    header->frame_count = static_cast<uint16_t>(stop);
  }

  return oldstop;
}

void VQA_GetInfo(VQAHandle* vqa, VQAInfo* info) {
  const auto* header = &vqa->header;

  info->NumFrames = header->frame_count;
  info->ImageHeight = header->image_height;
  info->ImageWidth = header->image_width;
  info->ImageBuf = vqa->data->Drawer.ImageBuf.data();
}

void VQA_GetStats(const VQAHandle* vqa, VQAStatistics* stats) {
  VQAData* vqabuf = vqa->data;

  stats->MemUsed = vqabuf->MemUsed;
  stats->StartTime = vqabuf->StartTime;
  stats->EndTime = vqabuf->EndTime;
  stats->FramesLoaded = vqabuf->LoadedFrames;
  stats->FramesDrawn = vqabuf->DrawnFrames;
  stats->FramesSkipped = vqabuf->Drawer.NumSkipped;
  stats->MaxFrameSize = vqabuf->Loader.MaxFrameSize;
  stats->SamplesPlayed = vqabuf->Audio.SamplesPlayed;
}

int64_t User_Update(const VQAHandle* vqa) {
  auto* vqabuf = vqa->data;

  if ((vqabuf->Flags & VQADATF_UPDATE) != 0) {
    // Remember the last frame released, for status reporting.
    vqabuf->Flipper.LastFrameNum = vqabuf->Flipper.CurFrame->FrameNum;

    // Clearing the flags hands the buffer back to the loader.
    vqabuf->Flipper.CurFrame->Flags = 0;
    vqabuf->Flags &= ~VQADATF_UPDATE;
  }

  return 0;
}
