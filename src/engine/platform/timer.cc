#include "engine/platform/timer.h"

#include <SDL.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>

#include <cstdint>
#include <memory>

#include "absl/log/check.h"

TickTimer* g_tick_timer = nullptr;

namespace {

// Owns the timer g_tick_timer points to.
std::unique_ptr<TickTimer> tick_timer_owner;

}  // namespace

static Uint32 TimerCallback(Uint32 interval, void* param) {
  static_cast<TickTimer*>(param)->UpdateTickCount();

  return interval;
}

TickTimer::TickTimer(const int tick_rate)
    : timer_id_(SDL_AddTimer(static_cast<Uint32>(1000 / tick_rate),
                             TimerCallback, this)) {
  SDL_Init(SDL_INIT_TIMER);
}

TickTimer::~TickTimer() {
  SDL_RemoveTimer(timer_id_);
}

uint32_t Get_Time_Ms() { return SDL_GetTicks(); }

void InitTickTimer(int tick_rate) {
  CHECK(g_tick_timer == nullptr);
  tick_timer_owner = std::make_unique<TickTimer>(tick_rate);
  g_tick_timer = tick_timer_owner.get();
}

void ShutdownTickTimer() {
  g_tick_timer = nullptr;
  tick_timer_owner.reset();
}
