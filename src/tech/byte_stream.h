#ifndef CNC_RED_ALERT_TECH_BYTE_STREAM_H_
#define CNC_RED_ALERT_TECH_BYTE_STREAM_H_

// File: ByteStream, a seekable source or sink of bytes with no name, and the
// three kinds the game composes: a file on disk, a block of memory, and a
// window onto another stream. A file inside a mixfile inside another mixfile
// is a RangeStream over a RangeStream over a DiskStream.

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>

#include "absl/base/attributes.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "sdllib/file_access.h"
#include "tech/file.h"

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

  // Returns the current position.
  base::ssize Tell() { return Seek(0, SeekOrigin::kCurrent); }
};

// A file on disk, open from construction until destruction.
class DiskStream final : public ByteStream {
 public:
  // Opens path with the given access and returns the stream, or nullptr if
  // the file could not be opened. Write access creates or truncates the
  // file; read-write access keeps its contents.
  static std::unique_ptr<DiskStream> Open(std::string_view path,
                                          FileAccess access);

  ~DiskStream() override;

  DiskStream(const DiskStream&) = delete;
  DiskStream& operator=(const DiskStream&) = delete;
  DiskStream(DiskStream&&) = delete;
  DiskStream& operator=(DiskStream&&) = delete;

  base::ssize Read(std::span<std::byte> buffer) override;
  base::ssize Write(std::span<const std::byte> buffer) override;

  // A seek to before the start of the file leaves the position where it was,
  // as stdio does; a seek past the end is allowed, and a write there extends
  // the file.
  base::ssize Seek(base::ssize offset,
                   SeekOrigin origin = SeekOrigin::kCurrent) override;
  base::ssize Size() override;

 private:
  explicit DiskStream(void* handle ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : handle_(handle) {}

  // Low-level IO handle from IO_Open_File; never null.
  void* handle_;
};

// A read-only view of bytes that someone else owns and keeps alive for as
// long as the stream is used, such as a file inside a cached mixfile.
class MemoryStream final : public ByteStream {
 public:
  explicit MemoryStream(
      std::span<const std::byte> bytes ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : bytes_(bytes) {}

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

  base::ssize Read(std::span<std::byte> buffer) override;
  base::ssize Write(std::span<const std::byte> /*buffer*/) override {
    return 0;
  }

  // The position is clamped to [0, Size()].
  base::ssize Seek(base::ssize offset,
                   SeekOrigin origin = SeekOrigin::kCurrent) override;
  base::ssize Size() override { return size_; }

 private:
  std::unique_ptr<ByteStream> inner_;

  // Where the window starts in inner_.
  base::ssize offset_;

  // Length of the window.
  base::ssize size_;

  // Current position within the window.
  base::ssize position_ = 0;
};

#endif  // CNC_RED_ALERT_TECH_BYTE_STREAM_H_
