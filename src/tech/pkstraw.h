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

/* $Header: /CounterStrike/PKSTRAW.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : PKSTRAW.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/08/96 *
 *                                                                                             *
 *                  Last Update : July 8, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef PKSTRAW_H
#define PKSTRAW_H

#include "tech/blowfish.h"
#include "tech/blwstraw.h"
#include "tech/pk.h"
#include "tech/rndstraw.h"
#include "tech/straw.h"

class PKStraw : public Straw {
 public:
  typedef enum CryptControl { ENCRYPT, DECRYPT } CryptControl;

  PKStraw(CryptControl control, RandomStraw& rnd);
  ~PKStraw() override = default;

  PKStraw(const PKStraw&) = delete;
  PKStraw& operator=(const PKStraw&) = delete;
  PKStraw(PKStraw&&) = delete;
  PKStraw& operator=(PKStraw&&) = delete;

  void Get_From(Straw* straw) override;
  void Get_From(Straw& straw) override { Get_From(&straw); }

  int Get(void* source, int slen) override;

  // Submit key to be used for encryption/decryption.
  void Key(const PKey* key);

 private:
  enum {
    BLOWFISH_KEY_SIZE = BlowfishEngine::MAX_KEY_LENGTH,
    MAX_KEY_BLOCK_SIZE = 256  // Maximum size of pk encrypted blowfish key.
  };

  /*
  **	This flag indicates whether the PK (fetch blowfish key) phase is
  **	in progress or not.
  */
  bool IsGettingKey = true;

  /*
  **	This is the random straw that is needed to generate the
  **	blowfish key.
  */
  RandomStraw& Rand;

  /*
  **	This is the attached blowfish pipe. After the blowfish key has been
  **	decrypted, then the PK processor goes dormant and the blowfish processor
  **	takes over the data flow.
  */
  BlowStraw BF;

  /*
  **	This control member tells what method (encryption or decryption) that
  *should *	be performed on the data stream.
  */
  CryptControl Control;

  /*
  **	Pointer to the key to use for encryption or decryption. If this pointer
  *is NULL, then *	the data passing through this segment will not be
  *modified.
  */
  const PKey* CipherKey = nullptr;

  /*
  **	This is the staging buffer for the block of data. This block must be as
  *large as *	the largest possible key size or the largest blowfish key size
  *(whichever is *	greater).
  */
  char Buffer[256];

  int Counter = 0;

  /*
  **	This records the number of bytes remaining in the current block. This
  **	will be the number of bytes left to accumulate before the block can be
  **	processed either for encryption or decryption.
  */
  int BytesLeft = 0;

  int Encrypted_Key_Length() const;
  int Plain_Key_Length() const;
};

#endif
