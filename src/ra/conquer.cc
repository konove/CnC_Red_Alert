/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// File: The main game loop.
//
// RunGame() owns the outer loop -- pick a game, play it, tear it down -- and
// RunFrame() runs one frame of it, pacing itself with WaitForNextFrame().
// ServiceRealTime() is the real-time servicing (sound, music, network) that
// also runs inside every blocking loop and dialog.
//
// Originally CONQUER.CPP, by Joe L. Bostic, started April 3, 1991.

#include "ra/conquer.h"

#include <absl/log/check.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/aircraft.h"  // IWYU pragma: keep
#include "ra/audio.h"
#include "ra/ccptr.h"
#include "ra/chat.h"
#include "ra/config.h"
#include "ra/debug_state.h"
#include "ra/defines.h"
#include "ra/display.h"
#include "ra/event.h"
#include "ra/filepcx.h"
#include "ra/game_clock.h"
#include "ra/game_state.h"
#include "ra/goptions.h"
#include "ra/heap.h"
#include "ra/hotkeys.h"
#include "ra/house.h"
#include "ra/infantry.h"  // IWYU pragma: keep
#include "ra/init.h"
#include "ra/input.h"
#include "ra/interpal.h"
#include "ra/jshell.h"
#include "ra/language.h"
#include "ra/logic.h"
#include "ra/mapedit.h"
#include "ra/mplayer.h"
#include "ra/msgbox.h"
#include "ra/msglist.h"
#include "ra/netdlg.h"
#include "ra/network.h"
#include "ra/nulldlg.h"
#include "ra/nullmgr.h"
#include "ra/object.h"
#include "ra/object_heaps.h"
#include "ra/palette.h"
#include "ra/palettes.h"
#include "ra/queue.h"
#include "ra/rawolapi.h"
#include "ra/record_playback.h"
#include "ra/rules.h"
#include "ra/saveload.h"
#include "ra/scenario.h"
#include "ra/score.h"
#include "ra/screen.h"
#include "ra/session.h"
#include "ra/special.h"
#include "ra/startup_options.h"
#include "ra/stats.h"
#include "ra/text_ids.h"
#include "ra/theme.h"
#include "ra/unit.h"  // IWYU pragma: keep
#include "ra/vector_dynamic.h"
#include "ra/version.h"
#include "ra/vessel.h"  // IWYU pragma: keep
#include "ra/vortex.h"
#include "ra/winstub.h"
#include "ra/wolapiob.h"
#include "ra/wolstrng.h"
#include "ra/world.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "sdllib/wwstd.h"
#include "tech/audio_mixer.h"
#include "tech/disk_file.h"
#include "tech/fixed.h"
#include "tech/ftimer.h"
#include "tech/game_file.h"
#include "tech/glow_pulse.h"
#include "tech/rgb.h"

// Cycles the animated palette entries. Two effects run off independent timers:
// a white that pulses between bright and half-dark, used by the radar box and
// other interface glows, and a rotation of the water colours.
//
// This needs to run at least 8 times a second to look smooth, which is why
// WaitForNextFrame() calls it while idling rather than the main loop calling it
// once per frame.
static void CyclePalette() {
  // The embers' slot holds the last faded value, so each pulse fades this
  // anew rather than reading the slot back.
  constexpr RGBClass kEmberBase(255, 80, 80);

  static Timer<SystemTickSource> water_timer;
  static GlowPulse<SystemTickSource> pulse(kTimerSecond / 6);

  if (!TheOptions().IsPaletteScroll) {
    return;
  }

  bool palette_changed = false;
  if (pulse.Update()) {
    ThePalettes().game_palette().at(kPulseColor) =
        pulse.Apply(ThePalettes().game_palette().at(kWhite));
    ThePalettes().game_palette().at(kEmberColor) = pulse.Apply(kEmberBase);
    palette_changed = true;
  }

  if (water_timer.IsFinished()) {
    water_timer.Set(kTimerSecond / 4);

    // Each water colour moves up one slot; the last wraps round to the first.
    const auto water_colors =
        ThePalettes()
            .game_palette()
            .colors()
            .subspan<kCycleColorStart, kCycleColorCount>();
    std::ranges::rotate(water_colors, water_colors.end() - 1);

    palette_changed = true;
  }

  // Either effect leaves the palette changed only in memory until it is
  // passed to the system.
  if (palette_changed) {
    ThePalettes().game_palette().Set();
  }
}

// Reads one input event from the map and dispatches any keypress. The mouse is
// erased from the hidden page first so the next render draws it afresh.
static void ProcessInput() {
  TheMouse()->Erase_Mouse(&TheScreen().hidden_view(), true);
  KeyNumType input = KN_NONE;
  int x = 0;
  int y = 0;
  TheMap().Input(input, x, y);
  if (input != KN_NONE) {
    Keyboard_Process(input);
  }
}

// The map editor's stand-in for RunFrame(): render, take input, and keep the
// real-time callbacks alive so music continues. No game logic runs, so the
// scenario stays frozen while it is edited.
//
// Returns true when the game should end.
static bool RunMapEditorFrame() {
  TheMap().Render();
  ProcessInput();

  ServiceRealTime();  // maintains Theme.AI() for music
  CyclePalette();

  return !TheGameState().active();
}

// Runs the dialog SpecialDialog asks for, then clears the request. The dialogs
// call RunFrame() themselves so the game keeps running behind them, which is
// why this is invoked between frames rather than from inside one.
static void RunPendingDialog() {
  const SpecialDialogType dialog = TheGameState().special_dialog();
  if (dialog == SDLG_NONE) {
    return;
  }

  TheMap().Help_Text(TXT_NONE);
  TheMap().Override_Mouse_Shape(MOUSE_NORMAL, false);
  switch (dialog) {
    case SDLG_SPECIAL:
      Special_Dialog();
      break;

    case SDLG_OPTIONS:
      TheOptions().Process();
      break;

    case SDLG_SURRENDER:
      if (Surrender_Dialog(TXT_SURRENDER)) {
        if constexpr (config::kScenarioEditorEnabled) {
          ThePlayer()->Flag_To_Lose();
        } else {
          TheNetwork().out_list().Add(EventClass(EventClass::DESTRUCT));
        }
      }
      break;

    case SDLG_NONE:
    default:
      break;
  }
  TheGameState().special_dialog() = SDLG_NONE;
  TheMap().Revert_Mouse_Shape();
}

// Per-scenario setup that Select_Game() leaves to the caller: vortex remap
// tables, the palette, and the mouse and statistics state for the session type.
static void BeginScenario() {
  TheWorld().scenario_init() = 0;

  TheWorld().chronal_vortex().Stop();
  TheWorld().chronal_vortex().Setup_Remap_Tables(TheScenario().Theater);

  // This PRESUMES that Select_Game() has told the map to draw itself.
  ThePalettes().game_palette().Set(kFadePaletteMedium);
  TheKeyboard().Clear();

  // A recording drives the view on playback, so there is no mouse to show.
  if (TheSession().Play) {
    Hide_Mouse();
    ResetRecordedEvents();
  } else {
    Show_Mouse();
  }

  if (TheSession().Type == GAME_INTERNET) {
    Register_Game_Start_Time();
    TheNetwork().statistics_sent() = false;
    TheNetwork().packet_later() = nullptr;
    TheNetwork().connection_lost() = false;
  }
}

// Runs frames, and any dialogs they request, until the scenario ends.
static void RunScenario() {
  for (;;) {
    // A plain `if`: a discarded `if constexpr` branch would leave
    // RunMapEditorFrame() unreferenced in builds without the editor.
    if (config::kScenarioEditorEnabled && TheDebugState().map_editor_active()) {
      if (RunMapEditorFrame()) {
        return;
      }
      continue;
    }

    TheWorld().time_quake() = TheWorld().pending_time_quake();
    TheWorld().pending_time_quake() = false;
    if (RunFrame()) {
      return;
    }

    RunPendingDialog();
  }
}

// Tears down what the finished scenario leaves behind.
//
// The modem and network are shut down rather than left running, so that
// selecting them again in Select_Game() restarts them from a known state.
// Playback never initialized either, so it skips this.
static void EndScenario() {
  if (!TheNetwork().statistics_sent() && TheNetwork().packet_later()) {
    Send_Statistics_Packet();  // After game sending if
                               // PacketLater set.
  }

  ThePalettes().black_palette().Set(kFadePaletteSlow);
  TheScreen().visible_page().Clear();

  if (TheSession().Record || TheSession().Play) {
    TheSession().RecordFile.Close();
  }

  if (!TheSession().Play) {
    if (TheSession().Type == GAME_NULL_MODEM ||
        TheSession().Type == GAME_MODEM) {
      Modem_Signoff();
    } else if (TheSession().Type == GAME_IPX) {
      Shutdown_Network();
    }
  }

  // Return from playback to the main menu with the mouse visible again.
  if (TheSession().Play) {
    Show_Mouse();
    TheSession().Type = GAME_NORMAL;
    TheSession().Play = false;
  }
}

// The game's entry point, after platform startup. Init_Game() does the
// one-time initialization; everything after it happens once per game played,
// because Select_Game() may hand back a wholly different kind of session --
// single player, network, modem, editor -- each needing its own setup and its
// own teardown.
void RunGame() {
  if (!Init_Game()) {
    return;
  }

  while (Select_Game(true)) {
    BeginScenario();
    RunScenario();
    EndScenario();
  }

  TheSession().Free_Scenario_Descriptions();
}

// Gives the Westwood Online chat and matchmaking objects a slice of time
// to deliver whatever their servers have queued.
//
// Both PumpMessages HRESULT's are dropped on purpose: a lost connection is
// reported through the callbacks they fire, which set
// pWolapi->bConnectionDown (rawolapi.cc), and every caller tests
// that flag instead. Only reachable when config::kWolapiEnabled, hence
// maybe_unused.
[[maybe_unused]] static void PumpWolapiMessages() {
  static_cast<void>(TheNetwork().wolapi()->pChat->PumpMessages());
  static_cast<void>(TheNetwork().wolapi()->pNetUtil->PumpMessages());
}

// Keeps the Westwood Online connection serviced. In a game it pumps more
// slowly and announces a dropped connection; outside one it pumps only while a
// modal dialog over the chat screen has asked for it.
[[maybe_unused]] static void ServiceWolapi() {
  if (!TheNetwork().wolapi() ||
      Get_Time_Ms() <= TheNetwork().wolapi()->dwTimeNextWolapiPump) {
    return;
  }

  if (!TheNetwork().wolapi()->bInGame) {
    if (TheNetwork().wolapi()->bPump_In_Call_Back) {
      PumpWolapiMessages();
      TheNetwork().wolapi()->dwTimeNextWolapiPump =
          Get_Time_Ms() + WOLAPIPUMPWAIT;
    }
    return;
  }

  if (TheNetwork().wolapi()->bConnectionDown) {
    return;
  }
  PumpWolapiMessages();
  TheNetwork().wolapi()->dwTimeNextWolapiPump =
      Get_Time_Ms() + WOLAPIPUMPWAIT + 700;
  if (TheNetwork().wolapi()->bConnectionDown) {
    // The Wolapi object is kept rather than deleted, so that the game results
    // can still be sent.
    TheSession().Messages.Add_Message(
        nullptr, 0, TXT_WOL_WOLAPIGONE, PCOLOR_GOLD,
        TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
        TheRules().MessageDelay * kTicksPerMinute);
    PlaySoundEffect(WOLSOUND_LOGOUT);
  }
}

void ServiceRealTime() {
  ServiceBackgroundTasks();
  Video_End_Frame();
}

void ServiceRealTimeFor(const int ticks) {
  const Timer<SystemTickSource> wait{ticks};
  do {
    ServiceRealTime();
  } while (wait.HasTimeLeft());
}

void ServiceBackgroundTasks() {
  // Music and speech maintenance
  if (TheAudio().is_open()) {
    TheAudio().PumpStreams();
    TheTheme().AI();
    ServiceSpeech();
  }

  // Network maintenance.
  if (TheSession().Type == GAME_IPX || TheSession().Type == GAME_INTERNET) {
    IPX_Call_Back();
  }

  // Serial game maintenance.
  if (TheSession().Type == GAME_NULL_MODEM ||
      (TheSession().Type == GAME_MODEM && TheSession().ModemService)) {
    TheNetwork().null_modem().Service();
  }

  if constexpr (config::kWolapiEnabled) {
    ServiceWolapi();
  }
}

// Holds the end of the current frame; StartFrameTimer() sets it and
// WaitForNextFrame() waits it out.
static Timer<SystemTickSource> frame_timer;

// Spins until the frame timer expires, holding the game to the rate set by the
// game-speed option.
//
// The wait is not idle: input, rendering and the real-time callbacks all run
// here. That keeps the interface responsive and the palette cycling smooth
// between logic frames, which tick far more slowly than the display does.
static void WaitForNextFrame() {
  while (frame_timer.HasTimeLeft()) {
    CyclePalette();
    ServiceRealTime();

    if (TheGameState().special_dialog() == SDLG_NONE) {
      ProcessInput();
      TheMap().Render();
    }
  }
  CyclePalette();
  ServiceRealTime();
}

// Sets the frame timer that WaitForNextFrame() waits out at the end of the
// frame.
//
// Multiplayer sessions run at the rate the machines negotiated, and playback as
// fast as possible. Otherwise the delay comes from the game-speed option, a
// tick slower on easy and a tick faster on hard.
static void StartFrameTimer() {
  if (TheSession().Type != GAME_NORMAL && TheSession().Type != GAME_SKIRMISH &&
      TheSession().CommProtocol == COMM_PROTOCOL_MULTI_E_COMP) {
    if (TheSession().Play) {
      frame_timer.Set(0);
      return;
    }
    // A zero rate was seen, rarely, and divided by zero.
    if (TheSession().DesiredFrameRate == 0) {
      TheSession().DesiredFrameRate = 60;
    }
    frame_timer.Set(kTimerSecond / TheSession().DesiredFrameRate);
    return;
  }

  int delay = static_cast<int>(TheOptions().GameSpeed);
  if (ThePlayer()->Difficulty == DIFF_EASY) {
    delay++;
  } else if (ThePlayer()->Difficulty == DIFF_HARD && delay > 0) {
    delay--;
  }
  frame_timer.Set(delay);
}

// How the scenario ended this frame, in the order the checks take priority.
enum class ScenarioOutcome { kNone, kWin, kLose, kRestart, kDraw };

// Reads the outcome flags that game logic raised during the frame. A draw needs
// both players of a two-player multiplayer game to have proposed it.
static ScenarioOutcome PendingOutcome() {
  if (TheGameState().player_wins()) {
    return ScenarioOutcome::kWin;
  }
  if (TheGameState().player_loses()) {
    return ScenarioOutcome::kLose;
  }
  if (TheGameState().player_restarts()) {
    return ScenarioOutcome::kRestart;
  }
  if (TheSession().Type != GAME_NORMAL && TheSession().Type != GAME_SKIRMISH &&
      TheSession().Players.Count() == 2 && TheScenario().bLocalProposesDraw &&
      TheScenario().bOtherProposesDraw) {
    return ScenarioOutcome::kDraw;
  }
  return ScenarioOutcome::kNone;
}

// Ends the scenario if this frame decided it: reports the result to the
// game-results server, clears the outcome flags and runs the matching end
// sequence. Returns true if the scenario ended.
static bool FinishScenarioIfDecided() {
  const ScenarioOutcome outcome = PendingOutcome();
  if (outcome == ScenarioOutcome::kNone) {
    return false;
  }

  if (outcome != ScenarioOutcome::kRestart &&
      TheSession().Type == GAME_INTERNET && !TheNetwork().statistics_sent()) {
    Register_Game_End_Time();
    Send_Statistics_Packet();
  }

  TheMouse()->Erase_Mouse(&TheScreen().hidden_view(), true);
  TheGameState().player_wins() = false;
  TheGameState().player_loses() = false;
  TheGameState().player_restarts() = false;
  TheMap().Help_Text(TXT_NONE);

  switch (outcome) {
    case ScenarioOutcome::kWin:
      Do_Win();
      break;
    case ScenarioOutcome::kLose:
      Do_Lose();
      break;
    case ScenarioOutcome::kRestart:
      Do_Restart();
      break;
    case ScenarioOutcome::kDraw:
      Do_Draw();
      break;
    case ScenarioOutcome::kNone:
    default:
      break;
  }
  return true;
}

// Logs one line per object in `objects`, tagged with `kind`, for the
// save/load smoke test to diff.
template <typename T>
static void LogObjectPositions(const TFixedIHeapClass<T>& objects,
                               const std::string_view kind) {
  for (int index = 0; index < objects.Count(); index++) {
    const T* object = objects.Ptr(index);
    LOG(INFO) << "frame " << CurrentFrame() << " " << kind << " "
              << std::string_view(object->Class->Name()) << " coord "
              << absl::StrFormat("%08x", object->Coord) << " mission "
              << magic_enum::enum_name(object->Mission) << " navcom "
              << absl::StrFormat("%08x", object->NavCom);
  }
}

// -QUITFRAME<n>: logs where every mobile object is each frame, then ends the
// game at frame n, saving to -SAVESLOT<n> first if one was given. Together with
// -LOADGAME this checks that loaded objects keep moving without anyone at the
// keyboard. Returns true once the game has been ended.
static bool LogFrameAndQuitIfDue() {
  LogObjectPositions(TheObjectHeaps().unit(), "unit");
  LogObjectPositions(TheObjectHeaps().infantry(), "infantry");
  LogObjectPositions(TheObjectHeaps().vessel(), "vessel");
  LogObjectPositions(TheObjectHeaps().aircraft(), "aircraft");

  if (CurrentFrame() < TheStartupOptions().quit_at_frame) {
    return false;
  }
  if (TheStartupOptions().save_slot >= 0) {
    Save_Game(TheStartupOptions().save_slot, "debug");
  }
  TheGameState().active() = false;
  return true;
}

// Records the visible screen each frame for Rule.MovieTime minutes, then
// writes the frames out as cap0000.pcx, cap0001.pcx, ... and stops.
static void CaptureMotionFrame() {
  // One captured screen per element. Empty between runs. Deliberately leaked
  // rather than given static storage duration with a destructor, which would
  // run at exit after the graphics system is already gone.
  // LLVM 23 treats resize as invalidating the vector itself. The reference
  // remains valid, and element views are acquired only after resizing.
  // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
  static auto& frames = *new std::vector<std::vector<std::byte>>();
  // Doubles as the frame counter and the end-of-run signal: reaching
  // frames.size() ends the capture and flushes to disk.
  static base::ssize captured_count = 0;

  if (frames.empty()) {
    // Sized when a capture run starts rather than once per process, so that
    // an edit to MovieTime takes effect on the next run.
    const int frame_count = TheRules().MovieTime * kTicksPerMinute;
    frames.resize(base::ToSize(frame_count));
  }

  // Leaked for the same reason as frames above.
  static auto& frame_page = *new GraphicBufferClass(
      TheScreen().visible_view().width(), TheScreen().visible_view().height(),
      {},
      TheScreen().visible_view().width() * TheScreen().visible_view().height());

  const base::ssize frame_bytes =
      static_cast<base::ssize>(TheScreen().visible_view().width()) *
      TheScreen().visible_view().height();

  if (captured_count < std::ssize(frames)) {
    // A no-op on a frame reused from an earlier run of the same resolution.
    frames.at(base::ToSize(captured_count)).resize(base::ToSize(frame_bytes));

    TheScreen().visible_view().Blit(frame_page);
    base::CopyBytes(std::as_writable_bytes(
                        std::span(frames.at(base::ToSize(captured_count)))),
                    std::as_bytes(frame_page.Get_Bytes()), frame_bytes);
    captured_count++;
    return;
  }

  TheDebugState().set_motion_capture(false);

  DiskFile file;
  for (base::ssize index = 0; index < captured_count; index++) {
    base::CopyBytes(std::as_writable_bytes(frame_page.Get_Bytes()),
                    std::as_bytes(std::span(frames.at(base::ToSize(index)))),
                    frame_bytes);
    file.SetName(absl::StrFormat("cap%04d.pcx", index));

    Write_PCX_File(file, frame_page, &ThePalettes().game_palette());
  }

  // Release the run's buffers so that the next run re-reads MovieTime.
  frames.clear();
  frames.shrink_to_fit();
  captured_count = 0;
}

// Runs one frame of the game. See the declaration in conquer.h.
//
// Nothing that affects game state may be skipped here on the grounds that it is
// only visual -- every machine in a multiplayer game runs this same sequence
// and must arrive at the same state, or the session desyncs.
bool RunFrame() {
  if (!TheGameState().active()) {
    return true;
  }

  Check_For_Focus_Loss();

  // Sync-bug trapping code
  if (CurrentFrame() >= TheSession().TrapFrame) {
    TheSession().Trap_Object();
  }

  // Initialize our AI processing timer
  TheSession().ProcessTimer = SystemTicks();

  // If there is no theme playing, but it looks like one is required, then
  // start one playing. This is usually the symptom of there being no
  // transition score.
  if (TheAudio().is_open() && TheTheme().What_Is_Playing() == THEME_NONE) {
    TheTheme().Queue_Song(THEME_PICK_ANOTHER);
  }

  StartFrameTimer();

  // Update the display, unless we're inside a dialog.
  //
  // Skipped entirely during playback: the recording drives the view instead,
  // and Do_Record_Playback() renders below once it has restored the
  // position.
  if (!TheSession().Play && TheGameState().special_dialog() == SDLG_NONE &&
      TheGameState().in_focus()) {
    ProcessInput();
    TheMap().Render();
  }

  // Save map's position & selected objects, if we're recording the game.
  if (TheSession().Record || TheSession().Play) {
    Do_Record_Playback();
  }

  if constexpr (!config::kSortDrawEnabled) {
    // Sort the map's ground layer by y-coordinate value.  This is done
    // outside the IsToRedraw check, for the purposes of keeping the game in
    // sync between machines; this way, all machines will sort the Map's layer
    // in the same way, and any processing done that's based on the order of
    // this layer will remain in sync.
    DisplayClass::Layer.at(LAYER_GROUND).Sort();
  }

  // AI logic operations are performed here.
  TheWorld().logic().AI();
  TheWorld().time_quake() = false;
  if (!TheWorld().pending_time_quake()) {
    TheWorld().time_quake_center() = 0;
  }

  // Manage the inter-player message list.  If Manage() returns true, it
  // means a message has expired & been removed, and the entire map must be
  // updated.
  if (TheSession().Messages.Manage()) {
    TheScreen().hidden_page().Clear();
    TheMap().Flag_To_Redraw(true);
  }

  // Measure how long it took to process the AI
  //
  // Multiplayer uses this running average to pick a frame rate every machine
  // in the session can actually keep up with.
  TheSession().ProcessTicks += SystemTicks() - TheSession().ProcessTimer;
  TheSession().ProcessFrames++;

  // Process all commands that are ready to be processed.
  Queue_AI();

  // Keep track of elapsed time in the game.
  TheWorld().score().ElapsedTime += kTimerSecond / kTicksPerSecond;

  ServiceRealTime();

  if (FinishScenarioIfDecided()) {
    return !TheGameState().active();
  }

  TheGameClock().Advance();

  if (TheStartupOptions().quit_at_frame >= 0 && LogFrameAndQuitIfDue()) {
    return true;
  }

  // Is there a memory trasher altering the map??
  if (TheDebugState().check_map() && (!TheMap().Validate())) {
    if (WWMessageBox().Process(kLanguageText.map_error, kLanguageText.stop,
                               kLanguageText.continue_button) == 0) {
      TheGameState().active() = false;
    }
    TheMap().Validate();  // give debugger a chance to catch it
  }

  if (TheDebugState().motion_capture()) {
    CaptureMotionFrame();
  }

  WaitForNextFrame();
  return !TheGameState().active();
}
