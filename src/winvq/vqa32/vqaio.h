// File: VqaIo, the file source the VQA player reads movies through. The
// player never touches files itself; whoever plays a movie installs a VqaIo
// with VqaPlayer::SetIo(). The game's implementation is GameFileVqaIo in
// tech/game_file_vqa_io.h; tests use an in-memory fake.
#ifndef CNC_RED_ALERT_WINVQ_VQA32_VQAIO_H_
#define CNC_RED_ALERT_WINVQ_VQA32_VQAIO_H_

#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>

#include "base/buffer.h"
#include "base/numeric.h"
#include "base/seek_origin.h"
#include "base/types.h"

class VqaIo {
 public:
  VqaIo() = default;
  virtual ~VqaIo() = default;
  VqaIo(const VqaIo&) = delete;
  VqaIo& operator=(const VqaIo&) = delete;
  VqaIo(VqaIo&&) = delete;
  VqaIo& operator=(VqaIo&&) = delete;

  // Opens the named movie for reading. Returns false if it cannot be opened.
  virtual bool Open(std::string_view name) = 0;

  // Fills buffer completely. A short read is a failure and returns false.
  virtual bool Read(std::span<std::byte> buffer) = 0;

  // Moves the read position. Returns false if the position could not be set.
  virtual bool Seek(base::ssize offset, SeekOrigin origin) = 0;

  // Closes the movie. Safe to call when none is open.
  virtual void Close() = 0;

  // Reads one trivially copyable value, such as a chunk header.
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  bool ReadObject(T& value) {
    return Read(base::ObjectBytes(value));
  }

  // Reads count bytes into a byte-sized buffer, which is what the decoders'
  // scratch buffers are.
  template <typename T>
    requires(sizeof(T) == 1 && std::is_trivially_copyable_v<T>)
  bool Read(std::span<T> buffer, base::ssize count) {
    if (count < 0 || base::ToSize(count) > buffer.size()) {
      return false;
    }
    return Read(std::as_writable_bytes(buffer.first(base::ToSize(count))));
  }
};

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQAIO_H_
