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

#ifndef CNC_RED_ALERT_RA_WOLAPI_NETUTILDEFS_H_
#define CNC_RED_ALERT_RA_WOLAPI_NETUTILDEFS_H_

// Every NETUTIL_E_* and NETUTIL_S_* value is built out of MAKE_HRESULT.
#include "port/win32/win32_com.h"

#define NETUTIL_E_ERROR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 100)
#define NETUTIL_E_BUSY MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 101)

#define NETUTIL_S_FINISHED MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 500)

#endif  // CNC_RED_ALERT_RA_WOLAPI_NETUTILDEFS_H_
