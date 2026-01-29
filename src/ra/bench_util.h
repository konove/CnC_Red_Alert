#ifndef CNC_RED_ALERT_RA_BENCH_UTIL_H_
#define CNC_RED_ALERT_RA_BENCH_UTIL_H_

// Inline helpers for starting/stopping benchmark timers.

#include "ra/config.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "tech/bench.h"

inline void BStart(BenchType bench) {
  if constexpr (config::kCheatKeysEnabled) {
    if (Benches != nullptr) {
      Benches[bench].Begin();
    }
  }
}

inline void BEnd(BenchType bench) {
  if constexpr (config::kCheatKeysEnabled) {
    if (Benches != nullptr) {
      Benches[bench].End();
    }
  }
}

#endif  // CNC_RED_ALERT_RA_BENCH_UTIL_H_
