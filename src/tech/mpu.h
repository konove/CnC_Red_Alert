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

/* $Header: /CounterStrike/MPU.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MPU.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/15/96 *
 *                                                                                             *
 *                  Last Update : July 17, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Get_CPU_Clock -- Fetches the current CPU clock time. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TECH_MPU_H_
#define CNC_RED_ALERT_TECH_MPU_H_

#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || \
    defined(_M_IX86)
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

// Returns the CPU timestamp counter (TSC) value.
inline uint64_t Get_CPU_Clock() { return __rdtsc(); }

#else
// Fallback for non-x86 platforms (e.g., ARM, WebAssembly).
#include <chrono>

inline uint64_t Get_CPU_Clock() {
  return static_cast<uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
}
#endif

// Legacy interface - returns low 32 bits, stores high 32 bits in 'high'.
inline unsigned long Get_CPU_Clock(unsigned long& high) {
  uint64_t tsc = Get_CPU_Clock();
  high = tsc >> 32;
  return tsc;
}

// Processor type constants (legacy).
constexpr int kProc80586 = 2;  // Pentium and later

// Returns the processor type. All modern CPUs return kProc80586.
inline int Processor() { return kProc80586; }

#endif  // CNC_RED_ALERT_TECH_MPU_H_
