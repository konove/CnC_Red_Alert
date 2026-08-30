// File: The Win32 scalar and handle types the 1990s sources spell out by name.
//
// Red Alert's Westwood Online client was written against the Win32 SDK and
// names its types the way that SDK does. Rather than rewrite ~19,000 lines of
// it, this header supplies the same names off Windows so the code compiles and
// the compiler can check it. On Windows the real <windows.h> is used instead,
// so a Windows build sees the genuine definitions.
//
// The names here are dictated by the API being emulated, which is why they do
// not follow the project's Google naming -- `DWORD` has to be spelled `DWORD`
// to be useful. Anything that is not required by a call site does not belong
// here.

#ifndef CNC_RED_ALERT_PORT_WIN32_WIN32_TYPES_H_
#define CNC_RED_ALERT_PORT_WIN32_WIN32_TYPES_H_

#ifdef _WIN32

#include <windows.h>

#else

#include <cstdint>

using BYTE = std::uint8_t;
using WORD = std::uint16_t;
using DWORD = std::uint32_t;
using LONG = std::int32_t;
using ULONG = std::uint32_t;
using UINT = std::uint32_t;
using INT = std::int32_t;
using BOOL = int;
using CHAR = char;

static_assert(sizeof(DWORD) == 4,
              "Win32 code passes sizeof(DWORD) as a buffer size");

using LPSTR = char*;
using LPCSTR = const char*;
using LPBYTE = BYTE*;
using LPWORD = WORD*;
using LPDWORD = DWORD*;
using LPVOID = void*;
using LPCVOID = const void*;
using PVOID = void*;

// Every Win32 handle is an opaque pointer here rather than a distinct type per
// kind. The original code casts freely between them and tests them for truth
// (`if (hDIB)`, `(int)::ShellExecute(...)`), so distinguishing them would cost
// a great deal of churn in code that can never run.
using HANDLE = void*;
using HWND = void*;
using HINSTANCE = void*;
using HMODULE = void*;
using HKEY = void*;
using HGLOBAL = void*;

// A procedure address returned by GetProcAddress. Windows types this as
// `int (WINAPI*)()`; the one call site invokes it and compares the result
// against S_OK.
using FARPROC = int (*)();

inline constexpr int TRUE = 1;
inline constexpr int FALSE = 0;

inline constexpr int MAX_PATH = 260;

// Windows' INVALID_HANDLE_VALUE is (HANDLE)-1, which no cast-free constant
// expression can spell. The address of a byte that exists for no other reason
// is just as unequal to every real handle, and needs no cast.
inline char kInvalidHandleStorage = 0;
inline void* const INVALID_HANDLE_VALUE = &kInvalidHandleStorage;

#endif  // _WIN32

#endif  // CNC_RED_ALERT_PORT_WIN32_WIN32_TYPES_H_
