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

/* $Header: /CounterStrike/PK.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : PK.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/04/96 *
 *                                                                                             *
 *                  Last Update : July 10, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * PKey::Decode_Exponent -- Decodes the exponent back into the key.
 ** PKey::Decode_Modulus -- Decodes the modulus value back into the key. *
 *   PKey::Decrypt -- Decrypt supplied cyphertext into its original plaintext. *
 *   PKey::Encode_Exponent -- Encode the exponent portion of the key into a
 *buffer.            * PKey::Encode_Modulus -- Encode the modulus portion of the
 *key.                            * PKey::Encrypt -- Encrypt blocks of
 *plaintext.                                             * PKey::Generate --
 *Generate a public and private key.                                      *
 *   PKey::PKey -- Construct a key using encoded strings. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "engine/crypto/pk.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "absl/log/check.h"
#include "base/numeric.h"
#include "engine/stream/byte_source.h"
#include "engine/stream/rust_bytes.h"
#include "engine_rs_cxx/lib.h"

// The numbers and their arithmetic are engine_rs's pk module
// (src/rust/src/pk); this class holds a key's two numbers and cuts data into
// blocks.

namespace {

// Writes `number` as a DER integer to the front of `buffer`; see
// PKey::Encode_Modulus().
int Encode(const std::array<uint8_t, 256>& number,
           const std::span<std::byte> buffer) {
  if (buffer.empty()) {
    return 0;
  }
  std::array<std::byte, PKey::kMaxEncodedBytes> encoded{};
  const std::size_t size = engine_rs::pk_der_encode(
      number, AsRustSlice(std::span<std::byte>(encoded)));
  CHECK_LE(size, buffer.size()) << "DER integer does not fit its buffer";
  std::ranges::copy(std::span(encoded).first(size), buffer.begin());
  return static_cast<int>(size);
}

}  // namespace

int PKey::Encrypt(const std::span<const std::byte> source,
                  const std::span<std::byte> dest) const {
  return Crypt(source, dest, Plain_Block_Size(), Crypt_Block_Size());
}

int PKey::Decrypt(const std::span<const std::byte> source,
                  const std::span<std::byte> dest) const {
  return Crypt(source, dest, Crypt_Block_Size(), Plain_Block_Size());
}

void PKey::Generate(ByteSource& random, const int bits, PKey& fastkey,
                    PKey& slowkey) {
  CHECK(bits >= 32 && bits <= 1024) << "Primes of " << bits << " bits";
  std::array<std::byte, kSeedBytes> seed{};
  // A source that runs dry would leave zeros in the seed, and a key anyone
  // could repeat.
  CHECK_EQ(random.Read(seed), kSeedBytes) << "Random source ran short";
  // Into numbers of its own: `fastkey` and `slowkey` may be one object, and
  // Rust may not be handed two mutable references to the same bytes.
  Number modulus{};
  Number fast_exponent{};
  Number slow_exponent{};
  engine_rs::pk_generate(AsRustSlice(std::span<const std::byte>(seed)),
                         base::ToSize(bits), modulus, fast_exponent,
                         slow_exponent);
  // The modulus is the same for both keys, and the fast exponent is always
  // Fast_Exponent(): a fast key is fully described by its modulus.
  fastkey.exponent_ = fast_exponent;
  fastkey.set_modulus(modulus);
  slowkey.exponent_ = slow_exponent;
  slowkey.set_modulus(modulus);
}

int PKey::Encode_Modulus(const std::span<std::byte> buffer) const {
  return Encode(modulus_, buffer);
}

int PKey::Encode_Exponent(const std::span<std::byte> buffer) const {
  return Encode(exponent_, buffer);
}

void PKey::Decode_Modulus(const std::span<const std::byte> buffer) {
  Number modulus = modulus_;
  engine_rs::pk_der_decode(AsRustSlice(buffer), modulus);
  set_modulus(modulus);
}

void PKey::Decode_Exponent(const std::span<const std::byte> buffer) {
  engine_rs::pk_der_decode(AsRustSlice(buffer), exponent_);
}

void PKey::SetFastExponent() {
  exponent_ = {};
  const uint32_t exponent = Fast_Exponent();
  for (std::size_t byte = 0; byte < sizeof(exponent); ++byte) {
    exponent_.at(byte) = static_cast<uint8_t>(exponent >> (byte * 8U));
  }
}

int PKey::Crypt(std::span<const std::byte> source, std::span<std::byte> dest,
                const int in_size, const int out_size) const {
  int total = 0;
  // Partial blocks are not processed and are not copied.
  while (in_size > 0 && std::cmp_greater_equal(source.size(), in_size) &&
         std::cmp_greater_equal(dest.size(), out_size)) {
    // Out of the caller's buffer first: `dest` may overlap `source`, and Rust
    // may not see one byte through a shared and a mutable slice at once.
    std::array<std::byte, sizeof(Number)> block{};
    const auto in = std::span(block).first(base::ToSize(in_size));
    std::ranges::copy(source.first(in.size()), in.begin());
    const auto out = dest.first(base::ToSize(out_size));
    engine_rs::pk_crypt_block(exponent_, modulus_,
                              AsRustSlice(std::span<const std::byte>(in)),
                              AsRustSlice(out));
    source = source.subspan(in.size());
    dest = dest.subspan(out.size());
    total += out_size;
  }
  return total;
}

void PKey::set_modulus(const Number& modulus) {
  modulus_ = modulus;
  bit_precision_ = static_cast<int>(engine_rs::pk_bit_count(modulus_)) - 1;
}
