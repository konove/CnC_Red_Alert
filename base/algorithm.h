// Utility algorithms for containers.
#ifndef CNC_RED_ALERT_BASE_ALGORITHM_H_
#define CNC_RED_ALERT_BASE_ALGORITHM_H_

#include <algorithm>  // IWYU pragma: keep
#include <iterator>

#include "base/types.h"

namespace base {

// Returns the index of the first false element, or -1 if none found.
template <typename Container>
ssize first_false(const Container& c) {
  auto it = std::find(c.begin(), c.end(), false);
  if (it == c.end()) {
    return -1;
  }
  return static_cast<ssize>(std::distance(c.begin(), it));
}

// Returns the index of the first true element, or -1 if none found.
template <typename Container>
ssize first_true(const Container& c) {
  auto it = std::find(c.begin(), c.end(), true);
  if (it == c.end()) {
    return -1;
  }
  return static_cast<ssize>(std::distance(c.begin(), it));
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_ALGORITHM_H_
