// File: Presentation, the pages Tiberian Dawn's full-screen sequences draw on.

#ifndef CNC_RED_ALERT_TD_PRESENTATION_H_
#define CNC_RED_ALERT_TD_PRESENTATION_H_

#include <array>

#include "absl/base/attributes.h"
#include "sdllib/pixel_buffer.h"

// The two pages a full-screen presentation draws through - the score screens,
// the map selection, the side choice and the Nod ending.
//
// Art is drawn into page() at the 320x200 it was made for and scaled to twice
// its size onto the screen, so the whole sequence looks the way it did on a
// VGA card. Text would come out doubled with it, so captions are printed into
// text_page(), which is the size of the screen, and only the rectangles that
// changed are brought forward: AddTextRect() records one and DrawTextRects()
// blits them all onto the hidden page, over the art that is already there.
//
// A presentation builds one on the stack and passes it down; nothing outlives
// the sequence that made it.
//
// Example:
//   Presentation show;
//   show.page().view().Clear();
//   show.text_page().view().Print("MISSION ACCOMPLISHED", 0, 0, kWhite,
//   kBlack); show.AddTextRect(0, 0, 0, 0, 320, 16); show.DrawTextRects();
class Presentation {
 public:
  // The screen must already have its video mode: the text page is sized to it.
  Presentation();
  ~Presentation() = default;
  Presentation(const Presentation&) = delete;
  Presentation& operator=(const Presentation&) = delete;
  Presentation(Presentation&&) = delete;
  Presentation& operator=(Presentation&&) = delete;

  // The art page, 320x200, which reaches the screen scaled to twice its size.
  PixelBuffer& page() ABSL_ATTRIBUTE_LIFETIME_BOUND { return page_; }
  // The caption page, the size of the screen, which reaches it unscaled.
  PixelBuffer& text_page() ABSL_ATTRIBUTE_LIFETIME_BOUND { return text_page_; }

  // Records a `width` x `height` rectangle of the text page at x,y to be
  // blitted to dest_x,dest_y by the next DrawTextRects(). Silently drops the
  // rectangle once kMaxTextRects are queued.
  void AddTextRect(int x, int y, int dest_x, int dest_y, int width, int height);
  // Forgets every queued rectangle.
  void ClearTextRects();
  // Blits every queued rectangle from the text page onto the hidden page,
  // leaving pixel 0 transparent so the art shows through around the letters.
  // The queue is left alone, because the same captions are redrawn every tick.
  void DrawTextRects();

 private:
  // The most rectangles one tick can queue. Any beyond this are dropped.
  static constexpr int kMaxTextRects = 128;

  struct TextRect {
    int source_x;
    int source_y;
    int dest_x;
    int dest_y;
    int width;
    int height;
  };

  PixelBuffer page_{320, 200};
  PixelBuffer text_page_;
  std::array<TextRect, kMaxTextRects> text_rects_{};
  int text_rect_count_ = 0;
};

#endif  // CNC_RED_ALERT_TD_PRESENTATION_H_
