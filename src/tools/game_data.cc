#include "tools/game_data.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "engine/base/numeric.h"
#include "engine/codec/base64.h"
#include "engine/crypto/int.h"
#include "engine/crypto/pk.h"
#include "tech/mix_archive.h"
#include "tech/search_paths.h"

namespace {

// The archives the games register, outermost first. A nested archive can only
// be opened once the one holding it is registered, so the order matters: the
// Steam release of Red Alert packs the original MIX files inside
// MAIN1..MAIN4.MIX. Tiberian Dawn's archives are unencrypted and loose; its
// names that Red Alert shares are listed once.
constexpr std::string_view kArchives[] = {
    "MAIN1.MIX",    "MAIN2.MIX",    "MAIN3.MIX",    "MAIN4.MIX",
    "EXPAND.MIX",   "EXPAND2.MIX",  "HIRES1.MIX",   "LORES1.MIX",
    "REDALERT.MIX", "MAIN.MIX",     "GENERAL.MIX",  "GENERAL1.MIX",
    "GENERAL2.MIX", "GENERAL3.MIX", "GENERAL4.MIX", "LOCAL.MIX",
    "CONQUER.MIX",  "HIRES.MIX",    "LORES.MIX",    "SCORES.MIX",
    "SOUNDS.MIX",   "SPEECH.MIX",   "RUSSIAN.MIX",  "ALLIES.MIX",
    "MOVIES1.MIX",  "MOVIES2.MIX",  "TRANSIT.MIX",  "INTERIOR.MIX",
    "SNOW.MIX",     "TEMPERAT.MIX", "CCLOCAL.MIX",  "UPDATE.MIX",
    "UPDATEC.MIX",  "LANGUAGE.MIX", "MOVIES.MIX",   "ZOUNDS.MIX"};

// The public half of the key the archives are indexed with, as ra/const.h
// embeds it and INIClass::Get_PKey(true) reads it. The exponent is the known
// fast constant rather than anything stored.
constexpr std::string_view kMixPublicKey =
    "AihRvNoIbTn85FZRYNZRcT+i6KpU+maCsEqr3Q5q+LDB5tH7Tz2qQ38V";

PKey BuildMixKey() {
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

}  // namespace

const PKey& MixKey() {
  static const PKey key = BuildMixKey();
  return key;
}

void OpenGameData(const std::string_view game_dir) {
  SearchPaths::Add(game_dir);
  // Missing archives are normal: no release ships all of them, and a nested
  // one appears only once its container is registered.
  for (const std::string_view name : kArchives) {
    MixArchive::Register(name, &MixKey());
  }
}
