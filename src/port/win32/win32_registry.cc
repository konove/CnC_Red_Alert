#include "port/win32/win32_registry.h"

#ifndef _WIN32

LONG RegOpenKeyEx(HKEY /*key*/, LPCSTR /*sub_key*/, DWORD /*options*/,
                  DWORD /*desired*/, HKEY* /*result*/) {
  return ERROR_FILE_NOT_FOUND;
}

LONG RegQueryValueEx(HKEY /*key*/, LPCSTR /*value_name*/, LPDWORD /*reserved*/,
                     LPDWORD /*type*/, LPBYTE /*data*/, LPDWORD /*data_size*/) {
  return ERROR_FILE_NOT_FOUND;
}

LONG RegQueryValue(HKEY /*key*/, LPCSTR /*sub_key*/, LPSTR /*value*/,
                   LONG* /*value_size*/) {
  return ERROR_FILE_NOT_FOUND;
}

LONG RegSetValueEx(HKEY /*key*/, LPCSTR /*value_name*/, DWORD /*reserved*/,
                   DWORD /*type*/, const BYTE* /*data*/, DWORD /*data_size*/) {
  return ERROR_SUCCESS;
}

LONG RegDeleteValue(HKEY /*key*/, LPCSTR /*value_name*/) {
  return ERROR_FILE_NOT_FOUND;
}

LONG RegCloseKey(HKEY /*key*/) { return ERROR_SUCCESS; }

#endif  // _WIN32
