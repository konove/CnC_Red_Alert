// Named speeds for the objects that move.

#ifndef CNC_RED_ALERT_GAME_SIM_MPH_H_
#define CNC_RED_ALERT_GAME_SIM_MPH_H_

namespace game {

// Named speeds, in leptons a game frame. A speed is an `int` in 0..255 and
// most are computed (a rules value scaled, a maximum times a throttle), so
// these are constants and not an enum. The trailing number is the speed as the
// rules files write it, in percent of 256.
inline constexpr int kMphImmobile = 0;
inline constexpr int kMphVerySlow = 5;       // 2
inline constexpr int kMphKindaSlow = 6;      // 3
inline constexpr int kMphSlow = 8;           // 4
inline constexpr int kMphSlowIsh = 10;       // 5
inline constexpr int kMphMediumSlow = 12;    // 6
inline constexpr int kMphMedium = 18;        // 9
inline constexpr int kMphMediumFast = 30;    // 12
inline constexpr int kMphMediumFaster = 35;  // 14
inline constexpr int kMphFast = 40;          // 16
inline constexpr int kMphRocket = 60;        // 24
inline constexpr int kMphVeryFast = 100;     // 40
inline constexpr int kMphLightSpeed = 255;   // 100

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_MPH_H_
