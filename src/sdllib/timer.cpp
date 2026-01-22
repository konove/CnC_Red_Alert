#include "sdllib/include/timer.h"

#include <SDL.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>

bool TimerSystemOn = false;

static Uint32 TimerCallback(Uint32 interval, void* param) {
  ((WinTimerClass*)param)->Update_Tick_Count();

  return interval;
}

// TimerClass/CountDownTimerClass are mostly used by TD
// (RA has it's own impl)
TimerClass::TimerClass(BaseTimerEnum timer, bool start) : TickType(timer) {
  if (start && TimerSystemOn) Start();
}

long TimerClass::Set(long value, bool start) {
  Started = 0;
  Accumulated = value;
  if (start) return Start();

  return Time();
}

long TimerClass::Start() {
  if (!Started) Started = Get_Ticks() + 1;
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
  if (WindowsTimer && TickType == BT_SYSTEM)  // BT_USER seems unused
    return WindowsTimer->Get_System_Tick_Count();

  return 0;
}

CountDownTimerClass::CountDownTimerClass(BaseTimerEnum timer, long set, bool on)
    : TimerClass(timer, on) {
  Set(set, on);
}

CountDownTimerClass::CountDownTimerClass(BaseTimerEnum timer, bool on)
    : TimerClass(timer, false), DelayTime(0) {
  if (on) Start();
}

long CountDownTimerClass::Set(long value, bool start) {
  DelayTime = value;
  TimerClass::Reset(start);
  return Time();
}

long CountDownTimerClass::Time() {
  long ticks = DelayTime - TimerClass::Time();

  if (ticks < 0) ticks = 0;

  return ticks;
}

WinTimerClass::WinTimerClass(std::uint32_t freq, bool /*partial*/) {
  SDL_Init(SDL_INIT_TIMER);
  TimerHandle = SDL_AddTimer(1000 / freq, TimerCallback, this);

  TimerSystemOn = TimerHandle != 0;

  // TickCount is a completely different type to TimerClass
  // (TTimerClass<SystemTimerClass>)
  // if(!partial)
  //    TickCount.Start();
}

WinTimerClass::~WinTimerClass() {
  SDL_RemoveTimer(TimerHandle);
  TimerSystemOn = false;
}

void WinTimerClass::Update_Tick_Count() {
  SysTicks++;
  UserTicks++;
}

std::uint64_t WinTimerClass::Get_System_Tick_Count() { return SysTicks; }

std::uint64_t WinTimerClass::Get_User_Tick_Count() { return UserTicks; }

uint32_t Get_Time_Ms() { return SDL_GetTicks(); }
