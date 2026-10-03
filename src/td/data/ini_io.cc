#include "td/data/ini_io.h"

#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "engine/file/file_access.h"
#include "engine/file/game_file.h"
#include "engine/file/ini_file.h"
#include "engine/file/profile_buffer.h"
#include "engine/stream/span_source.h"

namespace td {

void LoadIniText(const std::string_view text, engine::file::IniFile& ini) {
  SpanSource source(std::as_bytes(std::span(text)));
  ini.Load(source);
}

bool LoadIniFile(const std::string_view name, engine::file::IniFile& ini) {
  // ReadProfileFile() ends the text at the first NUL, the old readers' rule.
  const std::optional<std::string> text = engine::file::ReadProfileFile(name);
  if (!text) {
    return false;
  }
  LoadIniText(*text, ini);
  return true;
}

void SaveIniFile(const std::string_view name,
                 const engine::file::IniFile& ini) {
  if (const auto stream = OpenGameFile(name, FileAccess::kWrite)) {
    ini.Save(*stream);
  }
}

}  // namespace td
