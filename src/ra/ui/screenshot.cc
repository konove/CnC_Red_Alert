#include "ra/ui/screenshot.h"

#include <cstdint>

#include "absl/strings/str_format.h"
#include "engine/file/disk_file.h"
#include "engine/file/file_access.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "ra/assets/audio.h"
#include "ra/assets/filepcx.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/core/config.h"
#include "ra/core/defines.h"
#include "ra/debug/debug_state.h"

namespace ra {

void Screenshot_Key(const engine::window::KeyNumber key) {
  if constexpr (config::kCheatKeysEnabled) {
    if ((key == engine::window::KN_K && !TheDebugState().map_editor_active() &&
         (TheDebugState().developer_mode() || TheDebugState().playtest())) &&
        (!TheDebugState().motion_capture())) {
      PixelBuffer temp_page(
          TheScreen().visible_view().width(),
          TheScreen().visible_view().height(), {},
          static_cast<int32_t>(TheScreen().visible_view().width()) *
              TheScreen().visible_view().height());
      char filename[30];

      TheScreen().visible_view().BlitTo(temp_page.view());
      for (int lp = 0; lp < 99; lp++) {
        absl::SNPrintF(filename, sizeof(filename), "scrsht%02d.pcx", lp);
        if (!FindExistingFile(filename).has_value()) {
          break;
        }
      }

      if (const auto file = OpenDiskFile(filename, FileAccess::kWrite)) {
        Write_PCX_File(*file, temp_page, ThePalettes().game_palette());
      }
      PlaySoundEffect(VOC_BEEP);
    }
  }
}

}  // namespace ra
