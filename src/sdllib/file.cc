#include "sdllib/file.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "sdllib/file_access.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <fnmatch.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#endif

void* IO_Open_File(const char* filename, FileAccess mode) {
  switch (mode) {
    case FileAccess::kRead:
      return fopen(filename, "rb");
    case FileAccess::kWrite:
      return fopen(filename, "wb");
    case FileAccess::kReadWrite: {
      // "w+b" would empty an existing file; read-write access means keeping
      // its contents (the record file appends to itself). Only create the
      // file when there is nothing to keep.
      if (FILE* const file = fopen(filename, "r+b")) {
        return file;
      }
      return fopen(filename, "w+b");
    }
    default:
      return nullptr;
  }
}

void IO_Close_File(void* handle) {
  auto* file = static_cast<FILE*>(handle);
  fclose(file);
}

bool IO_Read_File(void* handle, void* buffer, size_t count,
                  size_t& actual_read) {
  auto* file = static_cast<FILE*>(handle);
  actual_read = fread(buffer, 1, count, file);
  return ferror(file) == 0;
}

bool IO_Write_File(void* handle, const void* buffer, size_t count,
                   size_t& actual_written) {
  auto* file = static_cast<FILE*>(handle);
  actual_written = fwrite(buffer, 1, count, file);
  return ferror(file) == 0;
}

int64_t IO_Seek_File(void* handle, int64_t offset, int origin) {
  auto* file = static_cast<FILE*>(handle);
  fseek(file, offset, origin);
  return ftell(file);
}

int64_t IO_Get_File_Size(void* handle) {
  auto* file = static_cast<FILE*>(handle);
  const int64_t pos = ftell(file);

  fseek(file, 0, SEEK_END);

  const int64_t length = ftell(file);

  fseek(file, pos, SEEK_SET);

  return length;
}

bool IO_Delete_File(const char* filename) { return unlink(filename) == 0; }

#ifdef _WIN32
static bool Update_Find_Result(FindFileState& state, WIN32_FIND_DATA& data) {
  // skip hidden/system/dir

  bool success = true;
  while (success && (data.dwFileAttributes &
                     (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN |
                      FILE_ATTRIBUTE_SYSTEM))) {
    success = FindNextFile((HANDLE)state.data, &data);
  }

  if (!success) {
    return false;
  }

  state.name = strdup(data.cFileName);

  ULARGE_INTEGER big;
  big.LowPart = data.ftLastWriteTime.dwLowDateTime;
  big.HighPart = data.ftLastWriteTime.dwHighDateTime;
  state.mod_time = big.QuadPart / 10000000ULL - 11644473600ULL;

  return true;
}

bool Find_First_File(const char* path_glob, FindFileState& state) {
  WIN32_FIND_DATA data;
  auto handle = FindFirstFile(path_glob, &data);

  if (handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  state.data = handle;

  if (!Update_Find_Result(state, data)) {
    FindClose(handle);
    state.data = NULL;
    return false;
  }

  return true;
}

bool Find_Next_File(FindFileState& state) {
  WIN32_FIND_DATA data;

  // free old filename
  free((char*)state.name);
  state.name = NULL;

  if (!FindNextFile((HANDLE)state.data, &data) ||
      !Update_Find_Result(state, data)) {
    FindClose((HANDLE)state.data);
    state.data = NULL;
    return false;
  }
  return true;
}

void End_Find_File(FindFileState& state) {
  if (state.name) {
    free((char*)state.name);
    state.name = NULL;
  }

  if (state.data) {
    FindClose((HANDLE)state.data);
    state.data = NULL;
  }
}

uint64_t Disk_Space_Available() {
  ULARGE_INTEGER space;
  if (GetDiskFreeSpaceEx(NULL, &space, NULL, NULL)) {
    return space.QuadPart;
  }

  return 0;
}
#else

// The names matching the pattern, in sorted order, and the index of the one
// `state.name` refers to.
struct FindFileMatches {
  std::vector<std::string> names;
  size_t offset = 0;
};

// Advances `state` to the next match that is a regular file (or anything
// but a directory) and fills in its name and modification time. Returns false
// once the matches are used up.
static bool Update_Find_Result(FindFileState& state) {
  auto* const matches = static_cast<FindFileMatches*>(state.data);
  struct stat stat_buf{};

  while (matches->offset < matches->names.size()) {
    const std::string& current = matches->names[matches->offset];

    // A name that cannot be stat'ed (broken symlink, permission denied) or
    // names a directory is skipped.
    if (stat(current.c_str(), &stat_buf) != 0 || S_ISDIR(stat_buf.st_mode)) {
      matches->offset++;
      continue;
    }

    state.mod_time = stat_buf.st_mtime;
    state.name = current.c_str();
    return true;
  }

  return false;
}

bool Find_First_File(const char* path_glob, FindFileState& state) {
  // The patterns are bare names ("SC*.MIX", "SAVEGAME.*") matched in the
  // working directory. The game's files came from a case-insensitive
  // filesystem, so the match ignores case.
  auto* matches = new FindFileMatches;
  std::error_code error;
  for (const auto& entry : std::filesystem::directory_iterator(".", error)) {
    const std::string name = entry.path().filename().string();
    if (fnmatch(path_glob, name.c_str(), FNM_CASEFOLD) == 0) {
      matches->names.push_back(name);
    }
  }
  std::ranges::sort(matches->names);

  state.data = matches;
  state.offset = 0;

  if (!Update_Find_Result(state)) {
    delete matches;
    state.data = nullptr;
    return false;
  }

  return true;
}

bool Find_Next_File(FindFileState& state) {
  auto* const matches = static_cast<FindFileMatches*>(state.data);
  if (!matches) {
    return true;
  }

  matches->offset++;
  if (!Update_Find_Result(state)) {
    delete matches;
    state.data = nullptr;
    return false;
  }

  return true;
}

void End_Find_File(FindFileState& state) {
  if (state.data) {
    delete static_cast<FindFileMatches*>(state.data);
    state.data = nullptr;
  }
}

uint64_t Disk_Space_Available() {
  struct statvfs fsbuf{};
  char path[1024];
  if (!getcwd(path, 1000)) {
    return 0;
  }

  if (statvfs(path, &fsbuf) < 0) {
    return 0;
  }

  return fsbuf.f_bavail * fsbuf.f_bsize;
}
#endif
