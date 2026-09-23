#ifndef CNC_RED_ALERT_SDLLIB_FILE_ACCESS_H_
#define CNC_RED_ALERT_SDLLIB_FILE_ACCESS_H_

#include <cstdint>

// File access rights used by the stream openers.
//
// These are bitmask flags: kRead and kWrite can be combined to request
// read-write access (kReadWrite is provided as a convenience).
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

#endif  // CNC_RED_ALERT_SDLLIB_FILE_ACCESS_H_
