#ifndef CNC_RED_ALERT_TD_STARTUP_STARTUP_H_
#define CNC_RED_ALERT_TD_STARTUP_STARTUP_H_

namespace engine::file {
class IniFile;
}  // namespace engine::file

namespace td {

struct StartupOptions;

// Tiberian Dawn's shared exit path. main() itself lives in startup.cc.

// Cleans up the engine systems the game started (audio, mouse)
// before the process exits. ShutDown() calls it.
void ShutDownEngine();

// Releases everything the game set up - ShutDownEngine(), the video pages and
// the Game - so the caller only has to exit. Every way out of the game goes
// through it: every return from main(), the fatal-error and missing-CD exits
// and the SDL quit handler. It does not draw, so it is safe before the video
// pages exist.
void ShutDown();

// Applies the settings CONQUER.INI holds that the game needs before it opens
// its window: the screen height, the 1.07 compatibility flag, the IPX socket
// and the bridge network. What the command line asked for in `options` wins
// over the file.
void ApplySetupOptions(const engine::file::IniFile& ini,
                       const StartupOptions& options);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_STARTUP_STARTUP_H_
