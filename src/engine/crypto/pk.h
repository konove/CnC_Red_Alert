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

/* $Header: /CounterStrike/PK.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : PK.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/03/96 *
 *                                                                                             *
 *                  Last Update : July 3, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_ENGINE_CRYPTO_PK_H_
#define CNC_RED_ALERT_ENGINE_CRYPTO_PK_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "engine/stream/byte_source.h"

// One half of an RSA-style key pair: a modulus and an exponent. A block is
// encrypted or decrypted by raising it to the exponent under the modulus;
// what one half of a pair encrypts, the other decrypts.
//
// The process is slow and the ciphertext is larger than the plaintext, so it
// suits only a little critical data, such as the key of a fast cipher: an
// encrypted MIX archive starts with its Blowfish key encrypted this way. Data
// is processed in blocks whose sizes follow from the modulus.
//
// The arithmetic is in Rust (src/rust/src/pk), which documents what the
// numbers' encodings do with malformed input.
//
// Example:
//   PKey key;
//   key.SetFastExponent();
//   key.Decode_Modulus(der_bytes);
//   const int size = key.Decrypt(cipher, plain);
class PKey {
 public:
  PKey() noexcept = default;

  // Encrypts (decrypts) the whole blocks of `source` into `dest` and returns
  // the number of bytes written. A trailing partial block is neither
  // processed nor copied, and blocks stop when `dest` has no room for the
  // next. A plain block is Plain_Block_Size() bytes, an encrypted one
  // Crypt_Block_Size(). The buffers may overlap.
  [[nodiscard]] int Encrypt(std::span<const std::byte> source,
                            std::span<std::byte> dest) const;
  [[nodiscard]] int Decrypt(std::span<const std::byte> source,
                            std::span<std::byte> dest) const;

  // Makes a key pair from two primes of `bits` bits each, 32 to 1024: `fastkey`
  // with Fast_Exponent(), `slowkey` with the exponent that undoes it. Draws
  // kSeedBytes from `random` and derives everything from them, so `random`
  // must be as unpredictable as the key should be.
  static void Generate(ByteSource& random, int bits, PKey& fastkey,
                       PKey& slowkey);

  [[nodiscard]] int Plain_Block_Size() const {
    return (bit_precision_ - 1) / 8;
  }
  [[nodiscard]] int Crypt_Block_Size() const { return Plain_Block_Size() + 1; }
  [[nodiscard]] int Block_Count(int plaintext_length) const {
    return ((plaintext_length - 1) / Plain_Block_Size()) + 1;
  }

  // Writes the modulus (exponent) as a DER integer to the front of `buffer`,
  // which must hold it (kMaxEncodedBytes always does), and returns its size;
  // 0 for an empty buffer. Only the low 512 bits are encoded.
  [[nodiscard]] int Encode_Modulus(std::span<std::byte> buffer) const;
  [[nodiscard]] int Encode_Exponent(std::span<std::byte> buffer) const;

  // Sets the modulus (exponent) from a DER integer. Malformed input leaves it
  // as it was.
  void Decode_Modulus(std::span<const std::byte> buffer);
  void Decode_Exponent(std::span<const std::byte> buffer);

  // Sets the exponent to Fast_Exponent(), the exponent of the public half of
  // every pair, so that such a key needs only its modulus stored.
  void SetFastExponent();

  static uint32_t Fast_Exponent() { return 65537; }

  // How much Generate() reads from its random source.
  static constexpr int kSeedBytes = 64;
  // The most Encode_Modulus() and Encode_Exponent() write.
  static constexpr int kMaxEncodedBytes = 67;

 private:
  // A 2048-bit number, least significant byte first.
  using Number = std::array<uint8_t, 256>;

  // Runs the blocks of `source`, `in_size` bytes each, through the key into
  // blocks of `out_size` bytes in `dest`. Returns the bytes written.
  [[nodiscard]] int Crypt(std::span<const std::byte> source,
                          std::span<std::byte> dest, int in_size,
                          int out_size) const;

  void set_modulus(const Number& modulus);

  Number modulus_{};   // p*q
  Number exponent_{};  // 65537, or its inverse under (p-1)(q-1)
  // One less than the modulus's bit count: the most bits a block may have and
  // still be below the modulus. -1 for a zero modulus.
  int bit_precision_ = 0;
};

#endif  // CNC_RED_ALERT_ENGINE_CRYPTO_PK_H_
