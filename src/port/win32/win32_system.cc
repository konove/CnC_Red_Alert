#include "port/win32/win32_system.h"

#ifndef _WIN32

#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace {

// FindFirstFile hands back a handle whose only job is to be distinguishable
// from INVALID_HANDLE_VALUE and to survive FindClose. Since FindNextFile does
// not exist here, there is no iteration state to carry.
char kFoundHandleStorage = 0;

bool HasWildcard(std::string_view path) {
  return path.find_first_of("*?") != std::string_view::npos;
}

}  // namespace

HANDLE FindFirstFile(LPCSTR file_name, WIN32_FIND_DATA* find_data) {
  if (file_name == nullptr || find_data == nullptr || HasWildcard(file_name)) {
    return INVALID_HANDLE_VALUE;
  }

  std::error_code error;
  const std::filesystem::path path(file_name);
  if (!std::filesystem::is_regular_file(path, error)) {
    return INVALID_HANDLE_VALUE;
  }

  const std::uintmax_t size = std::filesystem::file_size(path, error);
  if (error) {
    return INVALID_HANDLE_VALUE;
  }

  *find_data = {};
  find_data->nFileSizeLow = static_cast<DWORD>(size & 0xFFFFFFFFU);
  find_data->nFileSizeHigh = static_cast<DWORD>(size >> 32U);

  const std::string name = path.filename().string();
  const std::size_t copied =
      std::min(name.size(), sizeof(find_data->cFileName) - 1);
  std::memcpy(find_data->cFileName, name.data(), copied);
  find_data->cFileName[copied] = '\0';

  return &kFoundHandleStorage;
}

BOOL FindClose(HANDLE /*find_file*/) { return TRUE; }

BOOL DeleteFile(LPCSTR file_name) {
  if (file_name == nullptr) {
    return FALSE;
  }
  std::error_code error;
  return std::filesystem::remove(std::filesystem::path(file_name), error)
             ? TRUE
             : FALSE;
}

HINSTANCE ShellExecute(HWND /*window*/, LPCSTR /*operation*/, LPCSTR /*file*/,
                       LPCSTR /*parameters*/, LPCSTR /*directory*/,
                       int /*show*/) {
  return nullptr;
}

HINSTANCE LoadLibrary(LPCSTR /*file_name*/) { return nullptr; }

FARPROC GetProcAddress(HMODULE /*module*/, LPCSTR /*proc_name*/) {
  return nullptr;
}

BOOL FreeLibrary(HMODULE /*module*/) { return TRUE; }

BOOL CreateProcess(LPCSTR /*application_name*/, LPSTR /*command_line*/,
                   void* /*process_attributes*/, void* /*thread_attributes*/,
                   BOOL /*inherit_handles*/, DWORD /*creation_flags*/,
                   void* /*environment*/, LPCSTR /*current_directory*/,
                   STARTUPINFO* /*startup_info*/,
                   PROCESS_INFORMATION* process_information) {
  if (process_information != nullptr) {
    *process_information = {};
  }
  return FALSE;
}

DWORD WaitForInputIdle(HANDLE /*process*/, DWORD /*milliseconds*/) { return 0; }

BOOL GetExitCodeProcess(HANDLE /*process*/, LPDWORD exit_code) {
  if (exit_code != nullptr) {
    *exit_code = 0;
  }
  return TRUE;
}

BOOL ShowWindow(HWND /*window*/, int /*show*/) { return FALSE; }

BOOL SetForegroundWindow(HWND /*window*/) { return FALSE; }

HWND GetTopWindow(HWND /*parent*/) { return nullptr; }

DWORD GetLastError() { return 0; }

DWORD GetCurrentDirectory(DWORD buffer_length, LPSTR buffer) {
  if (buffer == nullptr || buffer_length == 0) {
    return 0;
  }
  std::error_code failed;
  const std::string path = std::filesystem::current_path(failed).string();
  if (failed || path.size() + 1 > buffer_length) {
    return 0;
  }
  std::memcpy(buffer, path.c_str(), path.size() + 1);
  return static_cast<DWORD>(path.size());
}

BOOL CreateDirectory(LPCSTR path_name, void* /*security*/) {
  if (path_name == nullptr) {
    return FALSE;
  }
  std::error_code failed;
  const bool created = std::filesystem::create_directory(path_name, failed);
  return created && !failed ? TRUE : FALSE;
}

BOOL SetCurrentDirectory(LPCSTR path_name) {
  if (path_name == nullptr) {
    return FALSE;
  }
  std::error_code failed;
  std::filesystem::current_path(path_name, failed);
  return failed ? FALSE : TRUE;
}

#endif  // _WIN32
