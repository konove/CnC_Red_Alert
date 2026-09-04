// File: The Win32 shell, module, process, window and file calls the Westwood
// Online client makes.
//
// These exist so ~19,000 lines of WOL code can be compiled and type-checked off
// Windows. The calls fall into two groups:
//
//   * Things this platform can genuinely do -- test a file for existence,
//     delete one -- are implemented for real over <filesystem>.
//   * Things that only mean something inside a Windows desktop session --
//     launching the user's browser, loading and self-registering wolapi.dll,
//     spawning a process and waiting on its window, raising a window -- fail or
//     do nothing. Every WOL caller already handles that failure, because it is
//     what Windows itself reported when the component was missing.
//
// Names follow the Win32 SDK, because the call sites spell them that way. On
// Windows the real SDK headers are used instead.

#ifndef CNC_RED_ALERT_PORT_WIN32_WIN32_SYSTEM_H_
#define CNC_RED_ALERT_PORT_WIN32_WIN32_SYSTEM_H_

#ifdef _WIN32

#include <windows.h>
// Not self-contained: needs the types from <windows.h> above.
#include <shellapi.h>

#else

#include <cstring>

#include "port/win32/win32_types.h"

// -- Files ------------------------------------------------------------------

// Only the fields the game reads. The real structure carries file times and
// short names as well.
struct WIN32_FIND_DATA {
  DWORD dwFileAttributes;
  DWORD nFileSizeHigh;
  DWORD nFileSizeLow;
  CHAR cFileName[MAX_PATH];
};

// Tests one path and fills `find_data` for it. Returns INVALID_HANDLE_VALUE if
// nothing is there.
//
// Wildcards are NOT supported -- every caller in this tree passes a literal
// file name, and a pattern is reported as not found rather than silently
// matching the wrong thing. There is no FindNextFile for the same reason.
HANDLE FindFirstFile(LPCSTR file_name, WIN32_FIND_DATA* find_data);
BOOL FindClose(HANDLE find_file);

BOOL DeleteFile(LPCSTR file_name);

// The process's working directory, written into `buffer` with a null
// terminator. Returns the length written, or 0 if it does not fit or cannot
// be read -- the Win32 contract, minus the "return the size needed" case,
// which no caller here uses.
DWORD GetCurrentDirectory(DWORD buffer_length, LPSTR buffer);

// Changes the working directory. Returns FALSE if the path is not a directory
// this process can enter.
BOOL SetCurrentDirectory(LPCSTR path_name);

// Creates one directory. Returns FALSE if it already exists or if any parent
// of it does not -- the Win32 behaviour, not mkdir -p. `security` is the
// SECURITY_ATTRIBUTES argument every caller here passes as null; it is
// accepted so the call sites read as they always did, and ignored.
BOOL CreateDirectory(LPCSTR path_name, void* security);

// -- Shell ------------------------------------------------------------------

inline constexpr int SW_SHOWMINIMIZED = 2;
inline constexpr int SW_SHOW = 5;
inline constexpr int SW_RESTORE = 9;

// Returns nullptr, which the callers read as failure: Windows returns a value
// above 32 on success and an error code at or below it otherwise. Deliberately
// not wired to xdg-open -- the URL is built with sprintf into a fixed buffer,
// so handing it to a shell would turn dead code into a live injection surface.
HINSTANCE ShellExecute(HWND window, LPCSTR operation, LPCSTR file,
                       LPCSTR parameters, LPCSTR directory, int show);

// -- Modules ----------------------------------------------------------------

// There is no wolapi.dll to load, so this fails and nothing can be resolved
// out of it.
HINSTANCE LoadLibrary(LPCSTR file_name);
FARPROC GetProcAddress(HMODULE module, LPCSTR proc_name);
BOOL FreeLibrary(HMODULE module);

// -- Processes --------------------------------------------------------------

inline constexpr DWORD STILL_ACTIVE = 259;

struct STARTUPINFO {
  DWORD cb;
};

struct PROCESS_INFORMATION {
  HANDLE hProcess;
  HANDLE hThread;
  DWORD dwProcessId;
  DWORD dwThreadId;
};

// Returns FALSE. The single caller launches the user's web browser and then
// blocks until its window closes, which has no meaning without a Windows
// desktop to own that window.
BOOL CreateProcess(LPCSTR application_name, LPSTR command_line,
                   void* process_attributes, void* thread_attributes,
                   BOOL inherit_handles, DWORD creation_flags,
                   void* environment, LPCSTR current_directory,
                   STARTUPINFO* startup_info,
                   PROCESS_INFORMATION* process_information);
DWORD WaitForInputIdle(HANDLE process, DWORD milliseconds);
BOOL GetExitCodeProcess(HANDLE process, LPDWORD exit_code);

// -- Windows ----------------------------------------------------------------

// The game's window is an SDL_Window here (sdllib's MainWindow is a void*), so
// none of the HWND operations apply. They do nothing.
BOOL ShowWindow(HWND window, int show);
BOOL SetForegroundWindow(HWND window);
HWND GetTopWindow(HWND parent);

// -- Errors -----------------------------------------------------------------

// Always 0: none of the stubs above set an error code worth reporting.
DWORD GetLastError();

#define ZeroMemory(destination, length) std::memset((destination), 0, (length))

#endif  // _WIN32

#endif  // CNC_RED_ALERT_PORT_WIN32_WIN32_SYSTEM_H_
