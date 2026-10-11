#include "game/sim/door.h"

#include <algorithm>

#include "absl/log/check.h"

namespace game {

bool Door::AI() {
  if (frames_per_stage_ <= 0) {
    return false;
  }
  --countdown_;
  if (countdown_ > 0) {
    return false;
  }
  ++stage_;
  countdown_ = frames_per_stage_;
  if (stage_ >= frame_count_) {
    frames_per_stage_ = 0;
    if (state_ == State::kOpening) {
      state_ = State::kOpen;
    } else if (state_ == State::kClosing) {
      state_ = State::kClosed;
    }
  }
  return true;
}

int Door::Frame() const {
  switch (state_) {
    case State::kClosing:
      return frame_count_ - 1 - stage_;
    case State::kOpening:
      return stage_;
    case State::kOpen:
      return frame_count_ - 1;
    case State::kClosed:
    default:
      return 0;
  }
}

bool Door::Open(const int frames_per_stage, const int frame_count) {
  if (state_ != State::kClosed && state_ != State::kClosing) {
    return false;
  }
  StartAnimation(State::kOpening, frames_per_stage, frame_count);
  return true;
}

bool Door::Close(const int frames_per_stage, const int frame_count) {
  if (state_ != State::kOpen && state_ != State::kOpening) {
    return false;
  }
  StartAnimation(State::kClosing, frames_per_stage, frame_count);
  return true;
}

void Door::StartAnimation(const State state, const int frames_per_stage,
                          const int frame_count) {
  // Without a frame an open door would be on frame -1.
  DCHECK_GE(frame_count, 1);
  DCHECK_GT(frames_per_stage, 0);
  // Read the frame before the state it depends on changes. The clamp only
  // matters when a door turns round with a different frame count.
  const int frame = std::clamp(Frame(), 0, frame_count - 1);
  stage_ = state == State::kOpening ? frame : frame_count - 1 - frame;
  state_ = state;
  frame_count_ = frame_count;
  countdown_ = frames_per_stage;
  frames_per_stage_ = frames_per_stage;
}

}  // namespace game
