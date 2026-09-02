// File: The Windows registry calls the game makes, answering "not found" off
// Windows.
//
// Red Alert reads a handful of values the installer wrote -- which expansions
// are present, where wolapi.dll lives, a few Westwood Online preferences -- and
// writes those preferences back. There is no registry on Linux or macOS and no
// separate installer, so every read fails and every write is dropped.
//
// That is not a new decision: conquer.cc's Is_Counterstrike_Installed() already
// says "No registry off Windows" and hardcodes the answer behind #ifdef _WIN32.
// These stubs let that reasoning live in one place instead of at each call.
//
// Names follow the Win32 SDK, because the call sites spell them that way.

#ifndef CNC_RED_ALERT_PORT_WIN32_WIN32_REGISTRY_H_
#define CNC_RED_ALERT_PORT_WIN32_WIN32_REGISTRY_H_

#ifdef _WIN32

#include <windows.h>

#else

#include "port/win32/win32_types.h"

inline constexpr LONG ERROR_SUCCESS = 0;
inline constexpr LONG ERROR_FILE_NOT_FOUND = 2;

// Root keys. Only their identity matters -- nothing dereferences them.
inline char kRegistryRootStorage[2] = {0, 0};
inline void* const HKEY_LOCAL_MACHINE = &kRegistryRootStorage[0];
inline void* const HKEY_CLASSES_ROOT = &kRegistryRootStorage[1];

inline constexpr DWORD KEY_READ = 0x20019;
inline constexpr DWORD KEY_WRITE = 0x20006;
inline constexpr DWORD KEY_ALL_ACCESS = 0xF003F;

inline constexpr DWORD REG_SZ = 1;
inline constexpr DWORD REG_BINARY = 3;
inline constexpr DWORD REG_DWORD = 4;

// Returns ERROR_FILE_NOT_FOUND and leaves *result untouched, so callers take
// the same path they would on a Windows box where the key was never written.
LONG RegOpenKeyEx(HKEY key, LPCSTR sub_key, DWORD options, DWORD desired,
                  HKEY* result);

// Returns ERROR_FILE_NOT_FOUND. Out-parameters are left untouched: on Windows
// a failed query does not write them either, so a caller that reads its buffer
// anyway is already broken and should not be given a false sense of a value.
LONG RegQueryValueEx(HKEY key, LPCSTR value_name, LPDWORD reserved,
                     LPDWORD type, LPBYTE data, LPDWORD data_size);
LONG RegQueryValue(HKEY key, LPCSTR sub_key, LPSTR value, LONG* value_size);

// Silently discards the write and reports success -- the preferences these
// carry are re-read through the failing queries above, which supply defaults.
LONG RegSetValueEx(HKEY key, LPCSTR value_name, DWORD reserved, DWORD type,
                   const BYTE* data, DWORD data_size);

LONG RegDeleteValue(HKEY key, LPCSTR value_name);
LONG RegCloseKey(HKEY key);

#endif  // _WIN32

#include <cstdint>
#include <optional>

namespace port {

// Reads the REG_DWORD value `value_name` from `sub_key` under `root`, which is
// how the installer recorded which expansions it put on disk. Returns nullopt
// when the key or value is absent or is not a DWORD -- and always off Windows,
// where there is no registry.
std::optional<uint32_t> ReadRegistryDword(HKEY root, const char* sub_key,
                                          const char* value_name);

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_WIN32_WIN32_REGISTRY_H_
