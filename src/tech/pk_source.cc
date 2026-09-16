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

#include "tech/pk_source.h"

#include <memory>
#include <span>

#include "base/numeric.h"
#include "tech/blowfish.h"
#include "tech/blowfish_source.h"
#include "tech/byte_source.h"
#include "tech/pk.h"

namespace {
constexpr int kBlowfishKeySize = BlowfishEngine::kMaxKeyLength;
constexpr int kMaxKeyBlockSize = 256;
}  // namespace

std::unique_ptr<BlowfishSource> MakePkDecryptSource(ByteSource& source,
                                                    const PKey& key) {
  // Calculate how many bytes the encrypted blowfish key occupies.
  const int encrypted_len =
      key.Block_Count(kBlowfishKeySize) * key.Crypt_Block_Size();

  // Read the encrypted key header.
  char encrypted_key[kMaxKeyBlockSize];
  if (source.Read(std::as_writable_bytes(
          std::span(encrypted_key).first(base::ToSize(encrypted_len)))) !=
      encrypted_len) {
    return nullptr;
  }

  // Decrypt to get the blowfish key.
  char blowfish_key[kMaxKeyBlockSize];
  key.Decrypt(encrypted_key, encrypted_len, blowfish_key);

  // Create and configure the BlowfishSource.
  auto straw = std::make_unique<BlowfishSource>(CipherMode::kDecrypt, source);
  straw->Key(blowfish_key, kBlowfishKeySize);
  return straw;
}
