/*
**	Command & Conquer(tm)
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

#ifndef TD_MIXFILE_H_
#define TD_MIXFILE_H_

#include <cstddef>
#include <cstdint>

#include "td/link.h"

// Manages MIX archive files (Westwood's packed game asset format).
//
// MIX files contain multiple sub-files indexed by CRC. Files are looked up by
// computing the CRC of the filename and binary searching the index. The raw
// data can be cached in RAM or read on-demand from disk.
//
// Example:
//   MixFileClass::Register("general.mix");
//   MixFileClass::Cache("general.mix");
//   void* data = MixFileClass::Retrieve("sounds.aud");
class MixFileClass : public LinkClass {
 public:
  char const *Filename;  // Filename of mixfile.

  ~MixFileClass(void);

  // Creates and registers a mixfile. Returns existing instance if already
  // registered, or nullptr on failure.
  static MixFileClass *Register(char const *filename);

  // Removes and deletes a mixfile by name. Returns true if found.
  static bool Unregister(char const *filename);

  // Frees cached data for the named mixfile. Returns true if found.
  static bool Free(char const *filename);

  // Frees all registered mixfiles.
  static void Free_All(void);

  // Frees this mixfile's cached data. Keeps index for re-caching.
  void Free(void);

  // Loads this mixfile's raw data into RAM. Returns true on success.
  bool Cache(void);

  // Loads the named mixfile's raw data into RAM. Returns true on success.
  static bool Cache(char const *filename);

  // Finds a file across all registered mixfiles. On success, outputs are set:
  // - realptr: pointer to data if cached, nullptr if on disk
  // - mixfile: the containing mixfile
  // - offset: byte offset from mixfile start (disk) or data block start (RAM)
  // - size: file size in bytes
  static bool Offset(char const *filename, void **realptr = nullptr,
                     MixFileClass **mixfile = nullptr, long *offset = nullptr,
                     long *size = nullptr);

  // Returns pointer to file data if cached in RAM, nullptr otherwise.
  static void const *Retrieve(char const *filename);

  struct SubBlock {
    int32_t CRC;     // CRC code for embedded file.
    int32_t Offset;  // Offset from start of data section.
    int32_t Size;    // Size of data subfile.

    int operator<(SubBlock &two) const { return (CRC < two.CRC); };
    int operator>(SubBlock &two) const { return (CRC > two.CRC); };
    int operator==(SubBlock &two) const { return (CRC == two.CRC); };
  };

 private:
  // Use Register() factory instead of direct construction.
  MixFileClass(char const *filename);

  // Searches registered mixfiles for one matching the filename suffix.
  static MixFileClass *Finder(char const *filename);

  long Offset(long crc, long *size = nullptr);

#pragma pack(push, 1)
  typedef struct {
    int16_t count;
    int32_t size;
  } FileHeader;
#pragma pack(pop)

  size_t Count;      // Number of sub-blocks in the index.
  long DataSize;     // Size of raw data section in bytes.
  SubBlock *Buffer;  // Index array of sub-blocks.
  void *Data;        // Cached raw data, or nullptr if not cached.

  static MixFileClass *First;  // Head of registered mixfile linked list.
};

#endif  // TD_MIXFILE_H_
