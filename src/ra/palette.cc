#include "ra/palette.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "port/bytes_of.h"
#include "ra/externs.h"
#include "sdllib/gbuffer.h"
#include "sdllib/ww_win.h"
#include "tech/ftimer.h"
#include "tech/rgb.h"

PaletteClass PaletteClass::CurrentPalette;

unsigned char* CurrentPalette = PaletteClass::CurrentPalette;

PaletteClass::PaletteClass(const RGBClass& col) noexcept {
  for (auto& i : data_) {
    i = col;
  }
}

void PaletteClass::Set(int fade, void (*callback)()) {
  if (fade) {
    // fade to new palette
    const auto start_time = TickCount.Value();

    PaletteClass fade_palette;

    while (true) {
      const int cur_time = static_cast<int>(
          std::min<int64_t>(TickCount.Value() - start_time, fade));

      const auto old_bytes = CurrentPalette.bytes();
      const auto new_bytes = bytes();
      const auto out_bytes = fade_palette.bytes();

      for (int c = 0; c < COLOR_COUNT * 3; c++) {
        const int new_val = base::At(new_bytes, static_cast<size_t>(c)) & 0x3F;
        const int old_val = base::At(old_bytes, static_cast<size_t>(c)) & 0x3F;
        base::At(out_bytes, static_cast<size_t>(c)) =
            static_cast<unsigned char>(old_val +
                                       ((new_val - old_val) * cur_time / fade));
      }

      Do_Set_Palette(fade_palette);
      if (callback) {
        callback();
      } else {
        // make sure we actually display the fade
        Video_End_Frame();
      }

      if (cur_time == fade) {
        break;
      }
    }
  }

  CurrentPalette = *this;
  Do_Set_Palette(*this);
}

// the only code that uses these two (Play_Movie and OptionsClass::Proccess)
// only use it to adjust the black palette then immediately adjust it back
// presumably this is to force a palette update
void PaletteClass::Adjust(int /*unused*/) {}

void PaletteClass::Adjust(int /*unused*/, PaletteClass& /*unused*/) {}

void PaletteClass::Partial_Adjust(int /*unused*/, char* /*unused*/) {
  absl::PrintF("PaletteClass::%s\n", __func__);
}

void PaletteClass::Partial_Adjust(int /*unused*/, PaletteClass& /*unused*/,
                                  char* /*unused*/) {
  absl::PrintF("PaletteClass::%s\n", __func__);
}

int PaletteClass::Closest_Color(const RGBClass& col) const {
  int index = -1;
  int diff = 256 * 3;

  for (int i = 0; i < COLOR_COUNT; i++) {
    const int new_diff =
        std::abs(col.Red_Component() - base::At(data_, i).Red_Component()) +
        std::abs(col.Green_Component() - base::At(data_, i).Green_Component()) +
        std::abs(col.Blue_Component() - base::At(data_, i).Blue_Component());

    if (new_diff == 0) {
      return i;
    }

    if (new_diff < diff) {
      index = i;
      diff = new_diff;
    }
  }

  return index;
}

RGBClass& PaletteClass::at(int index) { return base::At(data_, index); }

const RGBClass& PaletteClass::at(int index) const {
  return base::At(data_, index);
}

PaletteClass::operator unsigned char*() noexcept {
  return port::BytesOf(data_);
}

PaletteClass::operator const unsigned char*() const {
  return port::BytesOf(data_);
}

void Set_Palette(std::span<const unsigned char> palette) {
  if (palette.size() < static_cast<size_t>(PaletteClass::COLOR_COUNT) * 3) {
    return;
  }
  base::CopyBytes(std::as_writable_bytes(PaletteClass::CurrentPalette.bytes()),
                  std::as_bytes(palette), PaletteClass::COLOR_COUNT * 3);
  Do_Set_Palette(palette);
}
