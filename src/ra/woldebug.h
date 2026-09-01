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

#ifndef CNC_RED_ALERT_RA_WOLDEBUG_H_
#define CNC_RED_ALERT_RA_WOLDEBUG_H_

// Where the Westwood Online code sends its tracing.
//
// This was `#define debugprint OutputDebugString`, which never matched its
// call sites: every one of them passes printf-style arguments to what is a
// one-argument Win32 function. That went unnoticed because all ~325 calls are
// commented out -- ajw left the tracing in the source but out of the build.
//
// A variadic no-op keeps them that way while making the arguments type-check
// if any are ever uncommented, which the macro never did. Real logging in this
// tree goes through DLOG (see CLAUDE.md); this exists so the historical calls
// still mean something when read.
template <typename... Args>
inline void debugprint(const char* /*format*/, Args&&... /*args*/) {}

#endif  // CNC_RED_ALERT_RA_WOLDEBUG_H_
