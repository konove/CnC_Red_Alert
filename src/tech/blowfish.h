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

#ifndef CNC_RED_ALERT_TECH_BLOWFISH_H_
#define CNC_RED_ALERT_TECH_BLOWFISH_H_

#include <climits>
#include <cstdint>

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
  BlowfishEngine() = default;
  ~BlowfishEngine();

  BlowfishEngine(const BlowfishEngine&) = delete;
  BlowfishEngine& operator=(const BlowfishEngine&) = delete;
  BlowfishEngine(BlowfishEngine&&) = delete;
  BlowfishEngine& operator=(BlowfishEngine&&) = delete;

  void Submit_Key(const void* key, int length);

  // Encrypts (decrypts) `length` bytes from the source buffer into the
  // destination buffer and returns how many bytes were processed. Only whole
  // 8-byte blocks are transformed; a trailing partial block is copied as is.
  // The destination must not be null: to work in place, pass the same
  // non-const buffer as both arguments. Without a key the data is copied.
  int Encrypt(const void* plaintext, int length, void* cyphertext);
  int Decrypt(const void* cyphertext, int length, void* plaintext);

  /*
  **	This is the maximum key length supported.
  */
  static constexpr int kMaxKeyLength = 56;

 private:
  bool IsKeyed = false;

  void Sub_Key_Encrypt(uint32_t& left, uint32_t& right);

  void Process_Block(const void* plaintext, void* cyphertext,
                     const uint32_t* ptable);
  void Initialize_Tables();

  static constexpr int kRounds = 16;
  static constexpr int kBytesPerBlock =
      8;  // Feistal round count (16 is standard).

  /*
  **	Initialization data for sub keys. The initial values are constant and
  **	filled with a number generated from pi. Thus they are not random but
  **	they don't hold a weak pattern either.
  */
  static const uint32_t P_Init[static_cast<int>(kRounds) + 2];
  static const uint32_t S_Init[4][UCHAR_MAX + 1];

  /*
  **	Permutation tables for encryption and decryption.
  */
  uint32_t P_Encrypt[static_cast<int>(kRounds) + 2]{};
  uint32_t P_Decrypt[static_cast<int>(kRounds) + 2]{};

  /*
  **	S-Box tables (four).
  */
  uint32_t bf_S[4][UCHAR_MAX + 1]{};
};

#endif  // CNC_RED_ALERT_TECH_BLOWFISH_H_
