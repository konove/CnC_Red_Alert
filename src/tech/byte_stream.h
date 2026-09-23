#ifndef CNC_RED_ALERT_TECH_BYTE_STREAM_H_
#define CNC_RED_ALERT_TECH_BYTE_STREAM_H_

// File: ByteStream, a seekable source or sink of bytes with no name, and the
// three kinds the game composes: a file on disk, a block of memory, and a
// window onto another stream. A file inside a mixfile inside another mixfile
// is a RangeStream over a RangeStream over a DiskStream.

#include <cstddef>
#include <fstream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/log/check.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "sdllib/file_access.h"

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

// A file on disk, open from construction until destruction.
class DiskStream final : public ByteStream {
 public:
  // Opens path with the given access and returns the stream, or nullptr if
  // the file could not be opened. Write access creates or truncates the
  // file; read-write access keeps its contents.
  static std::unique_ptr<DiskStream> Open(std::string_view path,
                                          FileAccess access);

  ~DiskStream() override = default;

  DiskStream(const DiskStream&) = delete;
  DiskStream& operator=(const DiskStream&) = delete;
  DiskStream(DiskStream&&) = delete;
  DiskStream& operator=(DiskStream&&) = delete;

  using ByteStream::Read;
  using ByteStream::Write;
  base::ssize Read(std::span<std::byte> buffer) override;
  base::ssize Write(std::span<const std::byte> buffer) override;

  // A seek to before the start of the file leaves the position where it was,
  // as stdio does; a seek past the end is allowed, and a write there extends
  // the file.
  base::ssize Seek(base::ssize offset,
                   SeekOrigin origin = SeekOrigin::kCurrent) override;
  base::ssize Size() override;
  [[nodiscard]] bool ok() const override { return !failed_; }

 private:
  DiskStream() = default;

  // The open file. A filebuf reports a failed write as a short sputn, and a
  // failed read by throwing std::ios_base::failure instead of returning a
  // short count; DiskStream::Read catches that and turns it into ok()
  // reporting false, same as a failed write.
  std::filebuf file_;

  // Set when a read or write fails.
  bool failed_ = false;
};

// A read-only view of bytes that someone else owns and keeps alive for as
// long as the stream is used, such as a file inside a cached mixfile.
class MemoryStream final : public ByteStream {
 public:
  explicit MemoryStream(
      std::span<const std::byte> bytes ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : bytes_(bytes) {}

  using ByteStream::Read;
  using ByteStream::Write;
  base::ssize Read(std::span<std::byte> buffer) override;
  base::ssize Write(std::span<const std::byte> /*buffer*/) override {
    return 0;
  }

  // The position is clamped to [0, Size()].
  base::ssize Seek(base::ssize offset,
                   SeekOrigin origin = SeekOrigin::kCurrent) override;
  base::ssize Size() override { return std::ssize(bytes_); }

 private:
  std::span<const std::byte> bytes_;
  base::ssize position_ = 0;
};

// A read-only window of size bytes starting offset bytes into another
// stream, which it owns. Reads never leave the window, and positions are
// relative to its start, so the window behaves as a whole stream of its own.
class RangeStream final : public ByteStream {
 public:
  // The window is clipped to what inner actually holds.
  RangeStream(std::unique_ptr<ByteStream> inner, base::ssize offset,
              base::ssize size);

  using ByteStream::Read;
  using ByteStream::Write;
  base::ssize Read(std::span<std::byte> buffer) override;
  base::ssize Write(std::span<const std::byte> /*buffer*/) override {
    return 0;
  }

  // The position is clamped to [0, Size()].
  base::ssize Seek(base::ssize offset,
                   SeekOrigin origin = SeekOrigin::kCurrent) override;
  base::ssize Size() override { return size_; }
  [[nodiscard]] bool ok() const override { return !failed_ && inner_->ok(); }

 private:
  std::unique_ptr<ByteStream> inner_;

  // Set when inner_ could not be positioned for a read.
  bool failed_ = false;

  // Where the window starts in inner_.
  base::ssize offset_;

  // Length of the window.
  base::ssize size_;

  // Current position within the window.
  base::ssize position_ = 0;

  // Where inner_ was left by our last read, or -1 when that is unknown --
  // before the first read, and after any read that did not land where it was
  // asked to. A read only repositions inner_ when it disagrees with this.
  base::ssize inner_position_ = -1;
};

#endif  // CNC_RED_ALERT_TECH_BYTE_STREAM_H_
