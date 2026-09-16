#ifndef CNC_RED_ALERT_TD_NULLDLG_H_
#define CNC_RED_ALERT_TD_NULLDLG_H_

#include <span>
#include <string_view>

#include "absl/strings/str_format.h"
#include "td/defines.h"

bool Init_Null_Modem(SerialSettingsType* settings);
void Shutdown_Modem();
void Modem_Signoff();
int Test_Null_Modem();
int Reconnect_Modem();
void Destroy_Null_Connection(int id, int error);
GameType Select_Serial_Dialog();
int Com_Scenario_Dialog();
int Com_Show_Scenario_Dialog();

// Prints `text` to the mono page or stdout, as the debug print switches
// direct.
void Smart_Print(std::string_view text);
// Formats `format` with `args`, checked at compile time, and prints the
// result as Smart_Print does.
template <typename... Args>
void Smart_Printf(const absl::FormatSpec<Args...>& format,
                  const Args&... args) {
  Smart_Print(absl::StrFormat(format, args...));
}
void Hex_Dump_Data(std::span<const char> buffer);
void itoh(int i, std::span<char> s);

class ModemRegistryEntryClass;
extern ModemRegistryEntryClass* ModemRegistry;

#endif  // CNC_RED_ALERT_TD_NULLDLG_H_
