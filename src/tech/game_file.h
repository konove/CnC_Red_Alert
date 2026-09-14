#ifndef CNC_RED_ALERT_TECH_GAME_FILE_H_
#define CNC_RED_ALERT_TECH_GAME_FILE_H_

// File: GameFile, the File the game reads its data through. A name is looked
// up as a loose file through SearchPaths first and then inside the registered
// mixfile archives, whether those are cached in memory or still on disk, so
// callers see an ordinary file either way. game_file.cc also holds the
// name-to-stream lookup that the archives themselves are opened with, which is
// what lets a mixfile packed inside another mixfile work.
//
// Originally CCFILE.H (class CCFileClass) by Joe L. Bostic, October 17, 1994.

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "sdllib/file_access.h"
#include "tech/byte_stream.h"
#include "tech/file.h"

// A file object bound to a game data name. Opening resolves the name and
// attaches one ByteStream: a DiskStream for a loose file, a MemoryStream for
// a file in a cached archive, or a RangeStream for a file in an archive on
// disk (nested as deep as the archives are). A loose file wins over a packed
// copy, which is how patch files override archive data. Writes always go to
// a loose file.
//
// Example:
//   GameFile file("RULES.INI");
//   if (file.Open()) {
//     const std::string text = file.ReadString(file.Size());
//   }
class GameFile : public File {
 public:
  explicit GameFile(std::string_view name) : name_(name) {}
  GameFile() = default;

  GameFile(const GameFile&) = delete;
  GameFile& operator=(const GameFile&) = delete;
  GameFile(GameFile&&) = delete;
  GameFile& operator=(GameFile&&) = delete;

  ~GameFile() override = default;

  // Returns a stream over the bytes of name resolved the way Open() resolves
  // it, or nullptr if the name is found nowhere or cannot be opened. This is
  // how the mixfile archives open their own files.
  static std::unique_ptr<ByteStream> OpenStream(
      std::string_view name, FileAccess rights = FileAccess::kRead);

  // Returns the name as given; resolution happens when the file is opened.
  [[nodiscard]] std::string_view FileName() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND override {
    return name_;
  }

  // Binds name to the object, closing any open file.
  void SetName(std::string_view name) override;

  // Creates an empty loose file. Returns false if it could not be created.
  bool Create() override;

  // Deletes the loose file by this name. Returns false, deleting nothing, if
  // there is none; a file packed in an archive cannot be deleted.
  bool Delete() override;

  // Returns true if the file is open, packed in a registered archive, or found
  // as a loose file.
  bool IsAvailable() override;

  [[nodiscard]] bool IsOpen() const override { return stream_ != nullptr; }

  // Binds name and opens it; see Open(FileAccess).
  bool Open(std::string_view name,
            FileAccess rights = FileAccess::kRead) override;

  // Opens the file, closing it first if it was open, and returns whether a
  // stream could be attached; see OpenStream().
  bool Open(FileAccess rights = FileAccess::kRead) override;

  // Reads from the file, opening it for read access and closing it again if
  // it was not open.
  using File::Read;
  base::ssize Read(std::span<std::byte> buffer) override;

  // Writes to the file, opening it for write access and closing it again if
  // it was not open. A file inside an archive is read-only: writing it writes
  // nothing and returns 0.
  using File::Write;
  base::ssize Write(std::span<const std::byte> buffer) override;
  [[nodiscard]] bool ok() const override { return !failed_; }

  // Returns the new position, or 0 if the file is not open.
  base::ssize Seek(base::ssize offset,
                   SeekOrigin origin = SeekOrigin::kCurrent) override;

  // Returns the size in bytes, for a packed file the size of the embedded
  // file, without keeping the file open. Returns 0 for a file found nowhere.
  base::ssize Size() override;

  void Close() override { stream_.reset(); }

 private:
  std::string name_;

  // The bytes of the open file, or nullptr while it is closed.
  std::unique_ptr<ByteStream> stream_;

  // Set when a read or write fails; cleared by Open().
  bool failed_ = false;
};

#endif  // CNC_RED_ALERT_TECH_GAME_FILE_H_
