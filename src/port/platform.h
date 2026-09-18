// File: Compile-time facts about the platform being built for.

#ifndef CNC_RED_ALERT_PORT_PLATFORM_H_
#define CNC_RED_ALERT_PORT_PLATFORM_H_

namespace port {

// True when building for Windows. For choosing a path with `if constexpr`
// where both sides compile on every platform, so the other side is still
// type-checked. Code that names Windows-only declarations still needs
// #ifdef _WIN32.
#ifdef _WIN32
inline constexpr bool kIsWindows = true;
#else
inline constexpr bool kIsWindows = false;
#endif

// Path component buffer sizes, matching the Microsoft CRT's _MAX_* limits.
// Some of these size character arrays in save files; changing them breaks
// saved games.
inline constexpr int kMaxPath = 260;
inline constexpr int kMaxFname = 256;
inline constexpr int kMaxExt = 256;

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_PLATFORM_H_
