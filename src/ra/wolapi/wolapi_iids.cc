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

/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */

/* File created by MIDL compiler version 3.01.75 */
/* at Wed Jul 29 16:25:34 1998
 */
/* Compiler settings for WOLAPI.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )

// This was wolapi_i.c, MIDL's companion to wolapi.h, textually #included into
// rawolapi.cc because a C file could not be added to the Watcom build. It is
// an ordinary translation unit now, which is why the extension changed.
//
// Its own IID and CLSID typedefs are gone: they were a fallback for builds
// with no COM headers, and defining IID as a struct whose first field is
// `unsigned long` would make it 24 bytes here rather than 16, silently
// corrupting every constant below. port/win32/win32_com.h supplies the real
// 16-byte type and defines the two guards this file used to test.
//
// Exactly one definition of each constant may exist in the program, so this
// file must never be #included anywhere.

#include "port/win32/win32_com.h"
#include "ra/wolapi/wolapi.h"

#ifdef __cplusplus
extern "C" {
#endif

const IID IID_IRTPatcher = {0x925CDEDE,
                            0x71B9,
                            0x11D1,
                            {0xB1, 0xC5, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56}};

const IID IID_IRTPatcherEvent = {
    0x925CDEE3,
    0x71B9,
    0x11D1,
    {0xB1, 0xC5, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56}};

const IID IID_IChat = {0x4DD3BAF4,
                       0x7579,
                       0x11D1,
                       {0xB1, 0xC6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56}};

const IID IID_IChatEvent = {0x4DD3BAF6,
                            0x7579,
                            0x11D1,
                            {0xB1, 0xC6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56}};

const IID IID_IDownload = {0x0BF5FCEB,
                           0x9F03,
                           0x11D1,
                           {0x9D, 0xC7, 0x00, 0x60, 0x97, 0xC5, 0x43, 0x21}};

const IID IID_IDownloadEvent = {
    0x6869E99D,
    0x9FB4,
    0x11D1,
    {0x9D, 0xC8, 0x00, 0x60, 0x97, 0xC5, 0x43, 0x21}};

const IID IID_INetUtil = {0xB832B0AA,
                          0xA7D3,
                          0x11D1,
                          {0x97, 0xC3, 0x00, 0x60, 0x97, 0x06, 0xFA, 0x0C}};

const IID IID_INetUtilEvent = {
    0xB832B0AC,
    0xA7D3,
    0x11D1,
    {0x97, 0xC3, 0x00, 0x60, 0x97, 0x06, 0xFA, 0x0C}};

const IID IID_IChat2 = {0x8B938190,
                        0xEF3F,
                        0x11D1,
                        {0x98, 0x08, 0x00, 0x60, 0x97, 0x06, 0xFA, 0x0C}};

const IID IID_IChat2Event = {0x8B938192,
                             0xEF3F,
                             0x11D1,
                             {0x98, 0x08, 0x00, 0x60, 0x97, 0x06, 0xFA, 0x0C}};

const IID LIBID_WOLAPILib = {0x925CDED1,
                             0x71B9,
                             0x11D1,
                             {0xB1, 0xC5, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56}};

const CLSID CLSID_RTPatcher = {
    0x925CDEDF,
    0x71B9,
    0x11D1,
    {0xB1, 0xC5, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56}};

const CLSID CLSID_Chat = {0x4DD3BAF5,
                          0x7579,
                          0x11D1,
                          {0xB1, 0xC6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56}};

const CLSID CLSID_Download = {0xBF6EA206,
                              0x9E55,
                              0x11D1,
                              {0x9D, 0xC6, 0x00, 0x60, 0x97, 0xC5, 0x43, 0x21}};

const CLSID CLSID_NetUtil = {0xB832B0AB,
                             0xA7D3,
                             0x11D1,
                             {0x97, 0xC3, 0x00, 0x60, 0x97, 0x06, 0xFA, 0x0C}};

const CLSID CLSID_Chat2 = {0x8B938191,
                           0xEF3F,
                           0x11D1,
                           {0x98, 0x08, 0x00, 0x60, 0x97, 0x06, 0xFA, 0x0C}};

#ifdef __cplusplus
}
#endif
