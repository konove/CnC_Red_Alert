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

namespace port {

std::optional<uint32_t> ReadRegistryDword(HKEY root, const char* sub_key,
                                          const char* value_name) {
#ifdef _WIN32
  HKEY key;
  if (RegOpenKeyExA(root, sub_key, 0, KEY_READ, &key) != ERROR_SUCCESS) {
    return std::nullopt;
  }
  DWORD value = 0;
  DWORD type = 0;
  DWORD size = sizeof(value);
  const LONG status = RegQueryValueExA(key, value_name, nullptr, &type,
                                       reinterpret_cast<LPBYTE>(&value), &size);
  RegCloseKey(key);
  if (status != ERROR_SUCCESS || type != REG_DWORD || size != sizeof(value)) {
    return std::nullopt;
  }
  return value;
#else
  // No registry: the stubs above would fail the open anyway, but there is no
  // reason to go through them.
  static_cast<void>(root);
  static_cast<void>(sub_key);
  static_cast<void>(value_name);
  return std::nullopt;
#endif
}

}  // namespace port
