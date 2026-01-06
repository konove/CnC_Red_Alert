#ifndef CNC_RED_ALERT_PORT_SAFE_STRING_H_
#define CNC_RED_ALERT_PORT_SAFE_STRING_H_

#include <cstddef>

namespace port {

// Copies `src` into the buffer pointed to by `dest` of size `dest_size`.
//
// This function overwrites the content of `dest` and guarantees
// null-termination. If the length of `src` exceeds `dest_size`, the copy is
// silently truncated.
//
// If `dest` is null or `dest_size` is 0, this function does nothing.
// If `src` is null, `dest` is set to an empty string.
void SafeCopy(char* dest, const char* src, size_t dest_size);

// Appends `src` to the buffer pointed to by `dest` of size `dest_size`.
//
// This function appends to the existing content of `dest` and guarantees
// null-termination.
//
// If `dest` is null, `dest_size` is 0, or `src` is null, this function does
// nothing. If the combined length exceeds `dest_size`, the result is silently
// truncated.
void SafeAppend(char* dest, const char* src, size_t dest_size);

// Allocates a new copy of `src` on the heap.
//
// Returns a pointer to a newly allocated character array containing a copy of
// `src`. The caller takes ownership of the returned pointer and must free it
// using `delete[]`.
//
// If `src` is null, returns nullptr.
char* CloneString(const char* src);

// -----------------------------------------------------------------------------
// Template Helpers (Must remain in header)
// -----------------------------------------------------------------------------

// Copies `src` into the fixed-size array `dest`.
template <size_t N>
void SafeCopy(char (&dest)[N], const char* src) {
  SafeCopy(dest, src, N);
}

// Appends `src` to the fixed-size array `dest`.
template <size_t N>
void SafeAppend(char (&dest)[N], const char* src) {
  SafeAppend(dest, src, N);
}

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_SAFE_STRING_H_
