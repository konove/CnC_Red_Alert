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

#include "td/function.h"
#include "td/mixfile.h"

template <class T>
int Compare(T const *obj1, T const *obj2) {
  if (*obj1 < *obj2) return (-1);
  if (*obj1 > *obj2) return (1);
  return (0);
};

// Head of the global linked list of registered mixfiles.
MixFileClass *MixFileClass::First = nullptr;

// Looks up the mixfile by name and delegates to the instance Free() method.
bool MixFileClass::Free(char const *filename) {
  MixFileClass *ptr = Finder(filename);

  if (ptr) {
    ptr->Free();
    return (true);
  }
  return (false);
}

#ifndef NOMEMCHECK
void MixFileClass::Free_All(void) {
  while (First) {
    delete First;
  }
}
#endif

// Frees all allocated memory (filename, data, index buffer) and removes this
// mixfile from the global linked list.
MixFileClass::~MixFileClass(void) {
  // Deallocate any allocated memory.
  if (Filename) {
    free((char *)Filename);
  }
  if (Data) {
    delete[] Data;
  }
  if (Buffer) {
    delete[] Buffer;
  }

  // Unlink from the global mixfile chain.
  if (this == First) {
    First = (MixFileClass *)Get_Next();
  } else {
    Remove();
  }
  Zap();
}

// Factory method: returns existing instance if already registered, otherwise
// creates a new MixFileClass and appends it to the global linked list.
MixFileClass *MixFileClass::Register(char const *filename) {
  // Check if already registered
  MixFileClass *existing = Finder(filename);
  if (existing != nullptr) {
    return existing;
  }

  // Create new instance
  const auto mix = new MixFileClass(filename);

  // Register in linked list
  if (First == nullptr) {
    First = mix;
  } else {
    mix->Add_Tail(*First);
  }
  return mix;
}

// Finds the mixfile by name and deletes it. The destructor handles list
// removal.
bool MixFileClass::Unregister(char const *filename) {
  MixFileClass *mix = Finder(filename);
  if (mix) {
    delete mix;  // Destructor handles list removal
    return true;
  }
  return false;
}

// Opens the mixfile and reads the FileHeader and SubBlock index array.
// Does NOT load the raw data (call Cache() for that). Link pointers are
// initialized but registration in the global list happens in Register().
MixFileClass::MixFileClass(const char *filename) {
  CCFileClass file;

  // Initialize members and read the header.
  Data = nullptr;
  Count = 0;
  Buffer = nullptr;
  file.Set_Name(filename);
  Filename = strdup(file.File_Name());

  if (!Force_CD_Available(RequiredCD)) {
    Prog_End();
    exit(EXIT_FAILURE);
  }

  if (file.Is_Available(true)) {
    FileHeader fileheader;

    file.Open();
    file.Read(&fileheader, sizeof(fileheader));
    Count = fileheader.count;
    DataSize = fileheader.size;

    // Read the SubBlock index array.
    Buffer = new SubBlock[Count];
    file.Read(Buffer, Count * sizeof(SubBlock));
    file.Close();
  } else {
    return;
  }

  // Raw data starts uncached; call Cache() to load it.
  Data = nullptr;

  // Initialize link pointers (registration happens in Register() factory)
  Zap();
}

// Thin wrapper around Offset() that returns just the data pointer.
const void *MixFileClass::Retrieve(const char *filename) {
  void *ptr = nullptr;
  Offset(filename, &ptr);
  return (ptr);
}

// Searches registered mixfiles by suffix-matching the filename. This allows
// matching "general.mix" even if stored as "c:\data\general.mix".
MixFileClass *MixFileClass::Finder(const char *filename) {
  MixFileClass *ptr;

  ptr = First;
  while (ptr) {
    if (strlen(ptr->Filename) < strlen(filename)) {
      ptr = (MixFileClass *)ptr->Get_Next();
      continue;
    }

    if (stricmp(&ptr->Filename[strlen(ptr->Filename) - strlen(filename)],
                filename) == 0) {
      return (ptr);
    }
    ptr = (MixFileClass *)ptr->Get_Next();
  }
  return (nullptr);
}

// Looks up the mixfile by name and delegates to the instance Cache() method.
bool MixFileClass::Cache(char const *filename) {
  MixFileClass *mixer = Finder(filename);

  if (mixer) {
    return (mixer->Cache());
  }
  return (false);
}

// Allocates the Data buffer and reads the raw data section from disk.
// Seeks past the FileHeader and SubBlock index to reach the data. Returns
// immediately if already cached.
bool MixFileClass::Cache(void) {
  if (Data) return (true);

  Data = new char[DataSize];
  if (Data) {
    CCFileClass file(Filename);

    file.Open();
    file.Seek(sizeof(SubBlock) * Count + sizeof(FileHeader));
    long actual = file.Read(Data, DataSize);
    if (actual != DataSize) {
#ifdef GERMAN
      Fatal(
          "Korrupte .MIX-Datei \"%s\". Beim Versuch, %ld zu lesen, nur %ld "
          "gefunden.",
          Filename, DataSize, actual);
#else
#ifdef FRENCH
      Fatal(
          "Fichier .MIX corrumpu \"%s\". Essai de lecture de %ld, mais %ld "
          "obtenu.",
          Filename, DataSize, actual);
#else
      Fatal("Corrupt .MIX file \"%s\". Tried to read %ld, but got %ld.",
            Filename, DataSize, actual);
#endif
#endif
    }
    file.Close();
    return (true);
  }
#ifdef GERMAN
  Fatal("Kann Datei \"%s\" nicht laden.", Filename);
#else
#ifdef FRENCH
  Fatal("Impossible de charger \"%s\".", Filename);
#else
  Fatal("Unable to load \"%s\".", Filename);
#endif
#endif
  return (false);
}

// Frees only the raw data buffer, keeping the SubBlock index. This allows
// re-caching later without re-reading the index from disk.
void MixFileClass::Free(void) {
  if (Data) {
    delete[] Data;
    Data = nullptr;
  }
}

// Comparison function for bsearch() on SubBlock CRC values.
int compfunc(void const *ptr1, void const *ptr2) {
  if (*(int32_t const *)ptr1 < *(int32_t const *)ptr2) return (-1);
  if (*(int32_t const *)ptr1 > *(int32_t const *)ptr2) return (1);
  return (0);
}

// Searches all registered mixfiles for a file by computing its CRC and doing
// a binary search on each mixfile's SubBlock index. If found, populates the
// output parameters. For uncached mixfiles, the offset is adjusted to include
// the header and index size so it can be used for direct file seeks.
bool MixFileClass::Offset(char const *filename, void **realptr,
                          MixFileClass **mixfile, long *offset, long *size) {
  MixFileClass *ptr;

  if (!filename) return (false);

  // Compute CRC of uppercase filename for index lookup.
  char *upperFilename = strupr(strdup(filename));
  long crc = Calculate_CRC(upperFilename, strlen(filename));
  free(upperFilename);
  SubBlock key;
  key.CRC = crc;

  // Search each registered mixfile.
  ptr = First;
  while (ptr) {
    SubBlock *block;

    // Binary search the index for matching CRC.
    block = (SubBlock *)bsearch(&key, ptr->Buffer, ptr->Count, sizeof(SubBlock),
                                compfunc);
    if (block) {
      if (mixfile) *mixfile = ptr;
      if (size) *size = block->Size;
      if (realptr) *realptr = nullptr;
      if (offset) *offset = block->Offset;
      if (realptr && ptr->Data) {
        *realptr = Add_Long_To_Pointer(ptr->Data, block->Offset);
      }
      // For uncached files, adjust offset to account for header + index.
      if (!ptr->Data && offset) {
        *offset += sizeof(SubBlock) * ptr->Count + sizeof(FileHeader);
      }
      return (true);
    }

    ptr = (MixFileClass *)ptr->Get_Next();
  }

  return (false);
}
