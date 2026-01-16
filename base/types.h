// Core type definitions for the project.
#pragma once

#include <cstddef>

namespace base {

// Signed integer type for indices, counts, and sizes.
//
// Prefer this over size_t for general indexing to avoid signed/unsigned
// comparison issues and to allow negative values for sentinel/error cases.
// Guaranteed to be large enough to represent the size of any object.
using ssize = std::ptrdiff_t;

}  // namespace base
