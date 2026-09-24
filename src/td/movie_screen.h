// File: MovieScreen, where Tiberian Dawn shows the frames of a VQA movie.

#ifndef CNC_RED_ALERT_TD_MOVIE_SCREEN_H_
#define CNC_RED_ALERT_TD_MOVIE_SCREEN_H_

#include "winvq/vqa32/vqa_player.h"

// Copies each frame, centered, into the 320x200 system memory page, sets its
// palette, and scales the page to the screen. Esc stops the movie where
// breaking out is allowed; losing the window's focus pauses it.
//
// Example:
//   MovieScreen screen;
//   if (auto player = VqaPlayer::Open(io, "GDI1.VQA", screen, audio)) {
//     player->Run();
//   }
class MovieScreen final : public VqaClient {
 public:
  bool OnFrame(const VqaFrameView& frame) override;
  // A dropped frame still presents and reads the keyboard, so Esc works
  // while playback catches up.
  bool OnFrameSkipped(int frame_number) override;
  // Too early for the next frame: wait for the display's next frame.
  void OnIdle() override;

  // Whether the player pressed Esc to stop the movie.
  [[nodiscard]] bool broken_out() const { return broken_out_; }

 private:
  // Shows the page and services the keyboard and the window's focus. Returns
  // false when the player pressed Esc to stop the movie.
  bool Present();

  bool broken_out_ = false;
};

#endif  // CNC_RED_ALERT_TD_MOVIE_SCREEN_H_
