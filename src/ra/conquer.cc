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
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/aircraft.h"
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
#include "ra/infantry.h"
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
#include "ra/target.h"
#include "ra/text_ids.h"
#include "ra/theme.h"
#include "ra/type.h"
#include "ra/unit.h"
#include "ra/vector_dynamic.h"
#include "ra/version.h"
#include "ra/vessel.h"
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

// The map editor's stand-in for Main_Loop(): render, take input, and keep the
// real-time callbacks alive so music continues. No game logic runs, so the
// scenario stays frozen while it is edited.
//
// Returns true when the game should end.
static bool Map_Edit_Loop() {
  // Redraw the map.
  Map.Render();

  // Get user input (keys, mouse clicks).
  KeyNumType input = KN_NONE;

  WWMouse->Erase_Mouse(&HidPage, true);

  int x = 0;
  int y = 0;
  Map.Input(input, x, y);

  // Process keypress.
  if (input) {
    Keyboard_Process(input);
  }

  Call_Back();  // maintains Theme.AI() for music
  Color_Cycle();

  return !GameActive;
}

// The game's entry point, after platform startup. Init_Game() does the
// one-time initialization; everything after it happens once per game played,
// because Select_Game() may hand back a wholly different kind of session --
// single player, network, modem, editor -- each needing its own setup and its
// own teardown.
//
// The network and modem layers are shut down after every game rather than left
// running, so that selecting one again restarts it from a known state.
void Main_Game(const int argc, char* argv[]) {
  static bool fade = true;

  // Perform one-time-only initializations
  if (!Init_Game(argc, argv)) {
    return;
  }

  // Game processing loop:
  // 1) Select which game to play, or whether to exit (don't fade the palette
  // on the first game selection, but fade it in on subsequent calls)
  // 2) Invoke either the main-loop routine, or the editor-loop routine,
  // until they indicate that the user wants to exit the scenario.
  while (Select_Game(fade)) {
    // Original author's note; the two assignments to fade around it cancel
    // out, so only the ScenarioInit reset has any effect.
    fade = false;
    ScenarioInit = 0;  // Kludge.

    fade = true;

    // Initialise the color lookup tables for the chronal vortex
    ChronalVortex.Stop();
    ChronalVortex.Setup_Remap_Tables(Scen.Theater);

    // Make the game screen visible, clear the keyboard buffer of spurious
    // values, and then show the mouse.  This PRESUMES that Select_Game() has
    // told the map to draw itself.
    GamePalette.Set(kFadePaletteMedium);
    Keyboard->Clear();
    // Only show the mouse if we're not playing back a recording.
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

    for (;;) {
      if constexpr (config::kScenarioEditorEnabled) {
        if (MapEditorActive) {
          // Scenario-editor-mode: call the editor's main loop
          if (Map_Edit_Loop()) {
            break;
          }
          continue;
        }
      }

      TimeQuake = PendingTimeQuake;
      PendingTimeQuake = false;
      // Call the game's main loop
      if (Main_Loop()) {
        break;
      }

      // If the SpecialDialog flag is set, invoke the given special
      // dialog. This must be done outside the main loop, since the
      // dialog will call Main_Loop(), allowing the game to run in the
      // background.
      if (SpecialDialog != SDLG_NONE) {
        switch (SpecialDialog) {
          case SDLG_SPECIAL:
            Map.Help_Text(TXT_NONE);
            Map.Override_Mouse_Shape(MOUSE_NORMAL, false);
            Special_Dialog();
            Map.Revert_Mouse_Shape();
            SpecialDialog = SDLG_NONE;
            break;

          case SDLG_OPTIONS:
            Map.Help_Text(TXT_NONE);
            Map.Override_Mouse_Shape(MOUSE_NORMAL, false);
            Options.Process();
            Map.Revert_Mouse_Shape();
            SpecialDialog = SDLG_NONE;
            break;

          case SDLG_SURRENDER:
            Map.Help_Text(TXT_NONE);
            Map.Override_Mouse_Shape(MOUSE_NORMAL, false);
            if (Surrender_Dialog(TXT_SURRENDER)) {
              if constexpr (config::kScenarioEditorEnabled) {
                PlayerPtr->Flag_To_Lose();
              } else {
                OutList.Add(EventClass(EventClass::DESTRUCT));
              }
            }
            SpecialDialog = SDLG_NONE;
            Map.Revert_Mouse_Shape();
            break;

          case SDLG_NONE:
          default:
            break;
        }
      }
    }

    // Send the game stats if we haven't already done so
    if (!GameStatisticsPacketSent && PacketLater) {
      Send_Statistics_Packet();  // After game sending if PacketLater set.
    }

    // Scenario is done; fade palette to black
    BlackPalette.Set(kFadePaletteSlow);
    VisiblePage.Clear();

    // Un-initialize whatever needs it, for each game played.
    //
    // Shut down either the modem or network; they'll get re-initialized if
    // the user selections those options again in Select_Game().  This
    // "re-boots" the modem & network code, which I currently feel is safer
    // than just letting it hang around.
    // (Skip this step if we're in playback mode; the modem or net won't have
    // been initialized in that case.)
    if (Session.Record || Session.Play) {
      Session.RecordFile.Close();
    }

    if (Session.Type == GAME_NULL_MODEM || Session.Type == GAME_MODEM) {
      if (!Session.Play) {
        Modem_Signoff();
      }
    } else {
      if ((Session.Type == GAME_IPX) && (!Session.Play)) {
        Shutdown_Network();
      }
    }

    // If we're playing back, the mouse will be hidden; show it.
    // Also, set all variables back to normal, to return to the main menu.
    if (Session.Play) {
      Show_Mouse();
      Session.Type = GAME_NORMAL;
      Session.Play = false;
    }
  }

  // Free the scenario description buffers
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

  // Wolapi maintenance.
  if constexpr (config::kWolapiEnabled) {
    if (pWolapi) {
      if (pWolapi->bInGame) {
        if (!pWolapi->bConnectionDown &&
            Get_Time_Ms() > pWolapi->dwTimeNextWolapiPump) {
          Pump_Wolapi_Messages();
          pWolapi->dwTimeNextWolapiPump = Get_Time_Ms() + WOLAPIPUMPWAIT +
                                          700;  // Slower pump during games.
          if (pWolapi->bConnectionDown) {
            // Connection to server lost.
            Session.Messages.Add_Message(
                nullptr, 0, TXT_WOL_WOLAPIGONE, PCOLOR_GOLD,
                TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
                Rule.MessageDelay * kTicksPerMinute);
            Sound_Effect(WOLSOUND_LOGOUT);
            // ajw (Wolapi object is now left around, so we can try to send
            // game results.)
            //  // Kill wolapi.
            //  pWolapi->UnsetupCOMStuff();
            //  delete pWolapi;
            //  pWolapi = nullptr;
          }
        }
      } else {
        // When showing a modal dialog during chat, this pumping is turned
        // on. It's turned off immediately following.
        if (pWolapi->bPump_In_Call_Back &&
            Get_Time_Ms() > pWolapi->dwTimeNextWolapiPump) {
          Pump_Wolapi_Messages();
          pWolapi->dwTimeNextWolapiPump = Get_Time_Ms() + WOLAPIPUMPWAIT;
        }
      }
    }
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
  // Accumulate the number of 'spare' ticks that are frittered away here.
  SpareTicks += FrameTimer.Value();

  // Delay until the frame timer expires. This forces the game loop to be
  // regulated to a speed controlled by the game options slider.
  while (FrameTimer.HasTimeLeft()) {
    Color_Cycle();
    Call_Back();

    if (SpecialDialog == SDLG_NONE) {
      WWMouse->Erase_Mouse(&HidPage, true);
      KeyNumType input = KN_NONE;
      int x = 0;
      int y = 0;
      Map.Input(input, x, y);
      if (input) {
        Keyboard_Process(input);
      }
      Map.Render();
    }
  }
  Color_Cycle();
  Call_Back();
}

// Runs one frame of the game. See the declaration in conquer.h.
//
// Nothing that affects game state may be skipped here on the grounds that it is
// only visual -- every machine in a multiplayer game runs this same sequence
// and must arrive at the same state, or the session desyncs.
bool Main_Loop() {
  Mono_Set_Cursor(0, 0);

  if (!GameActive) {
    return !GameActive;
  }

  // Call the focus loss handler
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

  // Setup the timer so that the Main_Loop function processes at the correct
  // rate.
  if (Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH &&
      Session.CommProtocol == COMM_PROTOCOL_MULTI_E_COMP) {
    // In playback mode, run as fast as possible.
    if (Session.Play) {
      FrameTimer.Set(0);
    } else {
      if (!Session.DesiredFrameRate) {
        Session.DesiredFrameRate =
            60;  // A division by zero was happening (very rare).
      }
      const int frame_delay = kTimerSecond / Session.DesiredFrameRate;
      FrameTimer.Set(frame_delay);
    }
  } else {
    if (Options.GameSpeed != 0) {
      FrameTimer.Set(Options.GameSpeed +
                     (PlayerPtr->Difficulty == DIFF_EASY ? 1 : 0) -
                     (PlayerPtr->Difficulty == DIFF_HARD ? 1 : 0));
    } else {
      FrameTimer.Set(Options.GameSpeed +
                     (PlayerPtr->Difficulty == DIFF_EASY ? 1 : 0));
    }
  }

  // Update the display, unless we're inside a dialog.
  //
  // Skipped entirely during playback: the recording drives the view instead,
  // and Do_Record_Playback() renders below once it has restored the
  // position.
  if ((!Session.Play) && (SpecialDialog == SDLG_NONE && GameInFocus)) {
    WWMouse->Erase_Mouse(&HidPage, true);
    KeyNumType input = KN_NONE;
    int x = 0;
    int y = 0;
    Map.Input(input, x, y);
    if (input != KN_NONE) {
      Keyboard_Process(input);
    }
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

  // Check for player wins or loses according to global event flag.
  if (PlayerWins) {
    // Send the game statistics to the game-results server.
    if (Session.Type == GAME_INTERNET && !GameStatisticsPacketSent) {
      Register_Game_End_Time();
      Send_Statistics_Packet();  // Player just won.
    }

    WWMouse->Erase_Mouse(&HidPage, true);
    PlayerLoses = false;
    PlayerWins = false;
    PlayerRestarts = false;
    Map.Help_Text(TXT_NONE);
    Do_Win();
    return !GameActive;
  }
  if (PlayerLoses) {
    // Send the game statistics to the game-results server.
    if (Session.Type == GAME_INTERNET && !GameStatisticsPacketSent) {
      Register_Game_End_Time();
      Send_Statistics_Packet();  // Player just lost.
    }

    WWMouse->Erase_Mouse(&HidPage, true);
    PlayerWins = false;
    PlayerLoses = false;
    PlayerRestarts = false;
    Map.Help_Text(TXT_NONE);
    Do_Lose();
    return !GameActive;
  }
  if (PlayerRestarts) {
    WWMouse->Erase_Mouse(&HidPage, true);
    PlayerWins = false;
    PlayerLoses = false;
    PlayerRestarts = false;
    Map.Help_Text(TXT_NONE);
    Do_Restart();
    return !GameActive;
  }

  if (Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH &&
      Session.Players.Count() == 2 && Scen.bLocalProposesDraw &&
      Scen.bOtherProposesDraw) {
    // End game in a draw.
    if (Session.Type == GAME_INTERNET && !GameStatisticsPacketSent) {
      Register_Game_End_Time();
      Send_Statistics_Packet();
    }
    WWMouse->Erase_Mouse(&HidPage, true);
    Map.Help_Text(TXT_NONE);
    Do_Draw();
    return !GameActive;
  }

  // The frame logic has been completed. Increment the frame
  // counter.
  Frame++;

  // -QUITFRAME<n>: log where every mobile object is each frame, then end
  // the game at frame n, saving to -SAVESLOT<n> first if one was given.
  // Together with -LOADGAME this checks that loaded objects keep moving
  // without anyone at the keyboard.
  if (DebugQuitAtFrame >= 0) {
    for (int index = 0; index < Units.Count(); index++) {
      const UnitClass* unit = Units.Ptr(index);
      LOG(INFO) << "frame " << Frame << " unit " << unit->Class->IniName
                << " coord " << absl::StrFormat("%08x", unit->Coord)
                << " mission " << magic_enum::enum_name(unit->Mission)
                << " navcom " << absl::StrFormat("%08x", unit->NavCom);
    }
    for (int index = 0; index < Infantry.Count(); index++) {
      const InfantryClass* inf = Infantry.Ptr(index);
      LOG(INFO) << "frame " << Frame << " infantry " << inf->Class->IniName
                << " coord " << absl::StrFormat("%08x", inf->Coord)
                << " mission " << magic_enum::enum_name(inf->Mission)
                << " navcom " << absl::StrFormat("%08x", inf->NavCom);
    }
    for (int index = 0; index < Vessels.Count(); index++) {
      const VesselClass* vessel = Vessels.Ptr(index);
      LOG(INFO) << "frame " << Frame << " vessel " << vessel->Class->IniName
                << " coord " << absl::StrFormat("%08x", vessel->Coord)
                << " mission " << magic_enum::enum_name(vessel->Mission)
                << " navcom " << absl::StrFormat("%08x", vessel->NavCom);
    }
    for (int index = 0; index < Aircraft.Count(); index++) {
      const AircraftClass* air = Aircraft.Ptr(index);
      LOG(INFO) << "frame " << Frame << " aircraft " << air->Class->IniName
                << " coord " << absl::StrFormat("%08x", air->Coord)
                << " mission " << magic_enum::enum_name(air->Mission);
    }
    if (Frame >= DebugQuitAtFrame) {
      if (DebugSaveSlot >= 0) {
        Save_Game(DebugSaveSlot, "debug");
      }
      GameActive = false;
      return true;
    }
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
    } else {
      Debug_MotionCapture = false;

      DiskFile file;
      char filename[30];

      for (base::ssize index = 0; index < sequence; index++) {
        base::CopyBytes(
            std::as_writable_bytes(temp_page.Get_Bytes()),
            std::as_bytes(std::span(frames.at(base::ToSize(index)))), size);
        absl::SNPrintF(filename, sizeof(filename), "cap%04zd.pcx", index);
        file.SetName(filename);

        Write_PCX_File(file, temp_page, &GamePalette);
      }

      // Release the run's buffers so that the next run re-reads MovieTime.
      frames.clear();
      frames.shrink_to_fit();
      sequence = 0;
    }
  }

  BEnd(BENCH_GAME_FRAME);

  Sync_Delay();
  return !GameActive;
}
