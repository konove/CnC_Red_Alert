#ifndef CNC_RED_ALERT_ENGINE_GFX_FADING_TABLE_H_
#define CNC_RED_ALERT_ENGINE_GFX_FADING_TABLE_H_

#include <cstdint>
#include <span>

#include "absl/base/attributes.h"

// Builds a 256-entry remap table that fades every palette color toward
// `color` by `frac` (0..255, where 255 is fully faded). Each entry in
// `dest` maps a palette index to the index of the closest-matching color on
// the path toward `color`, found by nearest-neighbor search over `palette`.
//
// `palette` must hold at least 256 RGB triples (768 bytes) and `dest` at
// least 256 bytes; `color` must be a valid palette index (0..255). Returns
// `dest` unchanged if any of those preconditions fail.
std::span<uint8_t> Build_Fading_Table(
    std::span<const uint8_t> palette,
    std::span<uint8_t> dest ABSL_ATTRIBUTE_LIFETIME_BOUND, int color, int frac);

#endif  // CNC_RED_ALERT_ENGINE_GFX_FADING_TABLE_H_
