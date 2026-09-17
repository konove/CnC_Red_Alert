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
#ifndef CNC_RED_ALERT_RA_INSTALLATION_H_
#define CNC_RED_ALERT_RA_INSTALLATION_H_

// File: Which game content is installed, and which disc holds it.

// Returns the registry subkey, under HKEY_LOCAL_MACHINE, holding this game's
// installer settings. The key name is language specific, because each
// localized release installed as a separate product.
const char* Game_Registry_Key();

// Reads one of the DWORD flags the Windows installer left under
// Game_Registry_Key() -- CStrikeInstalled, AftermathInstalled, DVD. False when
// the flag is absent or zero, which is always the case off Windows.
bool ReadInstallerFlag(const char* value_name);

// Reports whether the Counterstrike expansion is installed, by reading the flag
// the installer left in the registry. The answer is cached: it cannot change
// while the game is running.
//
// Off Windows there is no registry and no separate expansion installer, so the
// content is simply assumed to be present. Before the installer wrote a
// registry flag, this was decided by probing for EXPAND.MIX.
bool Is_Counterstrike_Installed();

// Reports whether the Aftermath expansion is installed. See
// Is_Counterstrike_Installed(); this works the same way, and the probe
// it replaced was for EXPAND2.MIX.
bool Is_Aftermath_Installed();

// Identifies which C&C disc is in the given drive by matching its volume label.
// Returns a CD_VOLUME value (the kCd* values in
// installation.cc), or -1 if the disc is not a C&C
// one. Retries for up to timeout ticks, which gives a CD changer time to swap
// discs before giving up.
int Get_CD_Index(int cd_drive, int timeout);

// Ensures the requested disc is in a drive, searching every CD drive and then
// prompting the player until it turns up. Returns false if the player cancelled
// the prompt. cd is a CD_VOLUME value (the kCd* values in installation.cc): -2
// means the data is on the hard drive and nothing needs checking, -1 means any
// C&C disc will do.
//
// On success the file system's CD search path is repointed at the drive that
// holds the disc, and the secondary mix files are re-registered from it.
bool Force_CD_Available(int cd_desired);

// Ensures the disc holding the given official scenario is available. Expansion
// scenarios live on their own discs; everything else is always reachable.
bool Force_Scenario_Available(const char* name);

#endif  // CNC_RED_ALERT_RA_INSTALLATION_H_
