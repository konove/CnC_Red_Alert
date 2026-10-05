#include "ra/startup/scripted_run.h"

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
#include "engine/file/disk_file.h"
#include "engine/file/file_access.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/display.h"
#include "engine/window/player_script.h"
#include "engine/window/scripted_run.h"
#include "game/ui/input.h"
#include "ra/assets/filepcx.h"
#include "ra/assets/palette.h"
#include "ra/startup/startup.h"

namespace ra {

namespace {

// Saves what the window shows as the PCX file `path`, in the palette on
// screen, which during a fade is not the game's own.
void SaveShot(const std::filesystem::path& path) {
  engine::window::DisplayPicture picture =
      engine::window::TheDisplay().Capture();
  if (picture.pixels.empty()) {
    return;
  }
  PixelBuffer page(picture.width, picture.height, picture.pixels);
  const auto file = OpenDiskFile(path.string(), FileAccess::kWrite);
  if (!file) {
    absl::FPrintF(stderr, "-SCRIPT: cannot write %s\n", path.string());
    return;
  }
  Write_PCX_File(*file, page, PaletteClass::CurrentPalette);
}

}  // namespace

bool StartScriptedRun(const std::string& path, const bool real_time) {
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
      engine::window::TheDisplay(), game::TheKeyboard(), game::TheMouse(),
      real_time ? engine::window::ScriptedRun::Pace::kRealTime
                : engine::window::ScriptedRun::Pace::kFrames);

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

}  // namespace ra
