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

#include "tech/pkpipe.h"

#include <cstring>
#include <memory>
#include <span>

#include "base/numeric.h"
#include "tech/blowfish.h"
#include "tech/blowpipe.h"
#include "tech/pipe.h"
#include "tech/pk.h"
#include "tech/rndstraw.h"

namespace {
constexpr int kBlowfishKeySize = BlowfishEngine::MAX_KEY_LENGTH;
constexpr int kMaxKeyBlockSize = 256;
}  // namespace

std::unique_ptr<BlowPipe> MakePKEncryptPipe(Pipe& sink, const PKey& key,
                                            RandomStraw& rng) {
  // Generate a random blowfish key.
  char blowfish_key[kMaxKeyBlockSize];
  memset(blowfish_key, 0, sizeof(blowfish_key));
  rng.Get(
      std::as_writable_bytes(std::span(blowfish_key).first(kBlowfishKeySize)));

  // Calculate plain key length (padded to PK block size).
  const int plain_len =
      key.Block_Count(kBlowfishKeySize) * key.Plain_Block_Size();

  // Encrypt the blowfish key with the public key.
  char encrypted_key[kMaxKeyBlockSize];
  const int encrypted_len = key.Encrypt(blowfish_key, plain_len, encrypted_key);

  // Write the encrypted key header to the sink.
  sink.Put(std::as_bytes(
      std::span(encrypted_key).first(base::ToSize(encrypted_len))));

  // Create and configure the BlowPipe.
  auto pipe = std::make_unique<BlowPipe>(BlowPipe::ENCRYPT);
  pipe->Key(blowfish_key, kBlowfishKeySize);
  pipe->SetSink(sink);
  return pipe;
}
