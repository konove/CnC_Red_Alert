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

#ifndef CNC_RED_ALERT_RA_COMINIT_H_
#define CNC_RED_ALERT_RA_COMINIT_H_

//
// Link with this to automatically initialize COM at startup
//  - See cominit.cpp for more info
//

class ComInit {
 public:
  ComInit();
  ~ComInit();
};

#endif  // CNC_RED_ALERT_RA_COMINIT_H_
