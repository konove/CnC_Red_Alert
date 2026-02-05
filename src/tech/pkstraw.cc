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

#include "tech/pkstraw.h"

#include <memory>

#include "tech/blowfish.h"

namespace {
constexpr int kBlowfishKeySize = BlowfishEngine::MAX_KEY_LENGTH;
constexpr int kMaxKeyBlockSize = 256;
}  // namespace

std::unique_ptr<BlowStraw> MakePKDecryptStraw(Straw& source, const PKey& key) {
  // Calculate how many bytes the encrypted blowfish key occupies.
  int encrypted_len =
      key.Block_Count(kBlowfishKeySize) * key.Crypt_Block_Size();

  // Read the encrypted key header.
  char encrypted_key[kMaxKeyBlockSize];
  int got = source.Get(encrypted_key, encrypted_len);
  if (got != encrypted_len) {
    return nullptr;
  }

  // Decrypt to get the blowfish key.
  char blowfish_key[kMaxKeyBlockSize];
  key.Decrypt(encrypted_key, got, blowfish_key);

  // Create and configure the BlowStraw.
  auto straw = std::make_unique<BlowStraw>(BlowStraw::DECRYPT);
  straw->Key(blowfish_key, kBlowfishKeySize);
  straw->SetSource(source);
  return straw;
}
