// Utility algorithms for containers.
#ifndef CNC_RED_ALERT_BASE_ALGORITHM_H_
#define CNC_RED_ALERT_BASE_ALGORITHM_H_

#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>

#include "base/types.h"

namespace base {

// Returns the index of the first element whose projection equals value, or -1
// if none does. The projection, as in std::ranges::find, defaults to the
// element itself.
//
// Example:
//   base::IndexOf(nodes, StructType::kPower, &BaseNode::type);
template <typename Container, typename T, typename Projection = std::identity>
ssize IndexOf(const Container& c, const T& value, Projection projection = {}) {
  const auto it = std::ranges::find(c, value, projection);
  if (it == c.end()) {
    return -1;
  }
  return static_cast<ssize>(std::distance(c.begin(), it));
}

// Removes the first element whose projection equals value, keeping the others
// in order. Returns false, leaving the vector alone, if none does. Unlike
// std::erase, this removes one match, not all of them. The projection works as
// in IndexOf().
template <typename T, typename Allocator, typename U,
          typename Projection = std::identity>
bool EraseFirst(std::vector<T, Allocator>& vector, const U& value,
                Projection projection = {}) {
  const auto it = std::ranges::find(vector, value, projection);
  if (it == vector.end()) {
    return false;
  }
  vector.erase(it);
  return true;
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_ALGORITHM_H_
