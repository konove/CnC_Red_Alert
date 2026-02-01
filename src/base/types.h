// Core type definitions for the project.
#ifndef CNC_RED_ALERT_BASE_TYPES_H_
#define CNC_RED_ALERT_BASE_TYPES_H_

#include <cstddef>

namespace base {

// Signed integer type for indices, counts, and sizes.
//
// Prefer this over size_t for general indexing to avoid signed/unsigned
// comparison issues and to allow negative values for sentinel/error cases.
// Guaranteed to be large enough to represent the size of any object.
using ssize = std::ptrdiff_t;

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_TYPES_H_
