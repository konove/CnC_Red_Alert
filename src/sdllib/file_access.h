#ifndef CNC_RED_ALERT_SDLLIB_FILE_ACCESS_H_
#define CNC_RED_ALERT_SDLLIB_FILE_ACCESS_H_

// File access rights used by FileClass::Open() and IO_Open_File().
//
// These are bitmask flags: kRead and kWrite can be combined with bitwise OR
// to request read-write access (kReadWrite is provided as a convenience).
//
// Example:
//   file.Open(FileAccess::kRead);
//   file.Open(FileAccess::kReadWrite);
enum class FileAccess : int {
  kRead = 1,
  kWrite = 2,
  kReadWrite = 3,
};

constexpr FileAccess operator|(FileAccess lhs, FileAccess rhs) {
  return static_cast<FileAccess>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

constexpr FileAccess operator&(FileAccess lhs, FileAccess rhs) {
  return static_cast<FileAccess>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

// Returns true if any flags in `test` are set in `rights`.
constexpr bool HasAccess(FileAccess rights, FileAccess test) {
  return (static_cast<int>(rights) & static_cast<int>(test)) != 0;
}

#endif  // CNC_RED_ALERT_SDLLIB_FILE_ACCESS_H_
