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

/***************************************************************************
 **      C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S      **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood 32 bit Library                  *
 *                                                                         *
 *                    File Name : AUDIO.H                                  *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : March 10, 1995                           *
 *                                                                         *
 *                  Last Update : March 10, 1995   [PWG]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#ifndef CNC_RED_ALERT_SDLLIB_WW_AUDIO_H_
#define CNC_RED_ALERT_SDLLIB_WW_AUDIO_H_

#include <cstddef>
#include <cstdint>
#include <span>

/*=========================================================================*/
/* AUD file header type
 */
/*=========================================================================*/
// Bits of AudHeader::flags.
constexpr uint8_t kAudFlagStereo = 1;
constexpr uint8_t kAudFlag16Bit = 2;

// PWG 3-14-95: This structure used to have bit fields defined for Stereo
//   and Bits.  These were removed because watcom packs them into a 32 bit
//   flag entry even though they could have fit in a 8 bit entry.
#pragma pack(push, 1)
struct AudHeader {
  uint16_t sample_rate;        // Playback rate (hertz).
  int32_t compressed_bytes;    // Size of the data that follows the header.
  int32_t uncompressed_bytes;  // Size of the data once decoded.
  uint8_t flags;               // kAudFlagStereo, kAudFlag16Bit
  uint8_t compression;         // What kind of compression for this sample?
};
#pragma pack(pop)

/*=========================================================================*/
/*	There can be a different sound driver for sound effects, digitized
 */
/*	samples, and musical scores.  Each one must be of these specified
 */
/*	types.
 */
/*=========================================================================*/
enum class Sample_Type {
  SAMPLE_NONE = 0,  // No digitized sounds will be played.
  SAMPLE_SDL = 1,
};
using enum Sample_Type;

enum class SFX_Type {
  SFX_NONE = 0,  // No sound effects will be played.
  SFX_SDL = 1,
};
using enum SFX_Type;

/*=========================================================================*/
/* The following prototypes are for the file: SOUNDIO.CPP
 */
/*=========================================================================*/
int StreamSampleFile(const char* filename, int volume);
void PumpSampleStreams();
bool OpenAudio(int rate, bool stereo);
void CloseAudio();
void StopSample(int handle);
bool IsSamplePlaying(int handle);
bool IsSamplePlaying(const void* sample);
void StopSample(const void* sample);
int PlaySample(std::span<const std::byte> sample, int priority = 0xFF,
               int volume = 0xFF, int16_t panloc = 0x0);
int PlaySampleOnChannel(std::span<const std::byte> sample, int priority,
                        int volume, int16_t panloc, int handle);
int SetScoreVolume(int volume);
void FadeOutSample(int handle, int ticks);
int AcquireChannel(int priority);
int GetDigiHandle();
void ResumeAudio();
void PauseAudio();

std::span<std::byte> LoadSample(const char* filename);
void FreeSample(void* sample);

using AudioCallback = void (*)(uint8_t* device_buffer, int device_bytes);
uint32_t AudioDeviceId();
void* AudioOutputSpec();
AudioCallback*
ExtraAudioCallbackSlot();  // returns a ptr to a function ptr as we're passing
                           // this the wrong way around

extern SFX_Type SoundType;
extern Sample_Type SampleType;

#endif  // CNC_RED_ALERT_SDLLIB_WW_AUDIO_H_
