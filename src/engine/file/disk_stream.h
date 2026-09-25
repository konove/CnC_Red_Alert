#ifndef CNC_RED_ALERT_ENGINE_FILE_DISK_STREAM_H_
#define CNC_RED_ALERT_ENGINE_FILE_DISK_STREAM_H_

// File: DiskStream, a ByteStream over one file on disk.

#include <cstddef>
#include <fstream>
#include <memory>
#include <span>
#include <string_view>

#include "base/types.h"
#include "engine/file/file_access.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/seek_origin.h"

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
  bool Flush() override;
  [[nodiscard]] bool ok() const override { return !failed_; }

 private:
  DiskStream() = default;

  // The open file. A filebuf reports a failed write as a short sputn, and a
  // failed read by throwing std::ios_base::failure instead of returning a
  // short count; DiskStream::Read catches that and turns it into ok()
  // reporting false, same as a failed write. pubsync() (Flush()) can throw
  // the same way on some errors and is caught the same way.
  std::filebuf file_;

  // Set when a read, write or flush fails.
  bool failed_ = false;
};

#endif  // CNC_RED_ALERT_ENGINE_FILE_DISK_STREAM_H_
