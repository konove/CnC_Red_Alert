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

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/numeric.h"
#include "tech/base64.h"
#include "tech/byte_stream.h"
#include "sdllib/file_access.h"
#include "tech/int.h"
#include "tech/mix_archive.h"
#include "tech/pk.h"
#include "tech/search_paths.h"

// search_paths.cc calls the game's disc check through this hook. The tool
// reads an installed directory and never looks for a disc.
// NOLINTBEGIN(misc-use-internal-linkage): satisfies search_paths.cc's extern.
int Get_CD_Index(int cd_drive, int timeout);
int Get_CD_Index(int /*cd_drive*/, int /*timeout*/) { return -1; }
// NOLINTEND(misc-use-internal-linkage)

namespace {

// The archives the games register, outermost first. A nested archive can only
// be opened once the one holding it is registered, so the order matters: the
// Steam release packs the original MIX files inside MAIN1..MAIN4.MIX.
constexpr std::string_view kArchives[] = {
    "MAIN1.MIX",    "MAIN2.MIX",     "MAIN3.MIX",     "MAIN4.MIX",
    "EXPAND.MIX",   "EXPAND2.MIX",   "HIRES1.MIX",    "LORES1.MIX",
    "REDALERT.MIX", "MAIN.MIX",      "GENERAL.MIX",   "GENERAL1.MIX",
    "GENERAL2.MIX", "GENERAL3.MIX",  "GENERAL4.MIX",  "LOCAL.MIX",
    "CONQUER.MIX",  "HIRES.MIX",     "LORES.MIX",     "SCORES.MIX",
    "SOUNDS.MIX",   "SPEECH.MIX",    "RUSSIAN.MIX",   "ALLIES.MIX",
    "MOVIES1.MIX",  "MOVIES2.MIX",   "TRANSIT.MIX",   "INTERIOR.MIX",
    "SNOW.MIX",     "TEMPERAT.MIX"};

// The public half of the key the archives are indexed with, as ra/const.h
// embeds it and INIClass::Get_PKey(true) reads it. The exponent is the known
// fast constant rather than anything stored.
constexpr std::string_view kMixPublicKey =
    "AihRvNoIbTn85FZRYNZRcT+i6KpU+maCsEqr3Q5q+LDB5tH7Tz2qQ38V";

PKey MixKey() {
  PKey key;

  std::array<uint8_t, 512> der{};
  const BigInt exponent = PKey::Fast_Exponent();
  const int length = exponent.DEREncode(der);
  key.Decode_Exponent(
      std::as_bytes(std::span(der)).first(base::ToSize(length)));

  std::vector<std::byte> modulus(kMixPublicKey.size());
  const int decoded =
      Base64_Decode(std::as_bytes(std::span(kMixPublicKey)), modulus);
  key.Decode_Modulus(std::span(modulus).first(base::ToSize(decoded)));
  return key;
}

// Registers every archive that is present. Missing ones are normal: no release
// ships all of them, and a nested archive appears only once its container is
// registered.
void RegisterArchives(const PKey& key) {
  for (const std::string_view name : kArchives) {
    MixArchive::Register(name, &key);
  }
}

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

  SearchPaths::Add(base::At(args, 1));
  const PKey key = MixKey();
  RegisterArchives(key);

  const std::string_view command(base::At(args, 2));
  if (command == "--index") {
    for (const char* const arg : args.subspan(3)) {
      const std::string_view name(arg);
      const MixArchive* archive = MixArchive::Register(name, &key);
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
