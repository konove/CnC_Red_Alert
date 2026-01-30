#ifndef CNC_RED_ALERT_TD_CONFIG_H_
#define CNC_RED_ALERT_TD_CONFIG_H_

#include "td/defines.h"

// Modern C++23 compile-time configuration
// This file is gradually replacing preprocessor macros from defines.h
// with type-safe constexpr variables for use with 'if constexpr'

namespace config {

// Build version type
enum class BuildVersion {
  Release,   // Production release - no cheats, no editor
  Playtest,  // Playtest version - limited cheats, no editor
  Internal   // Internal dev build - full cheats, scenario editor
};

// Current build version (mutually exclusive by design)
#ifdef INTERNAL_VERSION
inline constexpr BuildVersion kBuildVersion = BuildVersion::Internal;
#elifdef PLAYTEST_VERSION
inline constexpr BuildVersion kBuildVersion = BuildVersion::Playtest;
#else
inline constexpr BuildVersion kBuildVersion = BuildVersion::Release;
#endif

// Convenience helpers for backward compatibility
inline constexpr bool kReleaseVersion = kBuildVersion == BuildVersion::Release;
inline constexpr bool kPlaytestVersion =
    kBuildVersion == BuildVersion::Playtest;
inline constexpr bool kInternalVersion =
    kBuildVersion == BuildVersion::Internal;

// Scenario editor enabled (migrated from SCENARIO_EDITOR macro).
// Enabled only for internal builds
inline constexpr bool kScenarioEditorEnabled = kInternalVersion;

// Cheat keys enabled for internal and playtest builds.
inline constexpr bool kCheatKeysEnabled = kInternalVersion || kPlaytestVersion;

// Virgin cheat keys - limited cheat key set. This is a subset that only allows
// Alt+W to win. Enabled only for playtest builds (internal builds have full
// cheat keys).
inline constexpr bool kVirginCheatKeysEnabled = kPlaytestVersion;

// Build language (TD supports Japanese in addition to the RA languages)
enum class BuildLanguage { English, German, French, Japanese };

#ifdef FRENCH
inline constexpr auto kBuildLanguage = BuildLanguage::French;
#elifdef GERMAN
inline constexpr auto kBuildLanguage = BuildLanguage::German;
#elifdef JAPANESE
inline constexpr auto kBuildLanguage = BuildLanguage::Japanese;
#else
inline constexpr auto kBuildLanguage = BuildLanguage::English;
#endif

}  // namespace config

#endif  // CNC_RED_ALERT_TD_CONFIG_H_
