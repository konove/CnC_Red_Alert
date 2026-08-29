#ifndef CNC_RED_ALERT_TD_RAND_H_
#define CNC_RED_ALERT_TD_RAND_H_

#include "sdllib/misc.h"

int Sim_IRandom(int minval, int maxval);
int Sim_Random();

template <class T>
T Random_Picky(T a, T b, [[maybe_unused]] const char* sfile,
               [[maybe_unused]] int line) {
  return static_cast<T>(
      IRandom(static_cast<int>(a), static_cast<int>(b)));
};

#define Random_Pick(low, high) Random_Picky((low), (high), __FILE__, __LINE__)

template <class T>
T Sim_Random_Pick(T a, T b) {
  return static_cast<T>(
      Sim_IRandom(static_cast<int>(a), static_cast<int>(b)));
};

#endif  // CNC_RED_ALERT_TD_RAND_H_
