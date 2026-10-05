#include "td/startup/scripted_run.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>

#include "absl/base/no_destructor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "engine/gfx/pcx_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/display.h"
#include "engine/window/player_script.h"
#include "engine/window/scripted_run.h"
#include "game/ui/input.h"
#include "td/assets/palette.h"
#include "td/assets/screen.h"
#include "td/startup/startup.h"

namespace td {

namespace {

// Saves the game area of the screen as the PCX file `path`, in the palette
// on screen, which during a fade is not the game's own.
void SaveShot(const std::filesystem::path& path) {
  PixelView& screen = TheScreen().visible_view();
  PixelBuffer copy(screen.width(), screen.height(), {},
                   static_cast<int32_t>(screen.width()) * screen.height());
  screen.BlitTo(copy.view());
  Write_PCX_File(path.string().c_str(), copy.view(), CurrentPalette);
}

}  // namespace

bool StartScriptedRun(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    absl::FPrintF(stderr, "-SCRIPT: cannot read %s\n", path);
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  // Never destroyed: the run ends from inside one of its own steps, and the
  // Display it is attached to goes with the Game.
  static absl::NoDestructor<std::unique_ptr<engine::window::ScriptedRun>> run;
  *run = std::make_unique<engine::window::ScriptedRun>(
      engine::window::TheDisplay(), game::TheKeyboard(), game::TheMouse());

  const std::filesystem::path shot_directory =
      std::filesystem::absolute(path).parent_path();
  const engine::window::PlayerScriptHooks hooks{
      .take_shot =
          [shot_directory](const std::string_view name) {
            SaveShot(shot_directory / absl::StrCat(name, ".pcx"));
          },
      .quit =
          [] {
            ShutDown();
            // The step that is running cannot be returned to.
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            exit(EXIT_SUCCESS);
          },
  };
  const std::string error = (*run)->Load(text, hooks);
  if (!error.empty()) {
    absl::FPrintF(stderr, "-SCRIPT: %s: %s\n", path, error);
    return false;
  }
  return true;
}

}  // namespace td
