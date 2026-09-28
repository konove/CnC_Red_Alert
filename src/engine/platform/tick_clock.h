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

// File: The game's clocks: SystemTicks(), real time in 60 Hz ticks, and the
// millisecond wall clock. The Stopwatch and Timer templates in
// engine/platform/timer.h measure time against them.

#ifndef CNC_RED_ALERT_ENGINE_PLATFORM_TICK_CLOCK_H_
#define CNC_RED_ALERT_ENGINE_PLATFORM_TICK_CLOCK_H_

#include <cstdint>

// The rate SystemTicks() counts at.
inline constexpr int kSystemTicksPerSecond = 60;

// Returns real time in milliseconds, read from the same monotonic clock as
// SystemTicks(). Like it, the zero point means nothing on its own; 64 bits
// because that clock counts from boot, so 32 bits would wrap after 49 days of
// machine uptime.
int64_t Get_Time_Ms();

// Returns real time in ticks of 1/60 second, read from a monotonic clock that
// needs no setup. The zero point means nothing on its own: callers compare two
// readings to measure how much time has passed.
int64_t SystemTicks();

#endif  // CNC_RED_ALERT_ENGINE_PLATFORM_TICK_CLOCK_H_
