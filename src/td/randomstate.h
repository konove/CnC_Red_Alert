// Saved state of TD's gameplay, byte, and simulation-table random streams.
#ifndef CNC_RED_ALERT_TD_RANDOMSTATE_H_
#define CNC_RED_ALERT_TD_RANDOMSTATE_H_

#include <cstdint>

struct TdRandomState {
  uint32_t gameplay = 0;
  uint32_t byte_stream = 0;
  int32_t simulation_index = 0;

  template <class Archive>
  void Serialize(Archive& ar) {
    ar(gameplay, byte_stream, simulation_index);
    if constexpr (Archive::kIsReading) {
      if (simulation_index < 0 || simulation_index > 255) {
        ar.Fail("invalid simulation random index");
      }
    }
  }
};

// These functions leave libc's UI/setup RNG independent of saved gameplay.
void SeedGameRandom(uint32_t seed);
int GameRandomRange(int low, int high);
int GameRandomDraw();
TdRandomState CaptureRandomState();
void RestoreRandomState(const TdRandomState& state);

#endif  // CNC_RED_ALERT_TD_RANDOMSTATE_H_
