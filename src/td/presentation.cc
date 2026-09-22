#include "td/presentation.h"

#include <span>

#include "base/array.h"
#include "td/screen.h"

Presentation::Presentation()
    : text_page_(TheScreen().visible_view().width(),
                 TheScreen().visible_view().height()) {
  text_page_.view().Clear();
}

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
  PixelView& dest = TheScreen().hidden_view();
  if (!dest.Lock()) {
    return;
  }
  for (int i = 0; i < text_rect_count_; i++) {
    const TextRect& rect = base::At(std::span(text_rects_), i);
    text_page_.view().BlitTo(dest, rect.source_x, rect.source_y, rect.dest_x,
                             rect.dest_y, rect.width, rect.height, true);
  }
  dest.Unlock();
}
