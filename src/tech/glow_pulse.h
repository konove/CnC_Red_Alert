#ifndef CNC_RED_ALERT_TECH_GLOW_PULSE_H_
#define CNC_RED_ALERT_TECH_GLOW_PULSE_H_

// The pulsing glow shared by the radar box, the embers and the mission map's
// hotspots.

#include <algorithm>
#include <cstdint>

#include "tech/ftimer.h"
#include "tech/rgb.h"

// A fade toward black that swings back and forth, one step per `period` ticks
// of the tick source T. The caller polls it and rewrites its glowing palette
// entries whenever it steps.
//
// Example:
//   static GlowPulse<SystemTickSource> pulse(kTimerSecond / 6);
//   if (pulse.Update()) {
//     palette.at(kPulseColor) = pulse.Apply(palette.at(kWhite));
//     palette.Set();
//   }
template <TickSource T>
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
    if (!timer_.IsFinished()) {
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
  [[nodiscard]] RGBClass Apply(const RGBClass& color) const {
    return color.Adjusted(fade_, kBlackColor);
  }

  // The current RGBClass::Adjust() ratio toward black, kMinFade..kMaxFade.
  [[nodiscard]] int fade() const { return fade_; }

 private:
  Timer<T> timer_;
  int64_t period_;
  int fade_ = kMaxFade;
  int step_ = -kFadeStep;  // Signed; flips at either end of the range.
};

#endif  // CNC_RED_ALERT_TECH_GLOW_PULSE_H_
