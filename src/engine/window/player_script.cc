// Implements LoadPlayerScript(): one line of text to one step.

#include "engine/window/player_script.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/ascii.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "base/strings/number_parse.h"
#include "engine/window/keyboard.h"
#include "engine/window/scripted_player.h"

namespace engine::window {

namespace {

// The keys a script can name, beyond the single letters and digits.
constexpr std::array<std::pair<std::string_view, KeyNumber>, 30> kNamedKeys = {{
    {"ESC", KN_ESC},       {"RETURN", KN_RETURN},
    {"SPACE", KN_SPACE},   {"BACKSPACE", KN_BACKSPACE},
    {"TAB", KN_TAB},       {"UP", KN_UP},
    {"DOWN", KN_DOWN},     {"LEFT", KN_LEFT},
    {"RIGHT", KN_RIGHT},   {"HOME", KN_HOME},
    {"END", KN_END},       {"PGUP", KN_PGUP},
    {"PGDN", KN_PGDN},     {"INSERT", KN_INSERT},
    {"DELETE", KN_DELETE}, {"COMMA", KN_COMMA},
    {"PERIOD", KN_PERIOD}, {"SLASH", KN_SLASH},
    {"F1", KN_F1},         {"F2", KN_F2},
    {"F3", KN_F3},         {"F4", KN_F4},
    {"F5", KN_F5},         {"F6", KN_F6},
    {"F7", KN_F7},         {"F8", KN_F8},
    {"F9", KN_F9},         {"F10", KN_F10},
    {"F11", KN_F11},       {"F12", KN_F12},
}};

std::optional<int> ParseInt(const std::string_view text) {
  return base::ParseInteger<int>(text);
}

std::optional<KeyNumber> ParseKey(const std::string_view name) {
  const std::string upper = absl::AsciiStrToUpper(name);
  if (upper.size() == 1) {
    const char c = upper.front();
    if (c >= 'A' && c <= 'Z') {
      return static_cast<KeyNumber>(static_cast<int>(KN_A) + (c - 'A'));
    }
    if (c == '0') {
      return KN_0;
    }
    if (c >= '1' && c <= '9') {
      return static_cast<KeyNumber>(static_cast<int>(KN_1) + (c - '1'));
    }
  }
  for (const auto& [key_name, key] : kNamedKeys) {
    if (key_name == upper) {
      return key;
    }
  }
  return std::nullopt;
}

// Queues the step the words of one line describe, after "at <frame>".
// Returns what is wrong with them, or an empty string.
std::string QueueStep(const std::vector<std::string_view>& words,
                      ScriptedPlayer& player, const PlayerScriptHooks& hooks) {
  const std::string_view verb = words.at(2);
  const size_t arguments = words.size() - 3;

  if (verb == "quit") {
    if (arguments != 0) {
      return "quit takes nothing";
    }
    player.Do(hooks.quit);
    return {};
  }

  if (verb == "key" || verb == "shot" || verb == "type") {
    if (arguments != 1) {
      return absl::StrFormat("%s takes one word", verb);
    }
    const std::string_view word = words.at(3);
    if (verb == "shot") {
      player.Do([take_shot = hooks.take_shot, name = std::string(word)] {
        take_shot(name);
      });
      return {};
    }
    if (verb == "type") {
      for (const char c : word) {
        if (!absl::ascii_isalnum(static_cast<unsigned char>(c))) {
          return "type takes letters and digits";
        }
      }
      player.Type(word);
      return {};
    }
    const std::optional<KeyNumber> key = ParseKey(word);
    if (!key) {
      return absl::StrFormat("no key is called %s", word);
    }
    player.Key(*key);
    return {};
  }

  if (verb == "click" || verb == "rclick" || verb == "press" ||
      verb == "move" || verb == "release") {
    if (arguments != 2) {
      return absl::StrFormat("%s takes x and y", verb);
    }
    const std::optional<int> x = ParseInt(words.at(3));
    const std::optional<int> y = ParseInt(words.at(4));
    if (!x || !y) {
      return absl::StrFormat("%s takes two numbers", verb);
    }
    if (verb == "click") {
      player.Click(*x, *y);
    } else if (verb == "rclick") {
      player.Click(*x, *y, MouseButton::kRight);
    } else if (verb == "press") {
      player.Press(*x, *y);
    } else if (verb == "move") {
      player.MoveTo(*x, *y);
    } else {
      player.Release(*x, *y);
    }
    return {};
  }

  return absl::StrFormat("unknown step %s", verb);
}

}  // namespace

std::string LoadPlayerScript(const std::string_view text,
                             ScriptedPlayer& player,
                             const PlayerScriptHooks& hooks) {
  int line_number = 0;
  int last_frame = 1;
  for (const std::string_view raw_line : absl::StrSplit(text, '\n')) {
    ++line_number;
    const std::string_view line = absl::StripAsciiWhitespace(raw_line);
    if (line.empty() || line.front() == '#') {
      continue;
    }
    const std::vector<std::string_view> words =
        absl::StrSplit(line, absl::ByAnyChar(" \t"), absl::SkipEmpty());

    std::string error;
    if (words.size() < 3 || words.front() != "at") {
      error = "a step is written: at <frame> <step> ...";
    } else if (const std::optional<int> frame = ParseInt(words.at(1));
               !frame || *frame < last_frame) {
      error = "the frame must be a number no earlier than the one before";
    } else {
      last_frame = *frame;
      player.At(*frame);
      error = QueueStep(words, player, hooks);
    }
    if (!error.empty()) {
      return absl::StrFormat("line %d: %s", line_number, error);
    }
  }
  return {};
}

}  // namespace engine::window
