// File: the integer-handle file API declared in sdllib/file.h, for code
// outside the game (audio streaming, PCX writing) that cannot
// use GameFile directly. Shared by both games.

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string_view>

#include "base/array.h"
#include "base/seek_origin.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/wwstd.h"
#include "tech/game_file.h"

namespace {

// A handle is an index into this table, so at most this many files are open
// through the API at once.
GameFile handle_table[10];

// Returns the open file behind handle, or nullptr if handle is kInvalidHandle,
// out of range, or closed.
GameFile* OpenFileForHandle(int handle) {
  if (handle < 0 || handle >= std::ssize(handle_table) ||
      !base::At(handle_table, handle).IsOpen()) {
    return nullptr;
  }
  return base::Suffix(handle_table, handle).data();
}

}  // namespace

int __cdecl OpenFileHandle(const std::string_view file_name, FileAccess mode) {
  for (int handle = 0; handle < std::ssize(handle_table); handle++) {
    if (!base::At(handle_table, handle).IsOpen()) {
      if (base::At(handle_table, handle).Open(file_name, mode)) {
        return handle;
      }
      // The first free slot is as good as any other; a failed open would fail
      // in them all.
      break;
    }
  }
  return kInvalidHandle;
}

void __cdecl CloseFileHandle(int handle) {
  if (GameFile* const file = OpenFileForHandle(handle)) {
    file->Close();
  }
}

int32_t __cdecl ReadFileHandle(int handle, std::span<std::byte> buffer) {
  if (GameFile* const file = OpenFileForHandle(handle)) {
    return static_cast<int32_t>(file->Read(buffer));
  }
  return 0;
}

int32_t __cdecl WriteFileHandle(int handle, std::span<const std::byte> buffer) {
  if (GameFile* const file = OpenFileForHandle(handle)) {
    return static_cast<int32_t>(file->Write(buffer));
  }
  return 0;
}

bool __cdecl FileExists(const std::string_view file_name) {
  GameFile file(file_name);
  return file.IsAvailable();
}

int32_t __cdecl FileHandleSize(int handle) {
  if (GameFile* const file = OpenFileForHandle(handle)) {
    return static_cast<int32_t>(file->Size());
  }
  return 0;
}

int32_t __cdecl SeekFileHandle(int handle, int32_t offset, int origin) {
  if (GameFile* const file = OpenFileForHandle(handle)) {
    return static_cast<int32_t>(
        file->Seek(offset, SeekOriginFromStdio(origin)));
  }
  return 0;
}
