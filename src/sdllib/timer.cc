#include "sdllib/timer.h"

#include <SDL.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>

#include <algorithm>

#include "absl/log/check.h"

bool TimerSystemOn = false;

TickTimer* g_tick_timer = nullptr;

static Uint32 TimerCallback(Uint32 interval, void* param) {
  static_cast<TickTimer*>(param)->UpdateTickCount();

  return interval;
}

// TimerClass/CountDownTimerClass are mostly used by TD
// (RA has its own impl)
TimerClass::TimerClass(bool start) {
  if (start && TimerSystemOn) {
    Start();
  }
}

long TimerClass::Set(long value, bool start) {
  Started = 0;
  Accumulated = value;
  if (start) {
    return Start();
  }

  return Time();
}

long TimerClass::Start() {
  if (Started == 0) {
    Started = Get_Ticks() + 1;
  }
  return Time();
}

long TimerClass::Time() {
  if (Started) {
    long ticks = Get_Ticks();
    Accumulated += ticks - (Started - 1);
    Started = ticks + 1;
  }
  return Accumulated;
}

long TimerClass::Get_Ticks() {
  if (g_tick_timer) {
    return g_tick_timer->TickCount();
  }

  return 0;
}

CountDownTimerClass::CountDownTimerClass(long set, bool on) : TimerClass(on) {
  Set(set, on);
}

CountDownTimerClass::CountDownTimerClass(bool on)
    : TimerClass(false), DelayTime(0) {
  if (on) {
    Start();
  }
}

void CountDownTimerClass::Set(long value, bool start) {
  DelayTime = value;
  TimerClass::Reset(start);
}

long CountDownTimerClass::Time() {
  return std::max<long>(DelayTime - TimerClass::Time(), 0);
}

TickTimer::TickTimer(const int tick_rate) {
  SDL_Init(SDL_INIT_TIMER);
  timer_id_ = SDL_AddTimer(1000 / tick_rate, TimerCallback, this);

  TimerSystemOn = timer_id_ != 0;
}

TickTimer::~TickTimer() {
  SDL_RemoveTimer(timer_id_);
  TimerSystemOn = false;
}

uint32_t Get_Time_Ms() { return SDL_GetTicks(); }

void InitTickTimer(int tick_rate) {
  CHECK(g_tick_timer == nullptr);
  g_tick_timer = new TickTimer(tick_rate);
}

void ShutdownTickTimer() {
  delete g_tick_timer;
  g_tick_timer = nullptr;
}
