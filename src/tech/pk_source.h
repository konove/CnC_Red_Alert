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

#ifndef CNC_RED_ALERT_TECH_PK_SOURCE_H_
#define CNC_RED_ALERT_TECH_PK_SOURCE_H_

#include <memory>

#include "tech/blowfish_source.h"
#include "tech/byte_source.h"
#include "tech/pk.h"

// Reads the PK-encrypted blowfish key header from source, decrypts it,
// and returns a BlowfishSource configured to decrypt the remaining data.
// Returns nullptr on failure (e.g., couldn't read full key header).
// Caller must ensure source outlives the returned BlowfishSource.
std::unique_ptr<BlowfishSource> MakePkDecryptSource(ByteSource& source,
                                                    const PKey& key);

#endif  // CNC_RED_ALERT_TECH_PK_SOURCE_H_
