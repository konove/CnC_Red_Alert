// File: the DiskStream implementation, over std::filebuf.

#include "tech/disk_stream.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <span>
#include <string_view>
#include <system_error>
#include <utility>

#include "engine/base/bytes_of.h"
#include "engine/base/types.h"
#include "engine/stream/seek_origin.h"
#include "tech/file_access.h"

namespace {
std::ios_base::seekdir SeekDir(const SeekOrigin origin) {
  switch (origin) {
    case SeekOrigin::kBegin:
      return std::ios_base::beg;
    case SeekOrigin::kEnd:
      return std::ios_base::end;
    case SeekOrigin::kCurrent:
    default:
      return std::ios_base::cur;
  }
}

base::ssize ToPosition(const std::streampos position) {
  return static_cast<base::ssize>(static_cast<std::streamoff>(position));
}
}  // namespace

std::unique_ptr<DiskStream> DiskStream::Open(const std::string_view path,
                                             const FileAccess access) {
  const std::filesystem::path file_path(path);
  // A directory opens as a file on POSIX; reading it then throws instead of
  // returning an error code. Refusing it here, rather than letting the first
  // read throw, is also what keeps IsAvailable()/FindExistingFile() from
  // treating a directory as though it were a file that merely can't be read.
  std::error_code error;
  if (std::filesystem::is_directory(file_path, error)) {
    return nullptr;
  }
  auto stream = std::unique_ptr<DiskStream>(new DiskStream);
  std::filebuf& file = stream->file_;
  constexpr std::ios_base::openmode kBinary = std::ios_base::binary;
  bool opened = false;
  switch (access) {
    case FileAccess::kRead:
      opened = file.open(file_path, std::ios_base::in | kBinary) != nullptr;
      break;
    case FileAccess::kWrite:
      opened = file.open(file_path, std::ios_base::out | std::ios_base::trunc |
                                        kBinary) != nullptr;
      break;
    case FileAccess::kReadWrite:
      // in|out keeps the contents (the record file appends to itself) but
      // needs the file to exist; create it only when there is nothing to keep.
      opened =
          file.open(file_path, std::ios_base::in | std::ios_base::out |
                                   kBinary) != nullptr ||
          file.open(file_path, std::ios_base::in | std::ios_base::out |
                                   std::ios_base::trunc | kBinary) != nullptr;
      break;
    default:
      break;
  }
  return opened ? std::move(stream) : nullptr;
}

base::ssize DiskStream::Read(const std::span<std::byte> buffer) {
  try {
    return file_.sgetn(port::CharBytes(buffer).data(), std::ssize(buffer));
  } catch (const std::ios_base::failure&) {
    // A real I/O error (EIO from a failing disk, a dropped network share)
    // throws instead of returning a short count; report it through ok(), as
    // the stdio-backed implementation did by checking ferror().
    failed_ = true;
    return 0;
  }
}

base::ssize DiskStream::Write(const std::span<const std::byte> buffer) {
  const base::ssize written =
      file_.sputn(port::CharBytes(buffer).data(), std::ssize(buffer));
  if (written != std::ssize(buffer)) {
    failed_ = true;
  }
  return written;
}

base::ssize DiskStream::Seek(const base::ssize offset,
                             const SeekOrigin origin) {
  // A write still buffered by the filebuf can fail to flush; pubseekoff
  // would then return -1, which Seek would otherwise read as "seek before
  // the start" and silently report the old position instead of the write
  // failure. Flush first, the same way Flush() itself does, so a failed
  // write surfaces through ok().
  Flush();
  const std::streampos moved = file_.pubseekoff(offset, SeekDir(origin));
  if (static_cast<std::streamoff>(moved) == -1) {
    // A seek to before the start fails and leaves the position alone, as
    // fseek did; report where the stream still is.
    return ToPosition(file_.pubseekoff(0, std::ios_base::cur));
  }
  return ToPosition(moved);
}

base::ssize DiskStream::Size() {
  const std::streampos here = file_.pubseekoff(0, std::ios_base::cur);
  const std::streampos end = file_.pubseekoff(0, std::ios_base::end);
  file_.pubseekpos(here);
  return ToPosition(end);
}

bool DiskStream::Flush() {
  try {
    if (file_.pubsync() != 0) {
      failed_ = true;
    }
  } catch (const std::ios_base::failure&) {
    // Same real I/O error case Read() catches, on the write-back path.
    failed_ = true;
  }
  return ok();
}
