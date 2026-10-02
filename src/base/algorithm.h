// Utility algorithms for containers.
#ifndef CNC_RED_ALERT_BASE_ALGORITHM_H_
#define CNC_RED_ALERT_BASE_ALGORITHM_H_

#include <algorithm>  // IWYU pragma: keep
#include <iterator>
#include <vector>

#include "base/types.h"

namespace base {

// Returns the index of the first false element, or -1 if none found.
template <typename Container>
ssize first_false(const Container& c) {
  const auto it = std::find(c.begin(), c.end(), false);
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

// Returns the index of the first element equal to value, or -1 if none is.
template <typename Container, typename T>
ssize IndexOf(const Container& c, const T& value) {
  const auto it = std::ranges::find(c, value);
  if (it == c.end()) {
    return -1;
  }
  return static_cast<ssize>(std::distance(c.begin(), it));
}

// Removes the first element equal to value, keeping the others in order.
// Returns false, leaving the vector alone, if no element is equal. Unlike
// std::erase, this removes one match, not all of them.
template <typename T, typename Allocator, typename U>
bool EraseFirst(std::vector<T, Allocator>& vector, const U& value) {
  const auto it = std::ranges::find(vector, value);
  if (it == vector.end()) {
    return false;
  }
  vector.erase(it);
  return true;
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_ALGORITHM_H_
