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

/* $Header: /CounterStrike/BLOWFISH.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : BLOWFISH.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 04/14/96 *
 *                                                                                             *
 *                  Last Update : April 14, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_ENGINE_CRYPTO_BLOWFISH_H_
#define CNC_RED_ALERT_ENGINE_CRYPTO_BLOWFISH_H_

#include <cstddef>
#include <memory>
#include <span>

// Whether a Blowfish link encrypts or decrypts what passes it.
enum class CipherMode { kEncrypt, kDecrypt };

/*
**	This engine will process data blocks by encryption and decryption.
**	The "Blowfish" algorithm is in the public domain. It uses
**	a Feistal network (similar to IDEA). It has no known
**	weaknesses, but is still relatively new. Blowfish is particularly strong
**	against brute force attacks. It is also quite strong against linear and
**	differential cryptanalysis. Its weakness is that it takes a relatively
**	long time to set up with a new key (1/100th of a second on a P6-200).
**	The time to set up a key is equivalent to encrypting 4240 bytes.
*/
class BlowfishEngine {
 public:
  BlowfishEngine();
  ~BlowfishEngine();

  BlowfishEngine(const BlowfishEngine&) = delete;
  BlowfishEngine& operator=(const BlowfishEngine&) = delete;
  BlowfishEngine(BlowfishEngine&&) = delete;
  BlowfishEngine& operator=(BlowfishEngine&&) = delete;

  // Sets the key, of at most kMaxKeyLength bytes. An empty key removes it.
  void Submit_Key(std::span<const std::byte> key);

  // Encrypts (decrypts) all bytes from the source buffer into the
  // destination buffer and returns how many bytes were processed. Only whole
  // 8-byte blocks are transformed; a trailing partial block is copied as is.
  // The destination must be at least as large as the source. To work in place,
  // pass the same buffer as both arguments. Without a key the data is copied.
  int Encrypt(std::span<const std::byte> plaintext,
              std::span<std::byte> cyphertext);
  int Decrypt(std::span<const std::byte> cyphertext,
              std::span<std::byte> plaintext);

  /*
  **	This is the maximum key length supported.
  */
  static constexpr int kMaxKeyLength = 56;

 private:
  // The tables a key expands to, which live in Rust (src/rust/src/blowfish).
  struct Key;

  // Runs `source` through the cipher into `dest`, or copies it without a key,
  // and returns what Encrypt() and Decrypt() do.
  [[nodiscard]] int Process(std::span<const std::byte> source,
                            std::span<std::byte> dest, CipherMode mode) const;

  std::unique_ptr<Key> key_;  // nullptr without a key
};

#endif  // CNC_RED_ALERT_ENGINE_CRYPTO_BLOWFISH_H_
