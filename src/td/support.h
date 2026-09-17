#ifndef CNC_RED_ALERT_TD_SUPPORT_H_
#define CNC_RED_ALERT_TD_SUPPORT_H_

#include <cstdint>
#include <source_location>
#include <span>

#include "absl/base/attributes.h"
#include "absl/log/check.h"
#include "sdllib/gbuffer.h"
#include "td/config.h"

std::span<uint8_t> Conquer_Build_Fading_Table(
    std::span<const uint8_t> palette,
    std::span<uint8_t> dest ABSL_ATTRIBUTE_LIFETIME_BOUND, int color, int frac);
void Fat_Put_Pixel(int x, int y, std::uint8_t color, int size,
                   GraphicViewPortClass& /*gpage*/);
void strtrim(char* buffer);

// Null pointer check that fires only in cheat-key builds.
inline void Check_Ptr(const void* ptr, std::source_location loc =
                                           std::source_location::current()) {
  if constexpr (config::kCheatKeysEnabled) {
    CHECK(ptr != nullptr) << "NULL pointer, module:" << loc.file_name()
                          << ", line:" << loc.line();
  }
}

#endif  // CNC_RED_ALERT_TD_SUPPORT_H_
