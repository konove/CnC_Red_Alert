// File: mixdump, a command line tool that lists and extracts the files inside
// Red Alert's MIX archives.
//
// The archives ship with encrypted indexes, so the only way in is the game's
// own PKey and MixArchive. Nested archives are registered too, which is what
// reaches the mission INIs: the ones shipped today hold the original MIX files
// inside MAIN1..MAIN4.MIX.
//
//   mixdump <game-dir> --index <archive>     print the archive's index
//   mixdump <game-dir> --list <name> ...     say which archive holds each name
//   mixdump <game-dir> <name>                write the file to stdout
//   mixdump <game-dir> <name> <out-file>     write the file to out-file
//
// docs/GAME_DATA.md covers what a stock installation holds and how to identify
// an entry, given that the index stores a CRC of each name rather than the
// name.

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "absl/strings/str_format.h"
#include "engine/base/array.h"
#include "engine/base/numeric.h"
#include "engine/file/disk_stream.h"
#include "engine/file/file_access.h"
#include "engine/file/mix_archive.h"
#include "tools/game_data.h"

namespace {

bool Extract(std::string_view name, std::string_view out_path) {
  const auto found = MixArchive::Offset(name);
  if (!found) {
    absl::FPrintF(stderr, "not in any archive: %s\n", name);
    return false;
  }
  if (!MixArchive::Cache(found->mixfile->Filename())) {
    absl::FPrintF(stderr, "could not cache %s\n",
                  found->mixfile->Filename());
    return false;
  }
  const std::span<const std::byte> data = MixArchive::RetrieveData(name);
  if (data.empty()) {
    absl::FPrintF(stderr, "could not read %s\n", name);
    return false;
  }

  const std::unique_ptr<DiskStream> out =
      DiskStream::Open(out_path, FileAccess::kWrite);
  if (out == nullptr) {
    absl::FPrintF(stderr, "could not write %s\n", out_path);
    return false;
  }
  if (out->Write(data) != std::ssize(data)) {
    absl::FPrintF(stderr, "short write to %s\n", out_path);
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  // argv is a pointer and a count, which is the one place a span has to be
  // built from both; argc counts the entries argv holds.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span<char* const> args(argv, base::ToSize(argc));
  if (args.size() < 3) {
    absl::FPrintF(stderr,
                  "usage: mixdump <game-dir> --list <name>...\n"
                  "       mixdump <game-dir> <name> [out-file]\n");
    return 2;
  }

  OpenGameData(base::At(args, 1));

  const std::string_view command(base::At(args, 2));
  if (command == "--index") {
    for (const char* const arg : args.subspan(3)) {
      const std::string_view name(arg);
      const MixArchive* archive = MixArchive::Register(name, &MixKey());
      if (archive == nullptr) {
        absl::FPrintF(stderr, "no such archive: %s\n", name);
        return 1;
      }
      // The CRC is what a lookup matches, so printing it lets a caller test a
      // guessed name against the entries without extracting anything.
      for (const MixArchive::FileEntry& entry : archive->index()) {
        absl::PrintF("%s\t%08x\t%d\t%d\n", name,
                     static_cast<uint32_t>(entry.crc), entry.offset,
                     entry.size);
      }
    }
    return 0;
  }
  if (command == "--list") {
    for (const char* const arg : args.subspan(3)) {
      const std::string_view name(arg);
      const auto found = MixArchive::Offset(name);
      if (found) {
        absl::PrintF("%s\t%d\t%s\n", name, found->size,
                     found->mixfile->Filename());
      } else {
        absl::PrintF("%s\t-\t-\n", name);
      }
    }
    return 0;
  }

  const std::string out_path =
      args.size() > 3 ? std::string(base::At(args, 3)) : std::string(command);
  return Extract(command, out_path) ? 0 : 1;
}
