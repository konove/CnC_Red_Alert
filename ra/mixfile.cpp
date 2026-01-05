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

/* $Header: /CounterStrike/MIXFILE.CPP 2     3/13/97 2:06p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MIXFILE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : August 8, 1994 *
 *                                                                                             *
 *                  Last Update : July 12, 1996 [JLB] *
 *                                                                                             *
 *                                                                                             *
 *                  Modified by Vic Grippi for WwXlat Tool 10/14/96 *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * MixFileClass::Cache -- Caches the named mixfile into RAM. *
 *   MixFileClass::Cache -- Loads this particular mixfile's data into RAM. *
 *   MixFileClass::Finder -- Finds the mixfile object that matches the name
 *specified.         * MixFileClass::Free -- Uncaches a cached mixfile. *
 *   MixFileClass::MixFileClass -- Constructor for mixfile object. *
 *   MixFileClass::Offset -- Searches in mixfile for matching file and returns
 *offset if found.* MixFileClass::Retrieve -- Retrieves a pointer to the
 *specified data file.                 * MixFileClass::~MixFileClass --
 *Destructor for the mixfile object.                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/mixfile.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "port/ex_string.h"
#include "ra/ccfile.h"
#include "ra/compat.h"
#include "ra/conquer.h"
#include "ra/externs.h"
#include "ra/startup.h"
#include "sdllib/include/misc.h"
#include "tech/buff.h"
#include "tech/pkstraw.h"
#include "tech/shastraw.h"
#include "tech/straw.h"
#include "tech/xstraw.h"

extern MFCD temp;

// template<class T> int Compare(T const *obj1, T const *obj2) {
//	if (*obj1 < *obj2) return(-1);
//	if (*obj1 > *obj2) return(1);
//	return(0);
// };

/*
**	This is the pointer to the first mixfile in the list of mixfiles
*registered *	with the mixfile system.
*/
template <class T>
List<MixFileClass<T> > MixFileClass<T>::MixList;

/***********************************************************************************************
 * MixFileClass::Free -- Uncaches a cached mixfile. *
 *                                                                                             *
 *    Use this routine to uncache a mixfile that has been cached. *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the filename of the mixfile that is to be
 *uncached.         *
 *                                                                                             *
 * OUTPUT:  bool; Was the mixfile found and freed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/23/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
bool MixFileClass<T>::Free(char const *filename) {
  MixFileClass *ptr = Finder(filename);

  if (ptr) {
    ptr->Free();
    return (true);
  }
  return (false);
}

/***********************************************************************************************
 * MixFileClass::~MixFileClass -- Destructor for the mixfile object. *
 *                                                                                             *
 *    This destructor will free all memory allocated by this mixfile and will
 *remove it from   * the system. A mixfile removed in this fashion must be
 *created anew in order to be        * subsequent used. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. * 01/06/1995 JLB : Puts mixfile header
 *table into EMS.                                      *
 *=============================================================================================*/
template <class T>
MixFileClass<T>::~MixFileClass(void) {
  /*
  **	Deallocate any allocated memory.
  */
  if (Filename) {
    free((char *)Filename);
  }
  if (Data != nullptr && IsAllocated) {
    delete[] Data;
    IsAllocated = false;
  }
  Data = nullptr;

  if (HeaderBuffer != nullptr) {
    delete[] HeaderBuffer;
    HeaderBuffer = nullptr;
  }

  /*
  **	Unlink this mixfile object from the chain.
  */
  this->Unlink();
}

/***********************************************************************************************
 * MixFileClass::MixFileClass -- Constructor for mixfile object. *
 *                                                                                             *
 *    This is the constructor for the mixfile object. It takes a filename and a
 *memory         * handler object and registers the mixfile object with the
 *system. The index block is      * allocated and loaded from disk by this
 *routine.                                          *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the filename of the mixfile object. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. * 07/12/1996 JLB : Handles compressed
 *file header.                                          *
 *=============================================================================================*/
template <class T>
MixFileClass<T>::MixFileClass(char const *filename, PKey const *key)
    : IsDigest(false),
      IsEncrypted(false),
      IsAllocated(false),
      Filename(nullptr),
      Count(0),
      DataSize(0),
      DataStart(0),
      HeaderBuffer(nullptr),
      Data(nullptr) {
  /*
  **	Check to see if the file is available. If it isn't, then
  **	no further processing is needed or possible.
  */
  if (!Force_CD_Available(RequiredCD)) {
    // Prog_End();
    Emergency_Exit(EXIT_FAILURE);
  }

  T file(filename);  // Working file object.
  Filename = strdup(file.File_Name());

  FileStraw fstraw(file);
  PKStraw pstraw(PKStraw::DECRYPT, CryptRandom);
  Straw *straw = &fstraw;

  if (!file.Is_Available()) return;

  /*
  **	Stuctures used to hold the various file headers.
  */
  FileHeader fileheader;
  struct {
    short First;   // Always zero for extended mixfile format.
    short Second;  // Bitfield of extensions to this mixfile.
  } alternate;

  /*
  **	Fetch the first bit of the file. From this bit, it is possible to detect
  **	whether this is an extended mixfile format or the plain format. An
  **	extended format may have extra options or data layout.
  */
  int got = straw->Get(&alternate, sizeof(alternate));

  /*
  **	Detect if this is an extended mixfile. If so, then see if it is
  *encrypted *	and/or has a message digest attached. Otherwise, just retrieve
  *the *	plain mixfile header.
  */
  if (alternate.First == 0) {
    IsDigest = ((alternate.Second & 0x01) != 0);
    IsEncrypted = ((alternate.Second & 0x02) != 0);

    if (IsEncrypted) {
      pstraw.Key(key);
      pstraw.Get_From(&fstraw);
      straw = &pstraw;
    }
    straw->Get(&fileheader, sizeof(fileheader));

  } else {
    memmove(&fileheader, &alternate, sizeof(alternate));
    straw->Get(((char *)&fileheader) + sizeof(alternate),
               sizeof(fileheader) - sizeof(alternate));
  }

  Count = fileheader.count;
  DataSize = fileheader.size;
  // Mono_Printf("Mixfileclass %s DataSize: %08x\n", filename, DataSize);
  // Get_Key();
  /*
  **	Load up the offset control array. If RAM is exhausted, then the mixfile
  *is invalid.
  */
  HeaderBuffer = new SubBlock[Count];
  if (HeaderBuffer == nullptr) return;
  straw->Get(HeaderBuffer, Count * sizeof(SubBlock));

  /*
  **	The start of the embedded mixfile data will be at the current file
  *offset. *	This should be true even if the file header has been encrypted
  *because the file *	header was cleverly written with just the sufficient
  *number of padding bytes so *	that this condition would be true.
  */
  DataStart = file.Seek(0, SEEK_CUR) + file.BiasStart;
  //	DataStart = file.Seek(0, SEEK_CUR);

  /*
  **	Attach to list of mixfiles.
  */
  MixList.Add_Tail(this);
}

/***********************************************************************************************
 * MixFileClass::Retrieve -- Retrieves a pointer to the specified data file. *
 *                                                                                             *
 *    This routine will return with a pointer to the specified data file if the
 *file resides   * in memory. Otherwise, this routine returns nullptr. Use this
 *routine to access a resident   * file directly rather than going through the
 *process of pseudo disk access. This will     * save both time and RAM. *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the filename of the data file to retrieve a
 *pointer to.     *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the data file's data. If the file is not
 *in RAM, then    * nullptr is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/23/1994 JLB : Created. *
 *=============================================================================================*/
template <class T>
void const *MixFileClass<T>::Retrieve(char const *filename) {
  void *ptr = nullptr;
  Offset(filename, &ptr);
  return (ptr);
};

/***********************************************************************************************
 * MixFileClass::Finder -- Finds the mixfile object that matches the name
 *specified.           *
 *                                                                                             *
 *    This routine will scan through all registered mixfiles and return with a
 *pointer to      * the matching mixfile. If no mixfile could be found that
 *matches the name specified,      * then nullptr is returned. *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the filename to search for. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the matching mixfile -- if found. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. * 06/08/1996 JLB : Only compares
 *filename and extension.                                    *
 *=============================================================================================*/
template <class T>
MixFileClass<T> *MixFileClass<T>::Finder(char const *filename) {
  MixFileClass<T> *ptr = MixList.First();
  while (ptr->Is_Valid()) {
    // Extract just the filename+extension (no path or drive)
    auto file_only = std::filesystem::path(ptr->Filename).filename().string();

    if (stricmp(file_only.c_str(), filename) == 0) {
      return ptr;
    }
    ptr = ptr->Next();
  }
  return nullptr;
}

/***********************************************************************************************
 * MixFileClass::Cache -- Caches the named mixfile into RAM. *
 *                                                                                             *
 *    This routine will cache the mixfile, specified by name, into RAM. *
 *                                                                                             *
 * INPUT:   filename -- The name of the mixfile that should be cached. *
 *                                                                                             *
 * OUTPUT:  bool; Was the cache successful? *
 *                                                                                             *
 * WARNINGS:   This routine could go to disk for a very long time. *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. *
 *=============================================================================================*/
template <class T>
bool MixFileClass<T>::Cache(char const *filename, Buffer const *buffer) {
  MixFileClass<T> *mixer = Finder(filename);

  if (mixer != nullptr) {
    return (mixer->Cache(buffer));
  }
  return (false);
}

/***********************************************************************************************
 * MixFileClass::Cache -- Loads this particular mixfile's data into RAM. *
 *                                                                                             *
 *    This load the mixfile data into ram for this mixfile object. This is the
 *counterpart     * to the Free() function. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Was the file load successful?  It could fail if there wasn't
 *enough room     * to allocate the raw data block. *
 *                                                                                             *
 * WARNINGS:   This routine goes to disk for a potentially very long time. *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. * 07/12/1996 JLB : Handles attached
 *message digest.                                         *
 *=============================================================================================*/
template <class T>
bool MixFileClass<T>::Cache(Buffer const *buffer) {
  /*
  **	If the mixfile is already cached, then no action needs to be performed.
  */
  if (Data != nullptr) return (true);

  /*
  **	If a buffer was supplied (and it is big enough), then use it as the data
  *block *	pointer. Otherwise, the data block must be allocated.
  */
  if (buffer != nullptr) {
    if (buffer->Get_Size() == 0 || buffer->Get_Size() >= DataSize) {
      Data = buffer->Get_Buffer();
    }
  } else {
    Data = new char[DataSize];
    IsAllocated = true;
  }

  /*
  **	If there is a data buffer to fill, then fill it now.
  */
  if (Data != nullptr) {
    T file(Filename);

    FileStraw fstraw(file);
    Straw *straw = &fstraw;

    /*
    **	If a message digest is attached, then link a SHA straw segment to the
    *data *	stream so that the actual SHA can be compared with the attached
    *one.
    */
    SHAStraw sha;
    if (IsDigest) {
      sha.Get_From(fstraw);
      straw = &sha;
    }

    /*
    **	Bias the file to the actual start of the data. This is necessary because
    *the *	real data starts some distance (not so easily determined) from
    *the beginning of *	the real file.
    */
    file.Open(READ);
    file.Bias(0);
    file.Bias(DataStart);

    /*
    **	Fetch the whole mixfile data in one step. If the number of bytes
    *retrieved *	does not equal that requested, then this indicates a
    *serious error.
    */
    long actual = straw->Get(Data, DataSize);
    if (actual != DataSize) {
      delete[] Data;
      Data = nullptr;
      file.Error(EIO);
      return (false);
    }

    /*
    **	If there is a digest attached to this mixfile, then read it in and
    **	compare it to the generated digest. If they don't match, then
    **	return with the "failure to cache" error code.
    */
    if (IsDigest) {
      char digest1[20];
      char digest2[20];
      sha.Result(digest2);
      fstraw.Get(digest1, sizeof(digest1));
      if (memcmp(digest1, digest2, sizeof(digest1)) != 0) {
        delete[] Data;
        Data = nullptr;
        return (false);
      }
    }

    return (true);
  }
  IsAllocated = false;
  return (false);
}

/***********************************************************************************************
 * MixFileClass::Free -- Frees the allocated raw data block (not the index
 *block).             *
 *                                                                                             *
 *    This routine will free the (presumably large) raw data block, but leave
 *the index        * block intact. By using this in conjunction with the Cache()
 *function, one can maintain   * tight control of memory usage. If the index
 *block is desired to be freed, then the       * mixfile object must be deleted.
 **
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. *
 *=============================================================================================*/
template <class T>
void MixFileClass<T>::Free(void) {
  if (Data != nullptr && IsAllocated) {
    delete[] Data;
  }
  Data = nullptr;
  IsAllocated = false;
}

int compfunc(void const *ptr1, void const *ptr2) {
  if (*(int32_t const *)ptr1 < *(int32_t const *)ptr2) return (-1);
  if (*(int32_t const *)ptr1 > *(int32_t const *)ptr2) return (1);
  return (0);
}

/***********************************************************************************************
 * MixFileClass::Offset -- Determines the offset of the requested file from the
 *mixfile system.*
 *                                                                                             *
 *    This routine will take the filename specified and search through the
 *mixfile system      * looking for it. If the file was found, then the mixfile
 *it was found in, the offset      * from the start of the mixfile, and the size
 *of the embedded file will be returned.       * Using this method it is
 *possible for the CCFileClass system to process it as a normal    * file. *
 *                                                                                             *
 * INPUT:   filename    -- The filename to search for. *
 *                                                                                             *
 *          realptr     -- Stores a pointer to the start of the file in memory
 *here. If the    * file is not in memory, then nullptr is stored here. *
 *                                                                                             *
 *          mixfile     -- The pointer to the corresponding mixfile is placed
 *here. If no      * mixfile was found that contains the file, then nullptr is
 *stored here. *
 *                                                                                             *
 *          offset      -- The starting offset from the beginning of the parent
 *mixfile is     * stored here. *
 *                                                                                             *
 *          size        -- The size of the embedded file is stored here. *
 *                                                                                             *
 * OUTPUT:  bool; Was the file found? The file may or may not be resident, but
 *it does exist   * and can be opened. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/17/1994 JLB : Created. *
 *=============================================================================================*/
template <class T>
bool MixFileClass<T>::Offset(char const *filename, void **realptr,
                             MixFileClass **mixfile, long *offset, long *size) {
  MixFileClass<T> *ptr;

  if (filename == nullptr) {
    assert(filename != nullptr);  // BG
    return (false);
  }

  /*
  **	Create the key block that will be used to binary search for the file.
  */
  char *upperFilename = strupr(strdup(filename));
  long crc = Calculate_CRC(upperFilename, strlen(filename));
  free(upperFilename);
  SubBlock key;
  key.CRC = crc;

  /*
  **	Sweep through all registered mixfiles, trying to find the file in
  *question.
  */
  ptr = MixList.First();
  while (ptr->Is_Valid()) {
    SubBlock *block;

    /*
    **	Binary search for the file in this mixfile. If it is found, then extract
    *the *	appropriate information and store it in the locations provided
    *and then return.
    */
    block = (SubBlock *)bsearch(&key, ptr->HeaderBuffer, ptr->Count,
                                sizeof(SubBlock), compfunc);
    if (block != nullptr) {
      if (mixfile != nullptr) *mixfile = ptr;
      if (size != nullptr) *size = block->Size;
      if (realptr != nullptr) *realptr = nullptr;
      if (offset != nullptr) *offset = block->Offset;
      if (realptr != nullptr && ptr->Data != nullptr) {
        *realptr = (char *)ptr->Data + block->Offset;
      }
      if (ptr->Data == nullptr && offset != nullptr) {
        *offset += ptr->DataStart;
      }
      return (true);
    }

    /*
    **	Advance to next mixfile.
    */
    ptr = ptr->Next();
  }

  /*
  **	All the mixfiles have been examined but no match was found. Return with
  *the non success flag.
  */
  assert(1);  // BG
  return (false);
}

template class MixFileClass<CCFileClass>;
