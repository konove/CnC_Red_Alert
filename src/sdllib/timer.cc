#include "sdllib/timer.h"

#include <SDL.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>

#include <algorithm>

bool TimerSystemOn = false;

static Uint32 TimerCallback(Uint32 interval, void* param) {
  static_cast<WinTimerClass*>(param)->UpdateTickCount();

  return interval;
}

// TimerClass/CountDownTimerClass are mostly used by TD
// (RA has it's own impl)
TimerClass::TimerClass(BaseTimerEnum timer, bool start) : TickType(timer) {
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
  if (!Started) {
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
  if (WindowsTimer && TickType == BT_SYSTEM) {  // BT_USER seems unused
    return WindowsTimer->TickCount();
  }

  return 0;
}

CountDownTimerClass::CountDownTimerClass(BaseTimerEnum timer, long set, bool on)
    : TimerClass(timer, on) {
  Set(set, on);
}

CountDownTimerClass::CountDownTimerClass(BaseTimerEnum timer, bool on)
    : TimerClass(timer, false), DelayTime(0) {
  if (on) {
    Start();
  }
}

long CountDownTimerClass::Set(long value, bool start) {
  DelayTime = value;
  TimerClass::Reset(start);
  return Time();
}

long CountDownTimerClass::Time() {
  return std::max<long>(DelayTime - TimerClass::Time(), 0);
}

WinTimerClass::WinTimerClass(const int tick_rate) {
  SDL_Init(SDL_INIT_TIMER);
  timer_id_ = SDL_AddTimer(1000 / tick_rate, TimerCallback, this);

  TimerSystemOn = timer_id_ != 0;
}

WinTimerClass::~WinTimerClass() {
  SDL_RemoveTimer(timer_id_);
  TimerSystemOn = false;
}

uint32_t Get_Time_Ms() { return SDL_GetTicks(); }
