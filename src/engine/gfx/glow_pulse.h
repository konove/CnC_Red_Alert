#ifndef CNC_RED_ALERT_ENGINE_GFX_GLOW_PULSE_H_
#define CNC_RED_ALERT_ENGINE_GFX_GLOW_PULSE_H_

// The pulsing glow shared by the radar box, the embers and the mission map's
// hotspots.

#include <algorithm>
#include <cstdint>

#include "engine/gfx/palette_color.h"
#include "engine/platform/timer.h"

// A fade toward black that swings back and forth, one step per `period` ticks
// of the tick source T. The caller polls it and rewrites its glowing palette
// entries whenever it steps.
//
// Example:
//   // The color at full brightness, and the palette slot that shows it glow.
//   constexpr engine::gfx::PaletteColor kGlowBase(255, 80, 80);
//   constexpr int kGlowSlot = 254;
//
//   static GlowPulse<engine::platform::SystemTickSource> pulse(
//       kTimerSecond / 6);
//   if (pulse.Update()) {
//     palette.at(kGlowSlot) = pulse.Apply(kGlowBase);
//     palette.Set();
//   }
template <engine::platform::TickSource T>
class GlowPulse {
 public:
  // Six steps carry the fade across its range, so a full swing out and back
  // takes twelve periods. The range stops short of both ends: the glow has to
  // stay legible at its darkest and stay distinct from the plain color at its
  // brightest.
  static constexpr int kFadeStep = 20;
  static constexpr int kMinFade = 32;
  static constexpr int kMaxFade = 150;

  // The first Update() steps at once; later ones step `period` ticks apart.
  explicit GlowPulse(int64_t period) noexcept : period_(period) {}

  // Steps the fade if the period has run out. Returns true if it stepped, which
  // is when colors taken from Apply() go stale.
  bool Update() {
    if (timer_.HasTimeLeft()) {
      return false;
    }
    timer_.Set(period_);

    fade_ = std::clamp(fade_ + step_, kMinFade, kMaxFade);
    if (fade_ == kMinFade || fade_ == kMaxFade) {
      step_ = -step_;
    }
    return true;
  }

  // Returns `color` dimmed by the current fade.
  [[nodiscard]] engine::gfx::PaletteColor Apply(
      const engine::gfx::PaletteColor& color) const {
    return color.FadedToward(fade_, engine::gfx::kBlackColor);
  }

  // The current PaletteColor::FadedToward() ratio toward black,
  // kMinFade..kMaxFade.
  [[nodiscard]] int fade() const { return fade_; }

 private:
  engine::platform::Countdown<T> timer_;
  int64_t period_;
  int fade_ = kMaxFade;
  int step_ = -kFadeStep;  // Signed; flips at either end of the range.
};

#endif  // CNC_RED_ALERT_ENGINE_GFX_GLOW_PULSE_H_
