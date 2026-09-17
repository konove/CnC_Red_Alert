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
// Main_Game() owns the outer loop -- pick a game, play it, tear it down -- and
// Main_Loop() runs one frame of it, pacing itself with Sync_Delay().
// Call_Back() is the real-time servicing (sound, music, network) that also runs
// inside every blocking loop and dialog.
//
// Originally CONQUER.CPP, by Joe L. Bostic, started April 3, 1991.

#include "ra/conquer.h"

#include <absl/log/check.h>

#include <array>
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
#include "ra/bench_util.h"
#include "ra/ccptr.h"
#include "ra/chat.h"
#include "ra/config.h"
#include "ra/debug.h"
#include "ra/defines.h"
#include "ra/display.h"
#include "ra/event.h"
#include "ra/externs.h"
#include "ra/filepcx.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/heap.h"
#include "ra/hotkeys.h"
#include "ra/house.h"
#include "ra/infantry.h"  // IWYU pragma: keep
#include "ra/init.h"
#include "ra/internet.h"
#include "ra/interpal.h"
#include "ra/jshell.h"
#include "ra/language.h"
#include "ra/logic.h"
#include "ra/mapedit.h"
#include "ra/monoc.h"
#include "ra/mplayer.h"
#include "ra/msgbox.h"
#include "ra/msglist.h"
#include "ra/netdlg.h"
#include "ra/nulldlg.h"
#include "ra/nullmgr.h"
#include "ra/object.h"
#include "ra/palette.h"
#include "ra/queue.h"
#include "ra/rawolapi.h"
#include "ra/record_playback.h"
#include "ra/rules.h"
#include "ra/saveload.h"
#include "ra/scenario.h"
#include "ra/score.h"
#include "ra/session.h"
#include "ra/special.h"
#include "ra/text_ids.h"
#include "ra/theme.h"
#include "ra/unit.h"  // IWYU pragma: keep
#include "ra/vector_dynamic.h"
#include "ra/version.h"
#include "ra/vessel.h"  // IWYU pragma: keep
#include "ra/vortex.h"
#include "ra/wolapiob.h"
#include "ra/wolstrng.h"
#include "ra/ww_audio.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/timer.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "sdllib/wwstd.h"
#include "tech/disk_file.h"
#include "tech/fixed.h"
#include "tech/ftimer.h"
#include "tech/game_file.h"
#include "tech/rgb.h"

// Cycles the animated palette entries. Two effects run off independent timers:
// a white that pulses between full and dark, used by the radar box and other
// interface glows, and a rotation of the water colours.
//
// This needs to run at least 8 times a second to look smooth, which is why
// Sync_Delay() calls it while idling rather than the main loop calling it once
// per frame.
static void Color_Cycle() {
  static Timer<SystemTickSource> _timer;
  static Timer<SystemTickSource> _ftimer;
  static bool _up = false;
  static int val = 255;

  if (Options.IsPaletteScroll) {
    bool changed = false;
    // Process the fading white color. It is used for the radar box and other
    // glowing game interface elements.
    if (_ftimer.IsFinished()) {
      _ftimer.Set(kTimerSecond / 6);

      // Six steps of 20 carry the pulse across its 0x20..150 range, so a full
      // cycle takes about two seconds at the timer rate set above. The range
      // stops short of both black and full white: the glow has to stay legible
      // at its dimmest and stay distinct from plain white at its brightest.
      constexpr int kStepRate = 20;
      if (_up) {
        val += kStepRate;
        if (val > 150) {
          val = 150;
          _up = false;
        }
      } else {
        val -= kStepRate;
        if (val < 0x20) {
          val = 0x20;
          _up = true;
        }
      }

      // Set the pulse color as the proportional value between white and
      // the minimum value for pulsing.
      GamePalette.at(kPulseColor) = GamePalette.at(kWhite);
      GamePalette.at(kPulseColor).Adjust(val, kBlackColor);

      // Pulse the glowing embers between medium and dark red.
      GamePalette.at(kEmberColor) = RGBClass(255, 80, 80);
      GamePalette.at(kEmberColor).Adjust(val, kBlackColor);

      changed = true;
    }

    // Process the color cycling effects -- water.
    if (_timer.IsFinished()) {
      _timer.Set(kTimerSecond / 4);

      const RGBClass first =
          GamePalette.at(kCycleColorStart + kCycleColorCount - 1);
      for (int index = kCycleColorStart + kCycleColorCount - 1;
           index >= kCycleColorStart; index--) {
        GamePalette.at(index) = GamePalette.at(index - 1);
      }
      GamePalette.at(kCycleColorStart) = first;

      changed = true;
    }

    // If any of the processing functions changed the palette, then this
    // palette must be passed to the system.
    if (changed) {
      BStart(BENCH_PALETTE);
      GamePalette.Set();
      BEnd(BENCH_PALETTE);
    }
  }
}

// Reads one input event from the map and dispatches any keypress. The mouse is
// erased from the hidden page first so the next render draws it afresh.
static void Process_Input() {
  WWMouse->Erase_Mouse(&HidPage, true);
  KeyNumType input = KN_NONE;
  int x = 0;
  int y = 0;
  Map.Input(input, x, y);
  if (input != KN_NONE) {
    Keyboard_Process(input);
  }
}

// The map editor's stand-in for Main_Loop(): render, take input, and keep the
// real-time callbacks alive so music continues. No game logic runs, so the
// scenario stays frozen while it is edited.
//
// Returns true when the game should end.
static bool Map_Edit_Loop() {
  Map.Render();
  Process_Input();

  Call_Back();  // maintains Theme.AI() for music
  Color_Cycle();

  return !GameActive;
}

// Runs the dialog SpecialDialog asks for, then clears the request. The dialogs
// call Main_Loop() themselves so the game keeps running behind them, which is
// why this is invoked between frames rather than from inside one.
static void Run_Special_Dialog() {
  const SpecialDialogType dialog = SpecialDialog;
  if (dialog == SDLG_NONE) {
    return;
  }

  Map.Help_Text(TXT_NONE);
  Map.Override_Mouse_Shape(MOUSE_NORMAL, false);
  switch (dialog) {
    case SDLG_SPECIAL:
      Special_Dialog();
      break;

    case SDLG_OPTIONS:
      Options.Process();
      break;

    case SDLG_SURRENDER:
      if (Surrender_Dialog(TXT_SURRENDER)) {
        if constexpr (config::kScenarioEditorEnabled) {
          PlayerPtr->Flag_To_Lose();
        } else {
          OutList.Add(EventClass(EventClass::DESTRUCT));
        }
      }
      break;

    case SDLG_NONE:
    default:
      break;
  }
  SpecialDialog = SDLG_NONE;
  Map.Revert_Mouse_Shape();
}

// Per-scenario setup that Select_Game() leaves to the caller: vortex remap
// tables, the palette, and the mouse and statistics state for the session type.
static void Begin_Scenario() {
  ScenarioInit = 0;

  ChronalVortex.Stop();
  ChronalVortex.Setup_Remap_Tables(Scen.Theater);

  // This PRESUMES that Select_Game() has told the map to draw itself.
  GamePalette.Set(kFadePaletteMedium);
  Keyboard->Clear();

  // A recording drives the view on playback, so there is no mouse to show.
  if (Session.Play) {
    Hide_Mouse();
    ResetRecordedEvents();
  } else {
    Show_Mouse();
  }

  if (Session.Type == GAME_INTERNET) {
    Register_Game_Start_Time();
    GameStatisticsPacketSent = false;
    PacketLater = nullptr;
    ConnectionLost = false;
  }
}

// Runs frames, and any dialogs they request, until the scenario ends.
static void Run_Scenario() {
  for (;;) {
    if constexpr (config::kScenarioEditorEnabled) {
      if (MapEditorActive) {
        if (Map_Edit_Loop()) {
          return;
        }
        continue;
      }
    }

    TimeQuake = PendingTimeQuake;
    PendingTimeQuake = false;
    if (Main_Loop()) {
      return;
    }

    Run_Special_Dialog();
  }
}

// Tears down what the finished scenario leaves behind.
//
// The modem and network are shut down rather than left running, so that
// selecting them again in Select_Game() restarts them from a known state.
// Playback never initialized either, so it skips this.
static void End_Scenario() {
  if (!GameStatisticsPacketSent && PacketLater) {
    Send_Statistics_Packet();  // After game sending if PacketLater set.
  }

  BlackPalette.Set(kFadePaletteSlow);
  VisiblePage.Clear();

  if (Session.Record || Session.Play) {
    Session.RecordFile.Close();
  }

  if (!Session.Play) {
    if (Session.Type == GAME_NULL_MODEM || Session.Type == GAME_MODEM) {
      Modem_Signoff();
    } else if (Session.Type == GAME_IPX) {
      Shutdown_Network();
    }
  }

  // Return from playback to the main menu with the mouse visible again.
  if (Session.Play) {
    Show_Mouse();
    Session.Type = GAME_NORMAL;
    Session.Play = false;
  }
}

// The game's entry point, after platform startup. Init_Game() does the
// one-time initialization; everything after it happens once per game played,
// because Select_Game() may hand back a wholly different kind of session --
// single player, network, modem, editor -- each needing its own setup and its
// own teardown.
void Main_Game(const int argc, char* argv[]) {
  if (!Init_Game(argc, argv)) {
    return;
  }

  while (Select_Game(true)) {
    Begin_Scenario();
    Run_Scenario();
    End_Scenario();
  }

  Session.Free_Scenario_Descriptions();
}

// Gives the Westwood Online chat and matchmaking objects a slice of time
// to deliver whatever their servers have queued.
//
// Both PumpMessages HRESULT's are dropped on purpose: a lost connection is
// reported through the callbacks they fire, which set
// pWolapi->bConnectionDown (rawolapi.cc), and every caller tests that flag
// instead. Only reachable when config::kWolapiEnabled, hence maybe_unused.
[[maybe_unused]] static void Pump_Wolapi_Messages() {
  static_cast<void>(pWolapi->pChat->PumpMessages());
  static_cast<void>(pWolapi->pNetUtil->PumpMessages());
}

// Keeps the Westwood Online connection serviced. In a game it pumps more
// slowly and announces a dropped connection; outside one it pumps only while a
// modal dialog over the chat screen has asked for it.
[[maybe_unused]] static void Wolapi_Call_Back() {
  if (!pWolapi || Get_Time_Ms() <= pWolapi->dwTimeNextWolapiPump) {
    return;
  }

  if (!pWolapi->bInGame) {
    if (pWolapi->bPump_In_Call_Back) {
      Pump_Wolapi_Messages();
      pWolapi->dwTimeNextWolapiPump = Get_Time_Ms() + WOLAPIPUMPWAIT;
    }
    return;
  }

  if (pWolapi->bConnectionDown) {
    return;
  }
  Pump_Wolapi_Messages();
  pWolapi->dwTimeNextWolapiPump = Get_Time_Ms() + WOLAPIPUMPWAIT + 700;
  if (pWolapi->bConnectionDown) {
    // The Wolapi object is kept rather than deleted, so that the game results
    // can still be sent.
    Session.Messages.Add_Message(
        nullptr, 0, TXT_WOL_WOLAPIGONE, PCOLOR_GOLD,
        TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
        Rule.MessageDelay * kTicksPerMinute);
    Sound_Effect(WOLSOUND_LOGOUT);
  }
}

void Call_Back() {
  // Music and speech maintenance
  if (SampleType != SAMPLE_NONE) {
    Sound_Callback();
    Theme.AI();
    Speak_AI();
  }

  // Network maintenance.
  if (Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) {
    IPX_Call_Back();
  }

  // Serial game maintenance.
  if (Session.Type == GAME_NULL_MODEM ||
      (Session.Type == GAME_MODEM && Session.ModemService)) {
    NullModem.Service();
  }

  if constexpr (config::kWolapiEnabled) {
    Wolapi_Call_Back();
  }

  Video_End_Frame();
}

// Spins until the frame timer expires, holding the game to the rate set by the
// game-speed option.
//
// The wait is not idle: input, rendering and the real-time callbacks all run
// here. That keeps the interface responsive and the palette cycling smooth
// between logic frames, which tick far more slowly than the display does.
// Ticks spent waiting are accumulated into SpareTicks as a measure of how much
// headroom the machine has.
static void Sync_Delay() {
  SpareTicks += FrameTimer.Value();

  while (FrameTimer.HasTimeLeft()) {
    Color_Cycle();
    Call_Back();

    if (SpecialDialog == SDLG_NONE) {
      Process_Input();
      Map.Render();
    }
  }
  Color_Cycle();
  Call_Back();
}

// Sets the frame timer that Sync_Delay() waits out at the end of the frame.
//
// Multiplayer sessions run at the rate the machines negotiated, and playback as
// fast as possible. Otherwise the delay comes from the game-speed option, a
// tick slower on easy and a tick faster on hard.
static void Set_Frame_Timer() {
  if (Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH &&
      Session.CommProtocol == COMM_PROTOCOL_MULTI_E_COMP) {
    if (Session.Play) {
      FrameTimer.Set(0);
      return;
    }
    // A zero rate was seen, rarely, and divided by zero.
    if (Session.DesiredFrameRate == 0) {
      Session.DesiredFrameRate = 60;
    }
    FrameTimer.Set(kTimerSecond / Session.DesiredFrameRate);
    return;
  }

  int delay = static_cast<int>(Options.GameSpeed);
  if (PlayerPtr->Difficulty == DIFF_EASY) {
    delay++;
  } else if (PlayerPtr->Difficulty == DIFF_HARD && delay > 0) {
    delay--;
  }
  FrameTimer.Set(delay);
}

// How the scenario ended this frame, in the order the checks take priority.
enum class ScenarioOutcome { kNone, kWin, kLose, kRestart, kDraw };

// Reads the outcome flags that game logic raised during the frame. A draw needs
// both players of a two-player multiplayer game to have proposed it.
static ScenarioOutcome Pending_Outcome() {
  if (PlayerWins) {
    return ScenarioOutcome::kWin;
  }
  if (PlayerLoses) {
    return ScenarioOutcome::kLose;
  }
  if (PlayerRestarts) {
    return ScenarioOutcome::kRestart;
  }
  if (Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH &&
      Session.Players.Count() == 2 && Scen.bLocalProposesDraw &&
      Scen.bOtherProposesDraw) {
    return ScenarioOutcome::kDraw;
  }
  return ScenarioOutcome::kNone;
}

// Ends the scenario if this frame decided it: reports the result to the
// game-results server, clears the outcome flags and runs the matching end
// sequence. Returns true if the scenario ended.
static bool Finish_Scenario_If_Decided() {
  const ScenarioOutcome outcome = Pending_Outcome();
  if (outcome == ScenarioOutcome::kNone) {
    return false;
  }

  if (outcome != ScenarioOutcome::kRestart && Session.Type == GAME_INTERNET &&
      !GameStatisticsPacketSent) {
    Register_Game_End_Time();
    Send_Statistics_Packet();
  }

  WWMouse->Erase_Mouse(&HidPage, true);
  PlayerWins = false;
  PlayerLoses = false;
  PlayerRestarts = false;
  Map.Help_Text(TXT_NONE);

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
static void Log_Positions(const TFixedIHeapClass<T>& objects,
                          const std::string_view kind) {
  for (int index = 0; index < objects.Count(); index++) {
    const T* object = objects.Ptr(index);
    LOG(INFO) << "frame " << Frame << " " << kind << " "
              << object->Class->IniName << " coord "
              << absl::StrFormat("%08x", object->Coord) << " mission "
              << magic_enum::enum_name(object->Mission) << " navcom "
              << absl::StrFormat("%08x", object->NavCom);
  }
}

// -QUITFRAME<n>: logs where every mobile object is each frame, then ends the
// game at frame n, saving to -SAVESLOT<n> first if one was given. Together with
// -LOADGAME this checks that loaded objects keep moving without anyone at the
// keyboard. Returns true once the game has been ended.
static bool Debug_Quit_Frame() {
  Log_Positions(Units, "unit");
  Log_Positions(Infantry, "infantry");
  Log_Positions(Vessels, "vessel");
  Log_Positions(Aircraft, "aircraft");

  if (Frame < DebugQuitAtFrame) {
    return false;
  }
  if (DebugSaveSlot >= 0) {
    Save_Game(DebugSaveSlot, "debug");
  }
  GameActive = false;
  return true;
}

// Records the visible screen each frame for Rule.MovieTime minutes, then
// writes the frames out as cap0000.pcx, cap0001.pcx, ... and stops.
static void Capture_Motion_Frame() {
  // One captured screen per element. Empty between runs. Deliberately leaked
  // rather than given static storage duration with a destructor, which would
  // run at exit after the graphics system is already gone.
  // LLVM 23 treats resize as invalidating the vector itself. The reference
  // remains valid, and element views are acquired only after resizing.
  // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
  static auto& frames = *new std::vector<std::vector<char>>();
  // Doubles as the frame counter and the end-of-run signal: reaching
  // frames.size() ends the capture and flushes to disk.
  static base::ssize sequence = 0;

  if (frames.empty()) {
    // Sized when a capture run starts rather than once per process, so that
    // an edit to MovieTime takes effect on the next run.
    const int frame_count = Rule.MovieTime * kTicksPerMinute;
    frames.resize(base::ToSize(frame_count));
  }

  // Leaked for the same reason as frames above.
  static auto& temp_page =
      *new GraphicBufferClass(SeenBuff.Get_Width(), SeenBuff.Get_Height(), {},
                              SeenBuff.Get_Width() * SeenBuff.Get_Height());

  const base::ssize size =
      static_cast<base::ssize>(SeenBuff.Get_Width()) * SeenBuff.Get_Height();

  if (sequence < std::ssize(frames)) {
    // A no-op on a frame reused from an earlier run of the same resolution.
    frames.at(base::ToSize(sequence)).resize(base::ToSize(size));

    SeenBuff.Blit(temp_page);
    base::CopyBytes(
        std::as_writable_bytes(std::span(frames.at(base::ToSize(sequence)))),
        std::as_bytes(temp_page.Get_Bytes()), size);
    sequence++;
    return;
  }

  Debug_MotionCapture = false;

  DiskFile file;
  char filename[30];
  for (base::ssize index = 0; index < sequence; index++) {
    base::CopyBytes(std::as_writable_bytes(temp_page.Get_Bytes()),
                    std::as_bytes(std::span(frames.at(base::ToSize(index)))),
                    size);
    absl::SNPrintF(filename, sizeof(filename), "cap%04zd.pcx", index);
    file.SetName(filename);

    Write_PCX_File(file, temp_page, &GamePalette);
  }

  // Release the run's buffers so that the next run re-reads MovieTime.
  frames.clear();
  frames.shrink_to_fit();
  sequence = 0;
}

// Runs one frame of the game. See the declaration in conquer.h.
//
// Nothing that affects game state may be skipped here on the grounds that it is
// only visual -- every machine in a multiplayer game runs this same sequence
// and must arrive at the same state, or the session desyncs.
bool Main_Loop() {
  Mono_Set_Cursor(0, 0);

  if (!GameActive) {
    return true;
  }

  Check_For_Focus_Loss();

  // Sync-bug trapping code
  if (Frame >= Session.TrapFrame) {
    Session.Trap_Object();
  }

  // Initialize our AI processing timer
  Session.ProcessTimer = TickCount.Value();

  if (Session.TrapCheckHeap) {
    Debug_Trap_Check_Heap = true;
  }

  if constexpr (config::kCheatKeysEnabled) {
    // Update the running status debug display.
    Self_Regulate();
  }

  BStart(BENCH_GAME_FRAME);

  // If there is no theme playing, but it looks like one is required, then
  // start one playing. This is usually the symptom of there being no
  // transition score.
  if (SampleType != SAMPLE_NONE && Theme.What_Is_Playing() == THEME_NONE) {
    Theme.Queue_Song(THEME_PICK_ANOTHER);
  }

  Set_Frame_Timer();

  // Update the display, unless we're inside a dialog.
  //
  // Skipped entirely during playback: the recording drives the view instead,
  // and Do_Record_Playback() renders below once it has restored the
  // position.
  if (!Session.Play && SpecialDialog == SDLG_NONE && GameInFocus) {
    Process_Input();
    Map.Render();
  }

  // Save map's position & selected objects, if we're recording the game.
  if (Session.Record || Session.Play) {
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
  Logic.AI();
  TimeQuake = false;
  if (!PendingTimeQuake) {
    TimeQuakeCenter = 0;
  }

  // Manage the inter-player message list.  If Manage() returns true, it
  // means a message has expired & been removed, and the entire map must be
  // updated.
  if (Session.Messages.Manage()) {
    HiddenPage.Clear();
    Map.Flag_To_Redraw(true);
  }

  // Measure how long it took to process the AI
  //
  // Multiplayer uses this running average to pick a frame rate every machine
  // in the session can actually keep up with.
  Session.ProcessTicks += TickCount.Value() - Session.ProcessTimer;
  Session.ProcessFrames++;

  // Process all commands that are ready to be processed.
  Queue_AI();

  // Keep track of elapsed time in the game.
  Score.ElapsedTime += kTimerSecond / kTicksPerSecond;

  Call_Back();

  if (Finish_Scenario_If_Decided()) {
    return !GameActive;
  }

  Frame++;

  if (DebugQuitAtFrame >= 0 && Debug_Quit_Frame()) {
    return true;
  }

  // Is there a memory trasher altering the map??
  if (Debug_Check_Map && (!Map.Validate())) {
    if (WWMessageBox().Process(kLanguageText.map_error, kLanguageText.stop,
                               kLanguageText.continue_button) == 0) {
      GameActive = false;
    }
    Map.Validate();  // give debugger a chance to catch it
  }

  if (Debug_MotionCapture) {
    Capture_Motion_Frame();
  }

  BEnd(BENCH_GAME_FRAME);

  Sync_Delay();
  return !GameActive;
}
