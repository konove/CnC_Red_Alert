// File source abstraction for the VQA player.

#ifndef CNC_RED_ALERT_WINVQ_VQA32_VQAIO_H_
#define CNC_RED_ALERT_WINVQ_VQA32_VQAIO_H_

#include <cstdint>

// Abstract file source the VQA player reads a movie through. Install on a
// handle with VQA_SetIo() before VQA_Open(). The player never owns the
// object; it must stay alive from Open() until Close() returns.
//
// All methods except Close() return 0 on success and nonzero on failure
// (the player maps failures to VQAERR_* codes).
class VqaIo {
 public:
  virtual ~VqaIo() = default;

  // Opens the named movie file for reading.
  virtual int Open(const char* filename) = 0;

  // Reads exactly `bytes` bytes into `buffer`. A short read is a failure.
  virtual int Read(void* buffer, int64_t bytes) = 0;

  // Moves the read position. `origin` is SEEK_CUR or SEEK_SET.
  virtual int Seek(int64_t offset, int origin) = 0;

  // Closes the file. Safe to call when no file is open.
  virtual void Close() = 0;
};

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQAIO_H_
