// Where a seek offset is measured from, and the mapping to the stdio SEEK_*
// constants that the C-style file APIs still pass.
#ifndef CNC_RED_ALERT_BASE_SEEK_ORIGIN_H_
#define CNC_RED_ALERT_BASE_SEEK_ORIGIN_H_

#include <cstdio>

// Where a Seek() offset is measured from.
enum class SeekOrigin { kBegin, kCurrent, kEnd };

// Maps a stdio SEEK_* constant to SeekOrigin. Anything unrecognized counts as
// SEEK_CUR, as the file classes have always treated it.
constexpr SeekOrigin SeekOriginFromStdio(int origin) {
  switch (origin) {
    case SEEK_SET:
      return SeekOrigin::kBegin;
    case SEEK_END:
      return SeekOrigin::kEnd;
    default:
      return SeekOrigin::kCurrent;
  }
}

// The stdio SEEK_* constant for origin.
constexpr int StdioOrigin(SeekOrigin origin) {
  switch (origin) {
    case SeekOrigin::kBegin:
      return SEEK_SET;
    case SeekOrigin::kEnd:
      return SEEK_END;
    case SeekOrigin::kCurrent:
    default:
      return SEEK_CUR;
  }
}

#endif  // CNC_RED_ALERT_BASE_SEEK_ORIGIN_H_
