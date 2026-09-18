// Arithmetic for the score screen's counting numbers.

#ifndef CNC_RED_ALERT_RA_COUNT_UP_H_
#define CNC_RED_ALERT_RA_COUNT_UP_H_

#include <algorithm>
#include <cstdint>

// Returns what a counter running from 0 to `final_value` shows at `step` of
// `steps` evenly spaced steps: 0 at step 0 and exactly `final_value` at the
// last step, whatever the rounding in between. Steps outside 0..`steps` are
// clamped, and a run of no steps is already at its final value.
constexpr int CountUpValue(int final_value, int step, int steps) {
  if (steps <= 0) {
    return final_value;
  }
  return static_cast<int>(int64_t{final_value} * std::clamp(step, 0, steps) /
                          steps);
}

#endif  // CNC_RED_ALERT_RA_COUNT_UP_H_
