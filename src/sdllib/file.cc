#include "sdllib/include/file.h"

#include <cstdio>
#include <string>

#include "absl/strings/ascii.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <glob.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#endif

void* IO_Open_File(const char* filename, int mode) {
  const char* mode_str;

  if (mode == READ) {
    mode_str = "rb";
  } else if (mode == WRITE) {
    mode_str = "wb";
  } else if (mode == (READ | WRITE)) {
    mode_str = "w+b";
  } else {
    return nullptr;
  }

  return fopen(filename, mode_str);
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

size_t IO_Seek_File(void* handle, size_t offset, int origin) {
  auto* file = static_cast<FILE*>(handle);
  fseek(file, offset, origin);
  return ftell(file);
}

size_t IO_Get_File_Size(void* handle) {
  auto* file = static_cast<FILE*>(handle);
  long pos = ftell(file);

  fseek(file, 0, SEEK_END);

  long length = ftell(file);

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

static bool Update_Find_Result(FindFileState& state) {
  auto* const glob_buf = static_cast<glob_t*>(state.data);
  struct stat stat_buf;

  // Iterate through paths until we find a valid file or run out of items
  while (state.offset < glob_buf->gl_pathc) {
    const char* current_path = glob_buf->gl_pathv[state.offset];

    // 1. Try to stat the file.
    // If stat fails (e.g., broken symlink, permission denied), skip this item.
    if (stat(current_path, &stat_buf) != 0) {
      state.offset++;
      continue;
    }

    // 2. If it is a directory, skip it.
    if (S_ISDIR(stat_buf.st_mode)) {
      state.offset++;
      continue;
    }

    // 3. Success: We found a valid non-directory file, and stat_buf is
    // populated. Populate the state and return true.
    state.mod_time = stat_buf.st_mtim.tv_sec;
    state.name = current_path;

    // Note: We leave state.offset pointing to this current valid item.
    return true;
  }

  // We reached the end of the list without finding a valid file.
  return false;
}

bool Find_First_File(const char* path_glob, FindFileState& state) {
  auto* glob_buf = new glob_t;
  int ret = glob(path_glob, GLOB_MARK, nullptr, glob_buf);

  // also search for lowercase filenames
  if (ret == 0 || ret == GLOB_NOMATCH) {
    std::string lower_glob = absl::AsciiStrToLower(path_glob);
    int ret2 =
        glob(lower_glob.c_str(), GLOB_MARK | GLOB_APPEND, nullptr, glob_buf);
    if (ret2 != GLOB_NOMATCH) {
      ret = ret2;
    }
  }

  if (ret) {
    delete glob_buf;
    return false;
  }

  state.offset = 0;
  state.data = glob_buf;

  if (!Update_Find_Result(state)) {
    globfree(glob_buf);
    delete glob_buf;
    return false;
  }

  return true;
}

bool Find_Next_File(FindFileState& state) {
  // increment offset
  state.offset++;
  auto* glob_buf = static_cast<glob_t*>(state.data);

  if (!glob_buf) {
    return 2;
  }

  if (!Update_Find_Result(state)) {
    globfree(glob_buf);
    delete glob_buf;
    state.data = nullptr;
    return false;
  }

  return true;
}

void End_Find_File(FindFileState& state) {
  if (state.data) {
    auto* glob_buf = static_cast<glob_t*>(state.data);
    globfree(glob_buf);
    delete glob_buf;
    state.data = nullptr;
  }
}

uint64_t Disk_Space_Available() {
  struct statvfs fsbuf;
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
