#ifndef CNC_RED_ALERT_TD_DATA_INI_IO_H_
#define CNC_RED_ALERT_TD_DATA_INI_IO_H_

// File: Loading an engine::file::IniFile from the text of a game file, and
// saving one back, for the scenario and configuration code.

#include <string_view>

namespace engine::file {
class IniFile;
}  // namespace engine::file

namespace td {

// Adds the sections of the INI `text` to `ini`. Text without a section leaves
// `ini` as it was, so every lookup returns its default.
void LoadIniText(std::string_view text, engine::file::IniFile& ini);

// Fills `ini` from the game file `name`, read as the old profile reader did:
// up to the first NUL byte. Returns false, leaving `ini` as it was, when the
// file does not exist; a file without a section counts as found.
bool LoadIniFile(std::string_view name, engine::file::IniFile& ini);

// Writes `ini` to the game file `name`, replacing it. A file that cannot be
// opened or written is not reported, as the text writer it replaced did not.
void SaveIniFile(std::string_view name, const engine::file::IniFile& ini);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_DATA_INI_IO_H_
