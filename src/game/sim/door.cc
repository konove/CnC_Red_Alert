#include "game/sim/door.h"

#include "absl/log/check.h"

namespace game {

void Door::AI() {
  if (frames_per_stage_ <= 0) {
    return;
  }
  --countdown_;
  if (countdown_ > 0) {
    return;
  }
  ++stage_;
  countdown_ = frames_per_stage_;
  if (stage_ >= final_stage_) {
    frames_per_stage_ = 0;
    if (state_ == State::kOpening) {
      state_ = State::kOpen;
    } else if (state_ == State::kClosing) {
      state_ = State::kClosed;
    }
  }
  needs_redraw_ = true;
}

int Door::Frame() const {
  switch (state_) {
    case State::kClosing:
      return final_stage_ - 1 - stage_;
    case State::kOpening:
      return stage_;
    case State::kOpen:
      return final_stage_ - 1;
    case State::kClosed:
    default:
      return 0;
  }
}

bool Door::Open(const int frames_per_stage, const int stage_count) {
  if (state_ != State::kClosed && state_ != State::kClosing) {
    return false;
  }
  StartAnimation(State::kOpening, frames_per_stage, stage_count);
  return true;
}

bool Door::Close(const int frames_per_stage, const int stage_count) {
  if (state_ != State::kOpen && state_ != State::kOpening) {
    return false;
  }
  StartAnimation(State::kClosing, frames_per_stage, stage_count);
  return true;
}

void Door::StartAnimation(const State state, const int frames_per_stage,
                          const int stage_count) {
  // Fewer than two stages would leave an open door on frame -1.
  DCHECK_GE(stage_count, 2);
  DCHECK_GT(frames_per_stage, 0);
  state_ = state;
  final_stage_ = stage_count - 1;
  stage_ = 0;
  countdown_ = frames_per_stage;
  frames_per_stage_ = frames_per_stage;
}

}  // namespace game
