#ifndef CNC_RED_ALERT_ENGINE_FILE_FILE_ACCESS_H_
#define CNC_RED_ALERT_ENGINE_FILE_FILE_ACCESS_H_

#include <cstdint>

// File: the access rights the stream openers take (DiskStream::Open,
// OpenDiskFile, OpenGameFile).
//
// The values are bit flags, so HasAccess(access, FileAccess::kWrite) is true
// for both kWrite and kReadWrite.
//
// Example:
//   OpenDiskFile(path, FileAccess::kRead);
//   OpenDiskFile(path, FileAccess::kReadWrite);
enum class FileAccess : uint32_t {
  kRead = 1,
  kWrite = 2,
  kReadWrite = 3,
};

// Returns true if any flags in `test` are set in `rights`.
constexpr bool HasAccess(FileAccess rights, FileAccess test) {
  return (static_cast<uint32_t>(rights) & static_cast<uint32_t>(test)) != 0;
}

#endif  // CNC_RED_ALERT_ENGINE_FILE_FILE_ACCESS_H_
