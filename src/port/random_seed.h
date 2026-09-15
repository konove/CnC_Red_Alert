// File: A fresh seed for the game's random number generators.
//
// The simulation generators (RandomClass) are deterministic on purpose; the
// seed they start from is the one number that must differ between games.
// RandomSeed draws it from the operating system's entropy source, replacing
// the srand/rand pairs that used the C library generator only for this.
//
// Example:
//   Seed = port::RandomSeed();
//   Scen.sync_rng_.set_seed(static_cast<uint32_t>(Seed));

#ifndef CNC_RED_ALERT_PORT_RANDOM_SEED_H_
#define CNC_RED_ALERT_PORT_RANDOM_SEED_H_

namespace port {

// Returns a non-negative seed from std::random_device. Non-negative keeps the
// range rand() had, since seeds travel in packets and INI files as int.
[[nodiscard]] int RandomSeed();

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_RANDOM_SEED_H_
