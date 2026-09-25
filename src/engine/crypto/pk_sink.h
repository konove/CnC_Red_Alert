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

#ifndef CNC_RED_ALERT_ENGINE_CRYPTO_PK_SINK_H_
#define CNC_RED_ALERT_ENGINE_CRYPTO_PK_SINK_H_

#include <memory>

#include "engine/crypto/blowfish_sink.h"
#include "engine/crypto/pk.h"
#include "engine/crypto/random_source.h"
#include "engine/stream/byte_sink.h"

// Generates a random blowfish key, encrypts it with the public key,
// writes the encrypted key to sink, and returns a BlowfishSink configured
// to encrypt subsequent data. Caller must ensure sink outlives the
// returned BlowfishSink.
std::unique_ptr<BlowfishSink> MakePkEncryptSink(ByteSink& sink, const PKey& key,
                                                RandomSource& rng);

#endif  // CNC_RED_ALERT_ENGINE_CRYPTO_PK_SINK_H_
