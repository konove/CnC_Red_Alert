#include "td/assets/presentation.h"

#include <span>

#include "base/array.h"
#include "engine/window/display.h"
#include "td/assets/screen.h"

namespace td {

Presentation::Presentation()
    : text_page_(TheScreen().visible_view().width(),
                 TheScreen().visible_view().height()) {
  text_page_.view().Clear();
}

Presentation::~Presentation() { HidePage(); }

// The page is a plain buffer with no surface behind it, so it needs no lock.
void Presentation::ShowPage() {
  engine::window::TheDisplay().PresentBackdrop(page_.bytes(), page_.width(),
                                               page_.height());
}

// Ends this presentation's art, though the art lives in the Display.
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Presentation::HidePage() { engine::window::TheDisplay().ClearBackdrop(); }

void Presentation::AddTextRect(int x, int y, int dest_x, int dest_y, int width,
                               int height) {
  if (text_rect_count_ >= kMaxTextRects) {
    return;
  }
  base::At(std::span(text_rects_), text_rect_count_) = {.source_x = x,
                                                        .source_y = y,
                                                        .dest_x = dest_x,
                                                        .dest_y = dest_y,
                                                        .width = width,
                                                        .height = height};
  text_rect_count_++;
}

void Presentation::ClearTextRects() { text_rect_count_ = 0; }

void Presentation::DrawTextRects() {
  PixelView& source = text_page_.view();
  PixelView& dest = TheScreen().hidden_view();
  if (source.Lock()) {
    if (dest.Lock()) {
      for (int i = 0; i < text_rect_count_; i++) {
        const auto& [source_x, source_y, dest_x, dest_y, width, height] =
            base::At(std::span(text_rects_), i);
        source.BlitToLocked(dest, source_x, source_y, dest_x, dest_y, width,
                            height, /*transparent=*/false);
      }
      dest.Unlock();
    }
    source.Unlock();
  }
}

}  // namespace td
