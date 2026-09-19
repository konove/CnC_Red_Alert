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

// Red Alert's sound effects (VocType) and EVA speech (VoxType): name lookups,
// playing an effect in the world or the interface, and the EVA speech queue.
// Everything here is silent when the audio device is closed or the game runs
// with Debug_Quiet.
//
// Originally AUDIO.H by Joe L. Bostic, started June 21, 1994.

#ifndef CNC_RED_ALERT_RA_AUDIO_H_
#define CNC_RED_ALERT_RA_AUDIO_H_

#include "ra/defines.h"
#include "tech/fixed.h"

// Returns the sound effect whose root file name matches `name`, ignoring case,
// or VOC_NONE if none does or `name` is nullptr. Scenario and INI files name
// sound effects this way.
VocType VocFromName(const char* name);

// Returns the root file name of the EVA voice `voice`, for building lists of
// voices such as the trigger editor's, or "none" for VOX_NONE.
const char* VoxName(VoxType voice);

// Returns the root file name of the sound effect `voc`, which also serves as
// its descriptive name, or "none" for VOC_NONE.
const char* VocName(VocType voc);

// Plays a sound effect that has no place in the game world, such as an
// interface click. `volume` is scaled by the player's sound volume setting.
// Unit responses come in variations: `variation` picks one (negative for
// vehicles and aircraft, positive for infantry; the units pass their ID), and
// `house` picks the accent, defaulting to the house the player acts like.
// Other sounds ignore both. Returns the mixer's sound handle, or -1 if
// nothing was played.
int PlaySoundEffect(VocType voc, fixed volume = fixed(1), int variation = 1,
                    HousesType house = HOUSE_NONE);

// Has EVA say `voice`. Only one voice waits in the queue: the request is
// dropped if another is already waiting or `voice` is being said now. It
// starts at once if EVA is silent, otherwise from ServiceSpeech() once the
// current voice ends.
void Speak(VoxType voice);

// Starts the queued EVA voice once the current one has finished, loading it
// into a speech buffer unless one of them still holds it. Call it as often as
// possible; once per game tick is enough.
void ServiceSpeech();

// Stops EVA at once and drops the queued voice.
void StopSpeaking();

// Plays a sound effect that happens at `coord` in the game world. Off screen
// it plays quieter the further it is from the centre of the view; on screen,
// or with a zero `coord`, it plays at full volume. `variation` and `house` are
// as for PlaySoundEffect().
void PlaySoundEffectAt(VocType voc, COORDINATE coord, int variation = 1,
                       HousesType house = HOUSE_NONE);

// Returns true while EVA is speaking or has a voice queued, for callers that
// wait for the voice to finish, say at the end of a game. Runs ServiceSpeech()
// first, so polling it in a loop keeps the queue moving.
bool IsSpeaking();

#endif  // CNC_RED_ALERT_RA_AUDIO_H_
