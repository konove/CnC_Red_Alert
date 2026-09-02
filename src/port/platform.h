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

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_PLATFORM_H_
