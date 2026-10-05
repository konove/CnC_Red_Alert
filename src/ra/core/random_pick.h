// The two random number streams: the synchronized one that drives the
// simulation, and the local one for everything that must not.

#ifndef CNC_RED_ALERT_RA_CORE_RANDOM_PICK_H_
#define CNC_RED_ALERT_RA_CORE_RANDOM_PICK_H_

#include "base/random.h"
#include "ra/ai/scenario.h"
#include "ra/map/world.h"

namespace ra {

// Source of random numbers for events that must NOT affect game logic, such
// as cosmetic animations and one-machine-only effects. Because it never
// influences the simulation, it does not need to stay in sync across
// networked machines, and it is not saved. For sync-critical randomness use
// TheScenario().sync_rng_.
inline RandomClass& LocalRandom() {
  static RandomClass rng;
  return rng;
}

// Returns a random value in [low, high] from the synchronized generator. Every
// machine in a multiplayer game must make the same calls in the same order, so
// use it for whatever affects the simulation, and only for that. `T` is an
// integer or an enum; the range must span less than 32768 values.
template <class T>
T RandomPick(const T low, const T high) {
  return static_cast<T>(TheScenario().sync_rng_.InRange(
      static_cast<int>(low), static_cast<int>(high)));
}

// Returns true `percent` times out of 100, drawing from the synchronized
// generator like RandomPick().
inline bool PercentChance(const int percent) {
  // The draw is 0..99, so 0 never succeeds and 100 always does.
  return TheScenario().sync_rng_.InRange(0, 99) < percent;
}

// Returns a random value in [low, high] from LocalRandom(), leaving the
// synchronized generator alone. For anything that differs between machines or
// is only cosmetic: animations, sounds, what one player's screen shows.
template <class T>
T LocalRandomPick(const T low, const T high) {
  return static_cast<T>(
      LocalRandom().InRange(static_cast<int>(low), static_cast<int>(high)));
}

// Returns true `percent` times out of 100, drawing from LocalRandom() like
// LocalRandomPick().
inline bool LocalPercentChance(const int percent) {
  return LocalRandom().InRange(0, 99) < percent;
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_RANDOM_PICK_H_
