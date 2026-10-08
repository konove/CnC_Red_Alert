#include "td/ui/screenshot.h"

#include <cstdint>
#include <filesystem>

#include "absl/strings/str_format.h"
#include "engine/gfx/pcx_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "td/assets/palette.h"
#include "td/assets/screen.h"
#include "td/core/config.h"

namespace td {

void Screenshot_Key(const engine::window::KeyNumber key) {
  if constexpr (config::kScenarioEditorEnabled) {
    if (key == engine::window::KN_K) {
      PixelBuffer temp_page(
          TheScreen().visible_view().width(),
          TheScreen().visible_view().height(), {},
          static_cast<int32_t>(TheScreen().visible_view().width()) *
              TheScreen().visible_view().height());
      char filename[30];

      TheScreen().visible_view().BlitTo(temp_page.view());
      for (int lp = 0; lp < 99; lp++) {
        absl::SNPrintF(filename, sizeof(filename), "scrsht%02d.pcx", lp);
        if (!std::filesystem::exists(filename)) {
          break;
        }
      }

      engine::gfx::WritePcxFile(filename, temp_page.view(), CurrentPalette);
    }
  }
}

}  // namespace td
