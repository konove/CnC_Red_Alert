#ifndef CNC_RED_ALERT_ENGINE_STREAM_BYTE_STREAM_H_
#define CNC_RED_ALERT_ENGINE_STREAM_BYTE_STREAM_H_

// File: ByteStream, a seekable source or sink of bytes with no name. The
// three kinds the game composes each have their own header: DiskStream (a file
// on disk, engine/file/disk_stream.h), MemoryStream (a block of memory,
// engine/stream/memory_stream.h) and RangeStream (a window onto another
// stream, engine/stream/range_stream.h). A file inside a mixfile inside
// another mixfile is a RangeStream over a RangeStream over a DiskStream.

#include <cstddef>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "absl/log/check.h"
#include "engine/base/buffer.h"
#include "engine/base/numeric.h"
#include "engine/base/types.h"
#include "engine/stream/seek_origin.h"

// A seekable sequence of bytes. Positions are measured from the start of the
// stream.
class ByteStream {
 public:
  ByteStream() = default;
  virtual ~ByteStream() = default;

  ByteStream(const ByteStream&) = delete;
  ByteStream& operator=(const ByteStream&) = delete;
  ByteStream(ByteStream&&) = delete;
  ByteStream& operator=(ByteStream&&) = delete;

  // Reads up to buffer.size() bytes at the current position and returns the
  // number read, which is smaller only at the end of the stream.
  virtual base::ssize Read(std::span<std::byte> buffer) = 0;

  // Writes buffer at the current position and returns the number of bytes
  // written. A read-only stream writes nothing and returns 0.
  virtual base::ssize Write(std::span<const std::byte> buffer) = 0;

  // Moves the position by offset from origin and returns the new position.
  virtual base::ssize Seek(base::ssize offset,
                           SeekOrigin origin = SeekOrigin::kCurrent) = 0;

  // Returns the number of bytes in the stream.
  virtual base::ssize Size() = 0;

  // Returns false once a read or write has failed. Reaching the end of the
  // stream is not a failure.
  [[nodiscard]] virtual bool ok() const { return true; }

  // Pushes buffered writes to the operating system. Returns ok().
  virtual bool Flush() { return ok(); }

  // Returns the current position.
  base::ssize Tell() { return Seek(0, SeekOrigin::kCurrent); }

  // Reads one trivially copyable value. Returns false on a short read, in
  // which case value is partially written.
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  bool ReadObject(T& value) {
    return Read(base::ObjectBytes(value)) == base::ToSigned(sizeof(T));
  }

  // Writes one trivially copyable value. Returns false on a short write.
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  bool WriteObject(const T& value) {
    return Write(base::ObjectBytes(value)) == base::ToSigned(sizeof(T));
  }

  // Reads up to count bytes; the result is shorter at the end of the stream.
  std::vector<std::byte> ReadBytes(base::ssize count);

  // Reads up to count bytes as text; the result is shorter at the end of the
  // stream.
  std::string ReadString(base::ssize count);

  // Typed spans of trivially copyable elements. The count returned is still
  // in bytes. A derived class that overrides the std::byte forms needs
  // "using ByteStream::Read;" and "using ByteStream::Write;" to keep these
  // visible.
  template <typename T, std::size_t N>
    requires(std::is_trivially_copyable_v<T> &&
             !std::is_same_v<std::remove_cv_t<T>, std::byte>)
  base::ssize Read(std::span<T, N> buffer) {
    return Read(std::as_writable_bytes(buffer));
  }
  template <typename T, std::size_t N>
    requires(std::is_trivially_copyable_v<T> &&
             !std::is_same_v<std::remove_cv_t<T>, std::byte>)
  base::ssize Write(std::span<T, N> buffer) {
    return Write(std::as_bytes(buffer));
  }

  // Reads or writes a byte-counted prefix of an existing bounded view.
  // count is always measured in bytes, including for wider element types.
  template <typename T, std::size_t N>
    requires(std::is_trivially_copyable_v<T> && !std::is_const_v<T>)
  base::ssize Read(std::span<T, N> buffer, base::ssize count) {
    CHECK_GE(count, 0);
    CHECK_LE(base::ToSize(count), buffer.size_bytes());
    return Read(std::as_writable_bytes(buffer).first(base::ToSize(count)));
  }
  template <typename T, std::size_t N>
    requires std::is_trivially_copyable_v<T>
  base::ssize Write(std::span<T, N> buffer, base::ssize count) {
    CHECK_GE(count, 0);
    CHECK_LE(base::ToSize(count), buffer.size_bytes());
    return Write(std::as_bytes(buffer).first(base::ToSize(count)));
  }

  // A character buffer and a byte count, for the many callers that read text
  // or raw bytes into a char array. Only byte-sized element types are
  // accepted, so the count cannot be misread as elements; use ReadObject()
  // or a span for anything else.
  template <typename T, std::size_t N>
    requires(sizeof(T) == 1 && std::is_trivially_copyable_v<T>)
  base::ssize Read(T (&buffer)[N], base::ssize count) {
    CHECK_GE(count, 0);
    CHECK_LE(base::ToSize(count), N);
    return Read(std::span(buffer).first(base::ToSize(count)));
  }
  template <typename T, std::size_t N>
    requires(sizeof(T) == 1 && std::is_trivially_copyable_v<T>)
  base::ssize Write(const T (&buffer)[N], base::ssize count) {
    CHECK_GE(count, 0);
    CHECK_LE(base::ToSize(count), N);
    return Write(std::span(buffer).first(base::ToSize(count)));
  }
};

// Returns position moved by offset from origin, clamped to [0, size]: the
// seek of a stream that cannot move outside its bytes.
base::ssize ClampedSeek(base::ssize position, base::ssize size,
                        base::ssize offset, SeekOrigin origin);

#endif  // CNC_RED_ALERT_ENGINE_STREAM_BYTE_STREAM_H_
