// File: One-bit-per-cell bitmaps that record which cells a path has entered.

#ifndef CNC_RED_ALERT_RA_PATH_OVERLAP_H_
#define CNC_RED_ALERT_RA_PATH_OVERLAP_H_

#include <cstdint>

#include "absl/log/check.h"
#include "base/numeric.h"

// Overlap bitmaps are arrays of 32-bit words: cell N lives in word N / 32, at
// bit N % 32.
//
// Example:
//   uint32_t words[OverlapWordCount(MAP_CELL_TOTAL)] = {};
//   SetOverlap(words, cell);
//   if (IsOverlapped(words, cell)) { ... }

// Returns the number of words needed to hold one bit per cell.
constexpr int OverlapWordCount(int cell_count) { return (cell_count + 31) / 32; }

// Returns the index of the word holding `cell`. `cell` must be non-negative.
constexpr int OverlapWord(int cell) { return cell / 32; }

// Returns the mask selecting `cell`'s bit within its word.
constexpr uint32_t OverlapMask(int cell) {
  return base::Bit<uint32_t>(cell % 32);
}

// Returns whether `cell` is marked in the bitmap `words`.
inline bool IsOverlapped(const uint32_t* words, int cell) {
  DCHECK(cell >= 0);
  return (words[OverlapWord(cell)] & OverlapMask(cell)) != 0;
}

// Marks `cell` in the bitmap `words`.
inline void SetOverlap(uint32_t* words, int cell) {
  DCHECK(cell >= 0);
  words[OverlapWord(cell)] |= OverlapMask(cell);
}

// Unmarks `cell` in the bitmap `words`.
inline void ClearOverlap(uint32_t* words, int cell) {
  DCHECK(cell >= 0);
  words[OverlapWord(cell)] &= ~OverlapMask(cell);
}

#endif  // CNC_RED_ALERT_RA_PATH_OVERLAP_H_
