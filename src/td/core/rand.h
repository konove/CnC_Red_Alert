#ifndef CNC_RED_ALERT_TD_CORE_RAND_H_
#define CNC_RED_ALERT_TD_CORE_RAND_H_

#include "game/sim/direction.h"
#include "td/core/randomstate.h"

namespace td {

int Sim_IRandom(int minval, int maxval);
int Sim_Random();

template <class T>
T Random_Picky(T a, T b, [[maybe_unused]] const char* sfile,
               [[maybe_unused]] int line) {
  return static_cast<T>(
      GameRandomRange(static_cast<int>(a), static_cast<int>(b)));
}

inline game::DirType Random_Picky(const game::DirType a, const game::DirType b,
                                  const char* sfile, const int line) {
  return game::DirType(Random_Picky(a.value(), b.value(), sfile, line));
}

#define Random_Pick(low, high) Random_Picky((low), (high), __FILE__, __LINE__)

template <class T>
T Sim_Random_Pick(T a, T b) {
  return static_cast<T>(Sim_IRandom(static_cast<int>(a), static_cast<int>(b)));
}

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_RAND_H_
