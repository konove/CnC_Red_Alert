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

// File: The main game loop, plus the game-wide services that grew up around it.
//
// This is the hub the rest of Red Alert hangs off. Main_Game() owns the
// outer loop -- pick a game, play it, tear it down -- and Main_Loop() runs one
// frame of it. Around those sit the odds and ends that never found a better
// home: keyboard dispatch, inter-player chat, palette cycling, VQA movie
// playback, the name-to-enum lookups the INI parser needs, and CD swapping.
//
// Originally CONQUER.CPP, by Joe L. Bostic, started April 3, 1991.

#include "ra/conquer.h"

#include <fcntl.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "base/types.h"
#include "port/ex_string.h"
#include "port/platform.h"
#include "port/safe_string.h"
#include "port/win32/win32_registry.h"
#include "ra/aircraft.h"
#include "ra/bench_util.h"
#include "ra/building.h"
#include "ra/ccfile.h"
#include "ra/ccptr.h"
#include "ra/config.h"
#include "ra/const.h"
#include "ra/coord.h"
#include "ra/debug.h"
#include "ra/defines.h"
#include "ra/display.h"
#include "ra/event.h"
#include "ra/externs.h"
#include "ra/face.h"
#include "ra/filepcx.h"
#include "ra/foot.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/infantry.h"
#include "ra/init.h"
#include "ra/inline.h"
#include "ra/interpal.h"
#include "ra/ipxaddr.h"
#include "ra/ipxgconn.h"
#include "ra/ipxmgr.h"
#include "ra/jshell.h"
#include "ra/keyframe.h"
#include "ra/language.h"
#include "ra/logic.h"
#include "ra/mapedit.h"
#include "ra/mission_id.h"
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
#include "ra/rules.h"
#include "ra/scenario.h"
#include "ra/score.h"
#include "ra/session.h"
#include "ra/special.h"
#include "ra/target.h"
#include "ra/theme.h"
#include "ra/type.h"
#include "ra/unit.h"
#include "ra/vector.h"
#include "ra/vector_dynamic.h"
#include "ra/version.h"
#include "ra/vessel.h"
#include "ra/vortex.h"
#include "ra/wolapiob.h"
#include "ra/wolstrng.h"
#include "ra/ww_audio.h"
#include "sdllib/bitmap.h"
#include "sdllib/drawbuff.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/playcd.h"
#include "sdllib/shape.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "sdllib/wwstd.h"
#include "tech/2keyfbuf.h"
#include "tech/cdfile.h"
#include "tech/fixed.h"
#include "tech/ftimer.h"
#include "tech/rect.h"
#include "tech/rgb.h"
#include "winvq/vqa32/vqaplay.h"

// The key that answers a page from a Westwood Online user outside the
// game.
constexpr KeyNumType kPageRespondKey = KN_RETURN;  // KN_COMMA

// Recording state for the current frame, consumed and cleared by
// Do_Record_Playback(). File-local: nothing outside this module touches them.
static char TeamEvent = 0;       // 0 = no event, 1,2,3 = team event type
static char TeamNumber = 0;      // which team was selected? (1-9)
static char FormationEvent = 0;  // 0 = no event, 1 = formation was toggled

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
      GamePalette[kPulseColor] = GamePalette[WHITE];
      GamePalette[kPulseColor].Adjust(val, kBlackColor);

      // Pulse the glowing embers between medium and dark red.
      GamePalette[kEmberColor] = RGBClass(255, 80, 80);
      GamePalette[kEmberColor].Adjust(val, kBlackColor);

      changed = true;
    }

    // Process the color cycling effects -- water.
    if (_timer.IsFinished()) {
      _timer.Set(kTimerSecond / 4);

      RGBClass first = GamePalette[kCycleColorStart + kCycleColorCount - 1];
      for (int index = kCycleColorStart + kCycleColorCount - 1;
           index >= kCycleColorStart; index--) {
        GamePalette[index] = GamePalette[index - 1];
      }
      GamePalette[kCycleColorStart] = first;

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
  KeyNumType input;

  WWMouse->Erase_Mouse(&HidPage, true);

  int x;
  int y;
  Map.Input(input, x, y);

  // Process keypress.
  if (input) {
    Keyboard_Process(input);
  }

  Call_Back();  // maintains Theme.AI() for music
  Color_Cycle();

  return !GameActive;
}

// Puts the player's currently selected group into formation, or takes it out of
// one if it is already in formation.
//
// A formation is stored per unit as an offset from the group's centre, so that
// the group keeps its shape as it moves. kNoFormationOffset is the "not in
// formation" sentinel; finding it on the first member is what decides whether
// this call sets the formation up or tears it down.
static void Toggle_Formation() {
  // kNoGroup means "no grouped unit found yet". It must be compared against
  // rather than -1: Group is an unsigned char, so an ungrouped unit reads back
  // as 255 and would otherwise be taken for a valid group number and used to
  // index the ten-entry TeamSpeed/TeamMaxSpeed arrays.
  int team = kNoGroup;
  // Seeded inverted -- min at the largest possible value, max at the smallest
  // -- so the first cell examined replaces both.
  long minx = 0x7FFFFFFFL, miny = 0x7FFFFFFFL;
  long maxx = 0, maxy = 0;
  int index;
  bool set_form = false;

  // Recording support
  if (Session.Record) {
    FormationEvent = 1;
  }

  // Find the first selected object that is a member of a team, and
  // register his group as the team we're using.  Once we find the team
  // number, update the 'set_form' flag to know whether we should be setting
  // the formation's offsets, or clearing them.  If they currently have
  // illegal offsets (kNoFormationOffset), then we're setting.
  //
  // The three passes are ordered units, infantry, vessels because a mixed
  // group takes its speed from whichever type is found first.
  for (index = 0; index < Units.Count(); index++) {
    UnitClass* obj = Units.Ptr(index);
    if (obj && !obj->IsInLimbo && obj->House == PlayerPtr && obj->IsSelected) {
      team = obj->Group;
      if (team != kNoGroup) {
        set_form = obj->XFormOffset == kNoFormationOffset;
        TeamSpeed[team] = SPEED_WHEEL;
        TeamMaxSpeed[team] = MPH_LIGHT_SPEED;
        break;
      }
    }
  }
  if (team == kNoGroup) {
    for (index = 0; index < Infantry.Count(); index++) {
      InfantryClass* obj = Infantry.Ptr(index);
      if (obj && !obj->IsInLimbo && obj->House == PlayerPtr &&
          obj->IsSelected) {
        team = obj->Group;
        if (team != kNoGroup) {
          set_form = obj->XFormOffset == kNoFormationOffset;
          TeamSpeed[team] = SPEED_WHEEL;
          TeamMaxSpeed[team] = MPH_LIGHT_SPEED;
          break;
        }
      }
    }
  }

  if (team == kNoGroup) {
    for (index = 0; index < Vessels.Count(); index++) {
      VesselClass* obj = Vessels.Ptr(index);
      if (obj && !obj->IsInLimbo && obj->House == PlayerPtr &&
          obj->IsSelected) {
        team = obj->Group;
        if (team != kNoGroup) {
          set_form = obj->XFormOffset == kNoFormationOffset;
          TeamSpeed[team] = SPEED_WHEEL;
          TeamMaxSpeed[team] = MPH_LIGHT_SPEED;
          break;
        }
      }
    }
  }

  if (team == kNoGroup) {
    return;
  }
  // Now that we have a team, let's go set (or clear) the formation offsets.
  for (index = 0; index < Units.Count(); index++) {
    UnitClass* obj = Units.Ptr(index);
    if (obj && !obj->IsInLimbo && obj->House == PlayerPtr &&
        obj->Group == team) {
      obj->Mark(MARK_CHANGE);
      if (set_form) {
        long xc = Cell_X(Coord_Cell(obj->Center_Coord()));
        long yc = Cell_Y(Coord_Cell(obj->Center_Coord()));
        minx = std::min(xc, minx);
        maxx = std::max(xc, maxx);
        miny = std::min(yc, miny);
        maxy = std::max(yc, maxy);
        if (obj->Class->MaxSpeed < TeamMaxSpeed[team]) {
          TeamMaxSpeed[team] = obj->Class->MaxSpeed;
          TeamSpeed[team] = obj->Class->Speed;
        }
      } else {
        obj->XFormOffset = obj->YFormOffset = kNoFormationOffset;
      }
    }
  }

  for (index = 0; index < Infantry.Count(); index++) {
    InfantryClass* obj = Infantry.Ptr(index);
    if (obj && !obj->IsInLimbo && obj->House == PlayerPtr &&
        obj->Group == team) {
      obj->Mark(MARK_CHANGE);
      if (set_form) {
        long xc = Cell_X(Coord_Cell(obj->Center_Coord()));
        long yc = Cell_Y(Coord_Cell(obj->Center_Coord()));
        minx = std::min(xc, minx);
        maxx = std::max(xc, maxx);
        miny = std::min(yc, miny);
        maxy = std::max(yc, maxy);
        TeamMaxSpeed[team] = std::min(obj->Class->MaxSpeed, TeamMaxSpeed[team]);
      } else {
        obj->XFormOffset = obj->YFormOffset = kNoFormationOffset;
      }
    }
  }

  for (index = 0; index < Vessels.Count(); index++) {
    VesselClass* obj = Vessels.Ptr(index);
    if (obj && !obj->IsInLimbo && obj->House == PlayerPtr &&
        obj->Group == team) {
      obj->Mark(MARK_CHANGE);
      if (set_form) {
        long xc = Cell_X(Coord_Cell(obj->Center_Coord()));
        long yc = Cell_Y(Coord_Cell(obj->Center_Coord()));
        minx = std::min(xc, minx);
        maxx = std::max(xc, maxx);
        miny = std::min(yc, miny);
        maxy = std::max(yc, maxy);
        TeamMaxSpeed[team] = std::min(obj->Class->MaxSpeed, TeamMaxSpeed[team]);
      } else {
        obj->XFormOffset = obj->YFormOffset = kNoFormationOffset;
      }
    }
  }

  // All the units have been counted to find the bounding rectangle and
  // center of the formation, or to clear their offsets.  Now, if we're to
  // set them into formation, proceed to do so.  Otherwise, bail.
  //
  // Offsets are taken from where each unit already stands, so the formation
  // locks in the group's current shape rather than imposing a canned one.
  if (set_form) {
    int center_x = static_cast<int>((maxx - minx) / 2 + minx);
    int center_y = static_cast<int>((maxy - miny) / 2 + miny);

    for (index = 0; index < Units.Count(); index++) {
      UnitClass* obj = Units.Ptr(index);
      if (obj && !obj->IsInLimbo && obj->House == PlayerPtr &&
          obj->Group == team) {
        long xc = Cell_X(Coord_Cell(obj->Center_Coord()));
        long yc = Cell_Y(Coord_Cell(obj->Center_Coord()));

        obj->XFormOffset = static_cast<int>(xc - center_x);
        obj->YFormOffset = static_cast<int>(yc - center_y);
      }
    }

    for (index = 0; index < Infantry.Count(); index++) {
      InfantryClass* obj = Infantry.Ptr(index);
      if (obj && !obj->IsInLimbo && obj->House == PlayerPtr &&
          obj->Group == team) {
        long xc = Cell_X(Coord_Cell(obj->Center_Coord()));
        long yc = Cell_Y(Coord_Cell(obj->Center_Coord()));

        obj->XFormOffset = static_cast<int>(xc - center_x);
        obj->YFormOffset = static_cast<int>(yc - center_y);
      }
    }

    for (index = 0; index < Vessels.Count(); index++) {
      VesselClass* obj = Vessels.Ptr(index);
      if (obj && !obj->IsInLimbo && obj->House == PlayerPtr &&
          obj->Group == team) {
        long xc = Cell_X(Coord_Cell(obj->Center_Coord()));
        long yc = Cell_Y(Coord_Cell(obj->Center_Coord()));

        obj->XFormOffset = static_cast<int>(xc - center_x);
        obj->YFormOffset = static_cast<int>(yc - center_y);
      }
    }
  }
}

// Opens an editable reply addressed to the Westwood Online user who paged us
// from outside the game, or reports that nobody has paged.
//
// Only reachable when config::kWolapiEnabled; the caller gates it with
// `if constexpr` so the whole page-respond path folds away with the toggle.
// The caller has already checked that pWolapi is live. Marked maybe_unused
// because the only call site sits in a discarded `if constexpr` branch when
// the toggle is off, which clang otherwise reports as an unneeded static.
[[maybe_unused]] static void Start_External_Page_Reply() {
  if (*pWolapi->szExternalPager == '\0') {
    Session.Messages.Add_Message(
        nullptr, 0, TXT_WOL_NOTPAGED, PCOLOR_GOLD,
        TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
        Rule.MessageDelay * kTicksPerMinute);
    Sound_Effect(VOC_SYS_ERROR);
    return;
  }

  // An all-zero address is the flag the send path reads later to mean "this
  // is a reply to whoever paged me from outside the game".
  NetNumType blip;
  NetNodeType blop;
  memset(blip, 0, sizeof(blip));
  memset(blop, 0, sizeof(blop));
  Session.MessageAddress = IPXAddressClass(blip, blop);

  // Tell pWolapi not to reset szExternalPager while the reply is being typed.
  pWolapi->bFreezeExternalPager = true;

  char txt[MAX_MESSAGE_LENGTH + 32] = {};
  // TXT_TO comes from the localized string table, so verify the translation
  // still takes exactly one %s before using it.
  auto format = absl::ParsedFormat<'s'>::New(Text_String(TXT_TO));
  if (format != nullptr) {
    port::SafeCopy(txt,
                   absl::StrFormat(*format, pWolapi->szExternalPager).c_str());
  }

  Session.Messages.Add_Edit(Session.ColorIdx,
                            TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
                            txt, 0, 464);

  Map.Flag_To_Redraw(false);

  Keyboard->Clear();
}

// Fills in Session.GPacket with the message just finished in the edit buffer
// and sends it over IPX, either to every connection (broadcast address) or to
// the single address the F-key handler recorded.
//
// rc is the code MessageListClass::Input returned: 3 for a message that fit
// the edit buffer, 4 for one that spilled into the overflow buffer.
static void Send_Network_Chat_Message(const int rc) {
  Session.GPacket.Command = NET_MESSAGE;
  port::SafeCopy(Session.GPacket.Name, Session.Players[0]->Name);
  Session.GPacket.Message.Color = Session.ColorIdx;
  Session.GPacket.Message.NameCRC = Compute_Name_CRC(Session.GameName);

  if (rc == 3) {
    port::SafeCopy(Session.GPacket.Message.Buf,
                   Session.Messages.Get_Edit_Buf());
  } else {
    port::SafeCopy(Session.GPacket.Message.Buf,
                   Session.Messages.Get_Overflow_Buf());
    Session.Messages.Clear_Overflow_Buf();
  }

  // If 'F4' was hit, MessageAddress will be a broadcast address; send the
  // message to every player we have a connection with.
  if (Session.MessageAddress.Is_Broadcast()) {
    char* ptr = &Session.GPacket.Message.Buf[0];
    if (!strncmp(ptr, "SECRET UNITS ON ", 15) && NewUnitsEnabled) {
      *ptr = 'X';  // force it to an odd hack so we know it was broadcast.
      Enable_Secret_Units();
    }
    for (int i = 0; i < Ipx.Num_Connections(); ++i) {
      Ipx.Send_Global_Message(&Session.GPacket, sizeof(GlobalPacketType), 1,
                              Ipx.Connection_Address(Ipx.Connection_ID(i)));
      Ipx.Service();
    }
  } else {
    // Otherwise, MessageAddress contains the exact address to send to.
    // Send to that address only.
    Ipx.Send_Global_Message(&Session.GPacket, sizeof(GlobalPacketType), 1,
                            &Session.MessageAddress);
    Ipx.Service();
  }

  // Store this message in our LastMessage buffer; the computer may send us a
  // version of it later.
  port::SafeCopy(Session.LastMessage, Session.GPacket.Message.Buf);
}

// Processes inter-player message input. F1 through F8 open an editable message
// addressed to one player or to everyone, and RETURN sends what has been typed.
//
// The two session types put the text on the wire differently -- a serial
// packet or an IPX global packet -- but both build the same message from the
// same edit buffer.
static void Message_Input(KeyNumType& input) {
  char txt[MAX_MESSAGE_LENGTH + 32];

  // Check keyboard input for a request to send a message.
  // The 'to' argument for Add_Edit is prefixed to the message buffer; the
  // message buffer is big enough for the 'to' field plus MAX_MESSAGE_LENGTH.
  // To send the message, calling Get_Edit_Buf retrieves the buffer minus the
  // 'to' portion.  At the other end, the buffer allocated to display the
  // message must be MAX_MESSAGE_LENGTH plus the size of "From: xxx (house)".

  // The page-respond key is not one of the per-player F-keys, so it gets its
  // own gate ahead of them. Compiled and type-checked either way, reachable
  // only when Westwood Online is on.
  if constexpr (config::kWolapiEnabled) {
    if (input == kPageRespondKey && Session.Type == GAME_INTERNET &&
        !Session.Messages.Is_Edit() && pWolapi != nullptr &&
        !pWolapi->bConnectionDown) {
      Start_External_Page_Reply();
    }
  }

  if (Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH &&
      input >= KN_F1 && input < KN_F1 + Session.MaxPlayers &&
      !Session.Messages.Is_Edit()) {
    memset(txt, 0, 40);

    // For a serial game, send a message on F1 or F4; set 'txt' to the
    // "Message:" string & add an editable message to the list.
    if (Session.Type == GAME_NULL_MODEM || Session.Type == GAME_MODEM) {
      if (input == KN_F1 || input == KN_F1 + Session.MaxPlayers - 1) {
        port::SafeCopy(txt, Text_String(TXT_MESSAGE));  // "Message:"

        Session.Messages.Add_Edit(
            Session.ColorIdx, TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
            txt, 0, 464);

        Map.Flag_To_Redraw(false);
      }
    } else if ((Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) &&
               !Session.Messages.Is_Edit()) {
      // For a network game:
      // F1-F7 = "To <name> (house):" (only allowed if we're not in
      // ObiWan mode) F8 = "To All:"
      if (input == KN_F1 + Session.MaxPlayers - 1) {
        Session.MessageAddress = IPXAddressClass();    // set to broadcast
        port::SafeCopy(txt, Text_String(TXT_TO_ALL));  // "To All:"

        Session.Messages.Add_Edit(
            Session.ColorIdx, TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
            txt, 0, 464);

        Map.Flag_To_Redraw(false);

      } else if (input - KN_F1 < Ipx.Num_Connections() && !Session.ObiWan) {
        int id = Ipx.Connection_ID(input - KN_F1);
        Session.MessageAddress = *Ipx.Connection_Address(id);
        // TXT_TO comes from the localized string table, so verify the
        // translation still takes exactly one %s before using it.
        auto format = absl::ParsedFormat<'s'>::New(Text_String(TXT_TO));
        if (format != nullptr) {
          port::SafeCopy(
              txt, absl::StrFormat(*format, Ipx.Connection_Name(id)).c_str());
        }

        Session.Messages.Add_Edit(
            Session.ColorIdx, TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
            txt, 0, 464);

        Map.Flag_To_Redraw(false);
      }
    }
  }

  // Process message-system input; send the message out if RETURN is hit.
  const KeyNumType copy_input = input;
  const int rc = Session.Messages.Input(input);

  // If a single character has been added to an edit buffer, update the
  // display.
  if (rc == 1 && Session.Type != GAME_NORMAL) {
    Map.Flag_To_Redraw(false);
  }

  // If backspace was hit, redraw the map.  If the edit message was removed,
  // the map must be force-drawn, since it won't be able to compute the
  // cells to redraw; otherwise, let the map compute the cells to redraw,
  // by not force-drawing it, but just setting the IsToRedraw bit.
  if (rc == 2 && Session.Type != GAME_NORMAL) {
    if (copy_input == KN_ESC) {
      Map.Flag_To_Redraw(true);
      if constexpr (config::kWolapiEnabled) {
        if (pWolapi) {
          // Just in case user was responding to a page from outside the
          // game, and we had frozen the "szExternalPager".
          pWolapi->bFreezeExternalPager = false;
        }
      }
    } else {
      Map.Flag_To_Redraw(false);
    }
    Map.DisplayClass::IsToRedraw = true;
  }

  // Send a message
  if ((rc == 3 || rc == 4) && Session.Type != GAME_NORMAL &&
      Session.Type != GAME_SKIRMISH) {
    // Serial game: fill in a SerialPacketType & send it.
    // (Note: The size of the SerialPacketType.Command must be the same as
    // the EventClass.Type!)
    if (Session.Type == GAME_NULL_MODEM || Session.Type == GAME_MODEM) {
      // The modem layer hands back a raw byte buffer that the packet is built
      // into in place; there is no portable alternative to the cast here.
      auto* serial_packet =
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<SerialPacketType*>(NullModem.BuildBuf);

      serial_packet->Command = SERIAL_MESSAGE;
      port::SafeCopy(serial_packet->Name, Session.Players[0]->Name);
      serial_packet->ID = Session.ColorIdx;

      if (rc == 3) {
        port::SafeCopy(serial_packet->Message.Message,
                       Session.Messages.Get_Edit_Buf());
      } else {
        port::SafeCopy(serial_packet->Message.Message,
                       Session.Messages.Get_Overflow_Buf());
        Session.Messages.Clear_Overflow_Buf();
      }

      // Send the message, and store this message in our LastMessage
      // buffer; the computer may send us a version of it later.
      NullModem.Send_Message(NullModem.BuildBuf, sizeof(SerialPacketType), 1);

      // A chat message is how the secret units get switched on for everyone
      // at once: both ends recognize the phrase and enable them locally, so
      // the setting stays in step without a new packet type.
      char* ptr = &serial_packet->Message.Message[0];
      if (!strncmp(ptr, "SECRET UNITS ON ", 15) && NewUnitsEnabled) {
        Enable_Secret_Units();
      }
      port::SafeCopy(Session.LastMessage, serial_packet->Message.Message);
    } else if (Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) {
      if constexpr (config::kWolapiEnabled) {
        // An all-zero address is the flag Start_External_Page_Reply set,
        // meaning "this is a reply to whoever paged me from outside the
        // game". No F-key ever produces that address.
        NetNumType blip;
        NetNodeType blop;
        Session.MessageAddress.Get_Address(blip, blop);
        const bool reply_to_external_page =
            blip[0] + blip[1] + blip[2] + blip[3] + blop[0] + blop[1] +
                blop[2] + blop[3] + blop[4] + blop[5] ==
            0;
        if (reply_to_external_page) {
          // (As connection may have gone down.)
          if (pWolapi && !pWolapi->bConnectionDown) {
            // The HRESULT carries nothing here: asked not to wait for a
            // result, Page returns 0 whether or not the request went out.
            static_cast<void>(pWolapi->Page(pWolapi->szExternalPager,
                                            Session.Messages.Get_Edit_Buf(),
                                            false));
            pWolapi->bFreezeExternalPager = false;
          }
        } else {
          Send_Network_Chat_Message(rc);
        }
      } else {
        Send_Network_Chat_Message(rc);
      }
    }

    // Tell the map to completely update itself, since a message is now
    // missing.
    Map.Flag_To_Redraw(true);
  }
}

// Saves or replays the parts of the game state that the event stream alone
// cannot reconstruct: where the map is scrolled to, which objects are selected,
// and any team or formation hotkey pressed this frame.
//
// The selection is written as a checksum followed by the target list. On
// playback the checksum says whether the current selection already matches; if
// it does, the objects are read but not selected again, which stops the unit
// acknowledgement voices from firing again on every frame.
static void Do_Record_Playback() {
  int count;
  TARGET tgt;
  int i;
  COORDINATE coord;
  unsigned long sum;
  unsigned long sum2;
  unsigned long ltgt;

  // Record a game
  if (Session.Record) {
    // Save the map's location
    Session.RecordFile.Write(&Map.DesiredTacticalCoord,
                             sizeof(Map.DesiredTacticalCoord));

    // Save the current object list count
    count = static_cast<int>(CurrentObject.Count());
    Session.RecordFile.Write(&count, sizeof(count));

    // Save a CRC of the selected-object list.
    sum = 0;
    for (i = 0; i < count; i++) {
      ltgt = static_cast<unsigned long>(CurrentObject[i]->As_Target());
      sum += ltgt;
    }
    Session.RecordFile.Write(&sum, sizeof(sum));

    // Save all selected objects.
    for (i = 0; i < count; i++) {
      tgt = CurrentObject[i]->As_Target();
      Session.RecordFile.Write(&tgt, sizeof(tgt));
    }

    // Save team-selection and formation events
    Session.RecordFile.Write(&TeamEvent, sizeof(TeamEvent));
    Session.RecordFile.Write(&TeamNumber, sizeof(TeamNumber));
    Session.RecordFile.Write(&FormationEvent, sizeof(FormationEvent));
    Session.RecordFile.Write(TeamMaxSpeed, sizeof(TeamMaxSpeed));
    Session.RecordFile.Write(TeamSpeed, sizeof(TeamSpeed));
    Session.RecordFile.Write(&FormMove, sizeof(FormMove));
    Session.RecordFile.Write(&FormSpeed, sizeof(FormSpeed));
    Session.RecordFile.Write(&FormMaxSpeed, sizeof(FormMaxSpeed));
    TeamEvent = 0;
    TeamNumber = 0;
    FormationEvent = 0;
  }

  // Play back a game ("attract" mode)
  if (Session.Play) {
    // Read & set the map's location.
    if (Session.RecordFile.Read(&coord, sizeof(coord)) == sizeof(coord)) {
      if (coord != Map.DesiredTacticalCoord) {
        Map.Set_Tactical_Position(coord);
      }
    }

    if (Session.RecordFile.Read(&count, sizeof(count)) == sizeof(count)) {
      // Compute a CRC of the current object-selection list.
      sum = 0;
      for (i = 0; i < CurrentObject.Count(); i++) {
        ltgt = static_cast<unsigned long>(CurrentObject[i]->As_Target());
        sum += ltgt;
      }

      // Load the CRC of the objects on disk; if it doesn't match, select
      // all objects as they're loaded.
      Session.RecordFile.Read(&sum2, sizeof(sum2));
      if (sum2 != sum) {
        Unselect_All();
      }

      AllowVoice = true;

      for (i = 0; i < count; ++i) {
        if (Session.RecordFile.Read(&tgt, sizeof(tgt)) == sizeof(tgt)) {
          ObjectClass* obj = As_Object(tgt);
          if (obj != nullptr && sum2 != sum) {
            obj->Select();
            AllowVoice = false;
          }
        }
      }

      AllowVoice = true;
    }

    // Save team-selection and formation events
    Session.RecordFile.Read(&TeamEvent, sizeof(TeamEvent));
    Session.RecordFile.Read(&TeamNumber, sizeof(TeamNumber));
    Session.RecordFile.Read(&FormationEvent, sizeof(FormationEvent));
    if (TeamEvent) {
      Handle_Team(TeamNumber, TeamEvent - 1);
    }
    if (FormationEvent) {
      Toggle_Formation();
    }

    Session.RecordFile.Read(TeamMaxSpeed, sizeof(TeamMaxSpeed));
    Session.RecordFile.Read(TeamSpeed, sizeof(TeamSpeed));
    Session.RecordFile.Read(&FormMove, sizeof(FormMove));
    Session.RecordFile.Read(&FormSpeed, sizeof(FormSpeed));
    Session.RecordFile.Read(&FormMaxSpeed, sizeof(FormMaxSpeed));
    // The map isn't drawn in playback mode, so draw it here.
    Map.Render();
  }
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
      TeamEvent = 0;
      TeamNumber = 0;
      FormationEvent = 0;
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
      if (Session.Type == GAME_IPX) {
        if (!Session.Play) {
          Shutdown_Network();
        }
      }
    }

    // If we're playing back, the mouse will be hidden; show it.
    // Also, set all variables back to normal, to return to the main menu.
    if (Session.Play) {
      Show_Mouse();
      Session.Type = GAME_NORMAL;
      Session.Play = 0;
    }
  }

  // Free the scenario description buffers
  Session.Free_Scenario_Descriptions();
}

// Handles keyboard input while the tactical map is displayed.
//
// Every clause that recognizes a key sets input to KN_NONE, so a key is only
// ever acted on once no matter how many clauses could match it. Message input
// gets first refusal for that reason: a player typing chat must not also be
// commanding their units.
void Keyboard_Process(KeyNumType& input) {
  ObjectClass* obj;
  int index;

  // Don't do anything if there is not keyboard event.
  if (input == KN_NONE) {
    return;
  }
  // For network & modem, process user input for inter-player messages.
  Message_Input(input);

  // The VK_BIT must be stripped from the "plain" value of the key so that a
  // comparison to KN_1, for example, will yield true if in fact the "1"
  // key was pressed.

  constexpr unsigned kModifierBits =
      unsigned{WWKEY_SHIFT_BIT} | unsigned{WWKEY_ALT_BIT} |
      unsigned{WWKEY_CTRL_BIT} | unsigned{WWKEY_VK_BIT};
  auto plain = static_cast<KeyNumType>(unsigned{input} & ~kModifierBits);
  auto key = static_cast<KeyNumType>(unsigned{input} & ~unsigned{WWKEY_VK_BIT});

  if constexpr (config::kCheatKeysEnabled) {
    if (Debug_Flag) {
      HousesType h;

      switch (static_cast<unsigned>(input)) {
        case static_cast<unsigned>(KN_M) | static_cast<unsigned>(KN_SHIFT_BIT):
        case static_cast<unsigned>(KN_M) | static_cast<unsigned>(KN_ALT_BIT):
        case static_cast<unsigned>(KN_M) | static_cast<unsigned>(KN_CTRL_BIT):
          for (h = HOUSE_FIRST; h < HOUSE_COUNT; ++h) {
            Houses.Ptr(h)->Refund_Money(10000);
          }
          break;

        default:
          break;
      }
    }
  }

  if constexpr (config::kCheatKeysEnabled) {
    if (Debug_Playtest &&
        static_cast<unsigned>(input) ==
            (static_cast<unsigned>(KN_W) | static_cast<unsigned>(KN_ALT_BIT))) {
      PlayerPtr->Blockage = false;
      PlayerPtr->Flag_To_Win();
    }

    if ((Debug_Flag || Debug_Playtest) && plain == KN_F4) {
      if (Session.Type == GAME_NORMAL) {
        Debug_Unshroud = !Debug_Unshroud;
        Map.Flag_To_Redraw(true);
      }
    }

    if (Debug_Flag && input == KN_SLASH) {
      if (Session.Type != GAME_NORMAL) {
        SpecialDialog = SDLG_SPECIAL;
        input = KN_NONE;
      } else {
        Special_Dialog();
      }
    }
  }

  // Process prerecorded team selection. This will be an additive select
  // if the SHIFT key is held down. It will create the team if the
  // CTRL or ALT key is held down.
  int action = 0;
  if ((unsigned{input} & unsigned{WWKEY_SHIFT_BIT}) != 0U) {
    action = 1;
  }
  if ((unsigned{input} & unsigned{WWKEY_ALT_BIT}) != 0U) {
    action = 3;
  }
  if ((unsigned{input} & unsigned{WWKEY_CTRL_BIT}) != 0U) {
    action = 2;
  }

  // If the "N" key is pressed, then select the next object.
  if (key != 0 && key == Options.KeyNext) {
    if (action) {
      obj = Map.Prev_Object(CurrentObject.Count() ? CurrentObject[0] : nullptr);
    } else {
      obj = Map.Next_Object(CurrentObject.Count() ? CurrentObject[0] : nullptr);
    }
    if (obj != nullptr) {
      Unselect_All();
      obj->Select();
      Map.Center_Map();
      Map.Flag_To_Redraw(true);
    }
    input = KN_NONE;
  }
  if (key != 0 && key == Options.KeyPrevious) {
    if (action) {
      obj = Map.Next_Object(CurrentObject.Count() ? CurrentObject[0] : nullptr);
    } else {
      obj = Map.Prev_Object(CurrentObject.Count() ? CurrentObject[0] : nullptr);
    }
    if (obj != nullptr) {
      Unselect_All();
      obj->Select();
      Map.Center_Map();
      Map.Flag_To_Redraw(true);
    }
    input = KN_NONE;
  }

  // All selected units will go into idle mode.
  if (key != 0 && key == Options.KeyStop) {
    if (CurrentObject.Count()) {
      for (index = 0; index < CurrentObject.Count(); index++) {
        const ObjectClass* tech = CurrentObject[index];

        if (tech != nullptr &&
            (tech->Can_Player_Move() ||
             (tech->Can_Player_Fire() && tech->What_Am_I() != RTTI_BUILDING))) {
          OutList.Add(EventClass(EventClass::IDLE, TargetClass(tech)));
        }
      }
    }
    input = KN_NONE;
  }

  // All selected units will attempt to go into guard area mode.
  if (key != 0 && key == Options.KeyGuard) {
    if (CurrentObject.Count()) {
      for (index = 0; index < CurrentObject.Count(); index++) {
        const ObjectClass* tech = CurrentObject[index];

        if (tech != nullptr && tech->Can_Player_Move() &&
            tech->Can_Player_Fire()) {
          OutList.Add(EventClass(TargetClass(tech), MISSION_GUARD_AREA));
        }
      }
    }
    input = KN_NONE;
  }

  // All selected units will attempt to scatter.
  if (key != 0 && key == Options.KeyScatter) {
    if (CurrentObject.Count()) {
      for (index = 0; index < CurrentObject.Count(); index++) {
        const ObjectClass* tech = CurrentObject[index];

        if (tech != nullptr && tech->Can_Player_Move()) {
          OutList.Add(EventClass(EventClass::SCATTER, TargetClass(tech)));
        }
      }
    }
    input = KN_NONE;
  }

  // Center the map around the currently selected objects. If no
  // objects are selected, then fall into the home case.
  if (key != 0 && (key == Options.KeyHome1 || key == Options.KeyHome2)) {
    if (CurrentObject.Count()) {
      Map.Center_Map();
      Map.Flag_To_Redraw(true);
      input = KN_NONE;
    } else {
      input = Options.KeyBase;
    }
  }

  // Center the map about the construction yard or construction vehicle
  // if one is present.
  if (key != 0 && key == Options.KeyBase) {
    Unselect_All();
    if (PlayerPtr->CurBuildings) {
      for (index = 0; index < Buildings.Count(); index++) {
        BuildingClass* building = Buildings.Ptr(index);

        if (building != nullptr && !building->IsInLimbo &&
            building->House == PlayerPtr && *building == STRUCT_CONST) {
          Unselect_All();
          building->Select();
          if (building->IsLeader) {
            break;
          }
        }
      }
    }
    if (CurrentObject.Count() == 0 && PlayerPtr->CurUnits) {
      for (index = 0; index < Units.Count(); index++) {
        UnitClass* unit = Units.Ptr(index);

        if (unit != nullptr && !unit->IsInLimbo && unit->House == PlayerPtr &&
            *unit == UNIT_MCV) {
          Unselect_All();
          unit->Select();
          break;
        }
      }
    }
    if (CurrentObject.Count()) {
      Map.Center_Map();
    } else {
      if (PlayerPtr->Center != 0) {
        Map.Center_Map(PlayerPtr->Center);
      }
    }
    Map.Flag_To_Redraw(true);
    input = KN_NONE;
  }

  // Toggle the status of formation for the current team
  if (key != 0 && key == Options.KeyFormation) {
    Toggle_Formation();
    input = KN_NONE;
  }

  // In multiplayer the resign key brings up the surrender dialog. Single
  // player has the mission abort in the options menu instead: surrendering
  // there would only self-destruct the base and lose the mission.
  if (key != 0 && key == Options.KeyResign) {
    if (Session.Type != GAME_NORMAL && !PlayerLoses && !PlayerPtr->IsDefeated) {
      SpecialDialog = SDLG_SURRENDER;
    }
    input = KN_NONE;
  }

  // Handle making and breaking alliances.
  if (key != 0 && key == Options.KeyAlliance) {
    if (Session.Type != GAME_NORMAL || Debug_Flag) {
      if (CurrentObject.Count() && !PlayerPtr->IsDefeated) {
        if (CurrentObject[0]->Owner() != PlayerPtr->Class->House) {
          OutList.Add(EventClass(EventClass::ALLY, CurrentObject[0]->Owner()));
        }
      }
    }
    input = KN_NONE;
  }

  // Select all the units on the current display. This is equivalent to
  // drag selecting the whole view.
  if (key != 0 && key == Options.KeySelectView) {
    // The corners are leptons relative to the tactical view, so 0 is its top
    // left and the size below is its full extent -- a drag select of everything
    // on screen.
    Map.Select_These(0x00000000,
                     XY_Coord(Map.TacLeptonWidth, Map.TacLeptonHeight));
    input = KN_NONE;
  }

  // Toggles the repair state similarly to pressing the repair button.
  if (key != 0 && key == Options.KeyRepair) {
    Map.Repair_Mode_Control(-1);
    input = KN_NONE;
  }

  // Toggles the sell state similarly to pressing the sell button.
  if (key != 0 && key == Options.KeySell) {
    Map.Sell_Mode_Control(-1);
    input = KN_NONE;
  }

  // Toggles the map zoom mode similarly to pressing the map button.
  if (key != 0 && key == Options.KeyMap) {
    Map.Zoom_Mode_Control();
    input = KN_NONE;
  }

  // Scrolls the sidebar up one slot.
  if (key != 0 && key == Options.KeySidebarUp) {
    Map.Scroll(true, -1);
    input = KN_NONE;
  }

  // Scrolls the sidebar down one slot.
  if (key != 0 && key == Options.KeySidebarDown) {
    Map.Scroll(false, -1);
    input = KN_NONE;
  }

  // Brings up the options dialog box.
  if (key != 0 && (key == Options.KeyOption1 || key == Options.KeyOption2)) {
    Map.Help_Text(TXT_NONE);  // Turns off help text.
    Queue_Options();
    input = KN_NONE;
  }

  // Scrolls the tactical map in the direction specified.
  int distance = CELL_LEPTON_W;
  if (key != 0 && key == Options.KeyScrollLeft) {
    Map.Scroll_Map(DIR_W, distance, true);
    input = KN_NONE;
  }
  if (key != 0 && key == Options.KeyScrollRight) {
    Map.Scroll_Map(DIR_E, distance, true);
    input = KN_NONE;
  }
  if (key != 0 && key == Options.KeyScrollUp) {
    Map.Scroll_Map(DIR_N, distance, true);
    input = KN_NONE;
  }
  if (key != 0 && key == Options.KeyScrollDown) {
    Map.Scroll_Map(DIR_S, distance, true);
    input = KN_NONE;
  }

  // Teams are handled by the 10 special team keys. The manual comparison
  // to the KN numbers is because the Windows keyboard driver can vary
  // the base code number for the key depending on the shift or alt key
  // state!
  if (input != 0 && (plain == Options.KeyTeam1 || plain == KN_1)) {
    Handle_Team(0, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam2 || plain == KN_2)) {
    Handle_Team(1, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam3 || plain == KN_3)) {
    Handle_Team(2, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam4 || plain == KN_4)) {
    Handle_Team(3, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam5 || plain == KN_5)) {
    Handle_Team(4, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam6 || plain == KN_6)) {
    Handle_Team(5, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam7 || plain == KN_7)) {
    Handle_Team(6, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam8 || plain == KN_8)) {
    Handle_Team(7, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam9 || plain == KN_9)) {
    Handle_Team(8, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam10 || plain == KN_0)) {
    Handle_Team(9, action);
    input = KN_NONE;
  }

  // Handle the bookmark hotkeys.
  if (input != 0 && plain == Options.KeyBookmark1 && !MapEditorActive) {
    Handle_View(0, action);
    input = KN_NONE;
  }
  if (input != 0 && plain == Options.KeyBookmark2 && !MapEditorActive) {
    Handle_View(1, action);
    input = KN_NONE;
  }
  if (input != 0 && plain == Options.KeyBookmark3 && !MapEditorActive) {
    Handle_View(2, action);
    input = KN_NONE;
  }
  if (input != 0 && plain == Options.KeyBookmark4 && !MapEditorActive) {
    Handle_View(3, action);
    input = KN_NONE;
  }

  if constexpr (config::kCheatKeysEnabled) {
    if (input != 0 && Debug_Flag &&
        (static_cast<unsigned>(input) & static_cast<unsigned>(KN_RLSE_BIT)) ==
            0U) {
      Debug_Key(input);
    }
  }
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
  if (SampleType) {
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

void IPX_Call_Back() {
  Ipx.Service();

  // Read packets only if the game is "closed", so we don't steal global
  // messages from the connection dialogs.
  if (!Session.NetOpen) {
    if (Ipx.Get_Global_Message(&Session.GPacket, &Session.GPacketlen,
                               &Session.GAddress, &Session.GProductID)) {
      if (Session.GProductID == IPXGlobalConnClass::COMMAND_AND_CONQUER0) {
        // If this is another player signing off, remove the connection &
        // mark that player's house as non-human, so the computer will take
        // it over.
        if (Session.GPacket.Command == NET_SIGN_OFF) {
          for (int i = 0; i < Ipx.Num_Connections(); i++) {
            int id = Ipx.Connection_ID(i);

            if (Session.GAddress == *Ipx.Connection_Address(id)) {
              Destroy_Connection(id, 0);
            }
          }
        } else {
          // Process a message from another user.
          if (Session.GPacket.Command == NET_MESSAGE) {
            bool msg_ok = false;

            // If NetProtect is set, make sure this message came from within
            // this game.
            if (!Session.NetProtect) {
              msg_ok = true;
            } else {
              msg_ok = Session.GPacket.Message.NameCRC ==
                       Compute_Name_CRC(Session.GameName);
            }

            if (msg_ok) {
              if (!Session.Messages.Concat_Message(
                      Session.GPacket.Name, Session.GPacket.Message.Color,
                      Session.GPacket.Message.Buf,
                      Rule.MessageDelay * kTicksPerMinute)) {
                if (NewUnitsEnabled && !strncmp(Session.GPacket.Message.Buf,
                                                "XECRET UNITS ON ", 15)) {
                  Session.GPacket.Message.Buf[0] = 'S';
                  Enable_Secret_Units();
                }
                Session.Messages.Add_Message(
                    Session.GPacket.Name, Session.GPacket.Message.Color,
                    Session.GPacket.Message.Buf, Session.GPacket.Message.Color,
                    TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
                    Rule.MessageDelay * kTicksPerMinute);

                Sound_Effect(VOC_INCOMING_MESSAGE);
              }

              // Tell the map to do a partial update (just to force the
              // messages to redraw).
              Map.Flag_To_Redraw(true);

              // Save this message in our last-message buffer
              port::SafeCopy(Session.LastMessage, Session.GPacket.Message.Buf);
            }
          } else {
            Process_Global_Packet(&Session.GPacket, &Session.GAddress);
          }
        }
      }
    }
  }
}

SourceType Source_From_Name(const char* name) {
  if (name) {
    for (SourceType source = SOURCE_FIRST; source < SOURCE_COUNT; ++source) {
      if (stricmp(SourceName[source], name) == 0) {
        return source;
      }
    }
  }
  return SOURCE_NONE;
}

const char* Name_From_Source(const SourceType source) {
  if (static_cast<unsigned>(source) < SOURCE_COUNT) {
    return SourceName[source];
  }
  return "None";
}

TheaterType Theater_From_Name(const char* name) {
  if (name != nullptr) {
    for (TheaterType index = THEATER_FIRST; index < THEATER_COUNT; ++index) {
      if (stricmp(name, Theaters[index].Name) == 0) {
        return index;
      }
    }
  }
  return THEATER_NONE;
}

FacingType KN_To_Facing(const unsigned input) {
  constexpr unsigned kModifierBits = static_cast<unsigned>(KN_ALT_BIT) |
                                     static_cast<unsigned>(KN_SHIFT_BIT) |
                                     static_cast<unsigned>(KN_CTRL_BIT);
  // C++17 init-statement: key exists only for the switch.
  switch (const unsigned key = input & ~kModifierBits; key) {
    case KN_LEFT:
      return FACING_W;

    case KN_RIGHT:
      return FACING_E;

    case KN_UP:
      return FACING_N;

    case KN_DOWN:
      return FACING_S;

    case KN_UPLEFT:
      return FACING_NW;

    case KN_UPRIGHT:
      return FACING_NE;

    case KN_DOWNLEFT:
      return FACING_SW;

    case KN_DOWNRIGHT:
      return FACING_SE;

    default:
      break;
  }
  return FACING_NONE;
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
      int x, y;
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

  // Allocate extra memory for uncompressed shapes as needed
  Reallocate_Big_Shape_Buffer();

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
  if (SampleType && Theme.What_Is_Playing() == THEME_NONE) {
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
  if (!Session.Play) {
    if (SpecialDialog == SDLG_NONE && GameInFocus) {
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
    DisplayClass::Layer[LAYER_GROUND].Sort();
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

  // Is there a memory trasher altering the map??
  if (Debug_Check_Map) {
    if (!Map.Validate()) {
      if (WWMessageBox().Process(kLanguageText.map_error, kLanguageText.stop,
                                 kLanguageText.continue_button) == 0) {
        GameActive = false;
      }
      Map.Validate();  // give debugger a chance to catch it
    }
  }

  if (Debug_MotionCapture) {
    // One captured screen per element. Empty between runs. Deliberately leaked
    // rather than given static storage duration with a destructor, which would
    // run at exit after the graphics system is already gone.
    static auto& frames = *new std::vector<std::vector<char>>();
    // Doubles as the frame counter and the end-of-run signal: reaching
    // frames.size() ends the capture and flushes to disk.
    static base::ssize sequence = 0;

    if (frames.empty()) {
      // Sized when a capture run starts rather than once per process, so that
      // an edit to MovieTime takes effect on the next run.
      int frame_count = Rule.MovieTime * kTicksPerMinute;
      frames.resize(frame_count);
    }

    // Leaked for the same reason as frames above.
    static auto& temp_page = *new GraphicBufferClass(
        SeenBuff.Get_Width(), SeenBuff.Get_Height(), nullptr,
        static_cast<long>(SeenBuff.Get_Width()) * SeenBuff.Get_Height());

    base::ssize size =
        static_cast<base::ssize>(SeenBuff.Get_Width()) * SeenBuff.Get_Height();

    if (sequence < std::ssize(frames)) {
      // A no-op on a frame reused from an earlier run of the same resolution.
      frames[sequence].resize(size);

      SeenBuff.Blit(temp_page);
      std::memcpy(frames[sequence].data(), temp_page.Get_Buffer(), size);
      sequence++;
    } else {
      Debug_MotionCapture = false;

      CDFileClass file;
      file.Cache(200000);
      char filename[30];

      for (base::ssize index = 0; index < sequence; index++) {
        std::memcpy(temp_page.Get_Buffer(), frames[index].data(), size);
        snprintf(filename, sizeof(filename), "cap%04zd.pcx", index);
        file.Set_Name(filename);

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

void Go_Editor(const bool flag) {
  // Go into Scenario Editor mode
  if (flag) {
    MapEditorActive = true;
    Debug_Unshroud = true;

    // Un-select any selected objects
    Unselect_All();

    // Turn off the sidebar if it's on
    Map.Activate(0);

    // Reset the map's Button list for the new mode
    Map.Init_IO();

    // Force a complete redraw of the screen
    HiddenPage.Clear();
    Map.Flag_To_Redraw(true);
    Map.Render();

  } else {
    // Go into normal game mode
    MapEditorActive = false;
    Debug_Unshroud = false;

    // Un-select any selected objects
    Unselect_All();

    // Reset the map's Button list for the new mode
    Map.Init_IO();

    // Force a complete redraw of the screen
    HidPage.Clear();
    Map.Flag_To_Redraw(true);
    Map.Render();
  }
}

MixFileVqaIo::MixFileVqaIo() = default;

MixFileVqaIo::~MixFileVqaIo() { Close(); }

int MixFileVqaIo::Open(const char* filename) {
  auto file = std::make_unique<CCFileClass>(filename);

  if (!file->Is_Available()) {
    return 1;
  }
  if (file->Open(filename, FileAccess::kRead) == -1) {
    return 1;
  }

  file_ = std::move(file);
  return 0;
}

int MixFileVqaIo::Read(void* buffer, const int64_t bytes) {
  return file_->Read(buffer, bytes) != bytes;
}

int MixFileVqaIo::Seek(const int64_t offset, const int origin) {
  return file_->Seek(offset, origin) == -1;
}

void MixFileVqaIo::Close() {
  if (file_ != nullptr) {
    file_->Close();
    file_.reset();
  }
}

void Rebuild_Interpolated_Palette(unsigned char* interpal) {
  for (int y = 0; y < 255; y++) {
    for (int x = y + 1; x < 256; x++) {
      *(interpal + (y * 256 + x)) = *(interpal + (x * 256 + y));
    }
  }
}

int Load_Interpolated_Palettes(const char* filename, const bool add) {
  int num_palettes = 0;
  int i;
  int start_palette;

  PalettesRead = false;
  CCFileClass file(filename);

  if (!add) {
    for (i = 0; i < std::ssize(InterpolatedPalettes); i++) {
      InterpolatedPalettes[i] = nullptr;
    }
    start_palette = 0;
  } else {
    for (start_palette = 0; start_palette < std::ssize(InterpolatedPalettes);
         start_palette++) {
      if (!InterpolatedPalettes[start_palette]) {
        break;
      }
    }
  }

  // Hack another interpolated palette if the requested one is
  // not present.
  if (!file.Is_Available()) {
    file.Set_Name("AAGUN.VQP");
  }

  if (file.Is_Available()) {
    file.Open(FileAccess::kRead);
    file.Read(&num_palettes, 4);

    for (i = 0; i < num_palettes; i++) {
      // 256 x 256: the blended result for every pair of palette indices.
      InterpolatedPalettes[i + start_palette] = new unsigned char[65536]();
      // Only the lower triangle is stored, row y holding y + 1 entries;
      // Rebuild_Interpolated_Palette() mirrors it to fill the rest.
      for (int y = 0; y < 256; y++) {
        file.Read(InterpolatedPalettes[i + start_palette] +
                      static_cast<base::ssize>(y) * 256,
                  y + 1);
      }

      Rebuild_Interpolated_Palette(InterpolatedPalettes[i + start_palette]);
    }

    PalettesRead = true;
    file.Close();
  }
  PaletteCounter = 0;
  return num_palettes;
}

void Free_Interpolated_Palettes() {
  for (int i = 0; i < std::ssize(InterpolatedPalettes); i++) {
    if (InterpolatedPalettes[i]) {
      delete[] InterpolatedPalettes[i];
      InterpolatedPalettes[i] = nullptr;
    }
  }
}

void Play_Movie(const char* name, const ThemeType theme, bool clear_screen) {
  DLOG(INFO) << "Play_Movie: " << name;

  // A movie blocks until it finishes, which would stall every other player in
  // a multiplayer session and interrupt editing, so both modes skip it.
  if (MapEditorActive) {
    return;
  }
  if (Session.Type != GAME_NORMAL) {
    return;
  }

  if (name) {
    auto fullname =
        std::filesystem::path(name).replace_extension(".VQA").string();
    auto pal_name =
        std::filesystem::path(name).replace_extension(".VQP").string();
    if (!CCFileClass(fullname.c_str()).Is_Available()) {
      DLOG(WARNING) << "Play_Movie: file not found: " << fullname;
      return;
    }
    Anim_Init();

    // Fade audio and video to black before launching the VQA player. The
    // adjust-set-adjust-set sequence below drives the palette to black and
    // then back, which is what produces the fade rather than a hard cut.
    Hide_Mouse();
    Theme.Queue_Song(theme);
    if (PreserveVQAScreen == 0 && !clear_screen) {
      BlackPalette.Set(kFadePaletteMedium);
      VisiblePage.Clear();
      BlackPalette.Adjust(0x08, WhitePalette);
      BlackPalette.Set();
      BlackPalette.Adjust(0xFF);
      BlackPalette.Set();
    }
    PreserveVQAScreen = 0;
    Keyboard->Clear();

    VqaPlayer player;
    MixFileVqaIo movie_io;  // Must outlive the open movie.
    player.SetIo(&movie_io);

    if (IsVQ640) {
      AnimControl.ImageWidth = 640;
      AnimControl.ImageHeight = 400;
      AnimControl.ImageBuf = VQ640.Get_Offset();
    } else {
      AnimControl.ImageWidth = 320;
      AnimControl.ImageHeight = 200;
      AnimControl.ImageBuf = SysMemPage.Get_Offset();
    }

    if (!Debug_Quiet && Get_Digi_Handle() != -1) {
      AnimControl.OptionFlags |= VQAOPTF_AUDIO;
    } else {
      AnimControl.OptionFlags &= ~VQAOPTF_AUDIO;
    }

    if (player.Open(fullname.c_str(), &AnimControl) == 0) {
      Brokeout = false;
      if (!IsVQ640) {
        Load_Interpolated_Palettes(pal_name.c_str());
      }
      SysMemPage.Clear();
      InMovie = true;
      player.Play(VQAMODE_RUN);
      player.Close();
      InMovie = false;
      if (!IsVQ640) {
        Free_Interpolated_Palettes();
      }
      IsVQ640 = false;

      // Early exit leaves the palette in an inconsistent state.
      if (Brokeout) {
        clear_screen = true;
        VisiblePage.Clear();
        Brokeout = false;
      }
    } else {
      DLOG(FATAL) << "VQA_Open failed unexpectedly";
    }

    // The VQA player may leave the framebuffer and palette dirty.
    if (clear_screen) {
      VisiblePage.Clear();
      BlackPalette.Adjust(0x08, WhitePalette);
      BlackPalette.Set();
      BlackPalette.Adjust(0xFF);
      BlackPalette.Set();
    }
    Show_Mouse();
  }
}

void Play_Movie(const VQType name, const ThemeType theme,
                const bool clear_screen) {
  if (name != VQ_NONE) {
    if (name == VQ_REDINTRO) {
      IsVQ640 = true;
    }
    Play_Movie(VQName[name], theme, clear_screen);
    IsVQ640 = false;
  }
}

// Unselect() removes the object from CurrentObject, so index 0 is always the
// next one to drop and the count shrinks on every pass.
void Unselect_All() {
  while (CurrentObject.Count()) {
    CurrentObject[0]->Unselect();
  }
}

std::string Fading_Table_Name(const char* base, const TheaterType theater) {
  // Build filename: first character of theater root + base name + .MRF
  // extension
  const auto root = std::string(1, Theaters[theater].Root[0]) + base;
  const auto file_path = std::filesystem::path(root).replace_extension(".MRF");
  return file_path.string();
}

// Icons are 24x24 pixels in the source art, and the radar draws them at
// zoom_factor pixels per cell -- so each output pixel stands for several source
// ones.
//
// Rather than point-sampling, each output pixel takes the first
// non-transparent of the nine source pixels around its sample point (the
// off_x/off_y offsets). Without that spread, anything thinner than the sample
// step -- walls, most of a structure's outline -- would vanish at radar zoom.
std::unique_ptr<char[]> Get_Radar_Icon(const void* shapefile,
                                       const int shape_num, int frames,
                                       const int zoom_factor) {
  static int off_x[] = {0, 0, -1, 1, 0, -1, 1, -1, 1};
  static int off_y[] = {0, 0, -1, 1, 0, -1, 1, -1, 1};

  // If there is no shape file, then there can be no radar icon imagery.
  if (shapefile == nullptr) {
    return nullptr;
  }

  // Get the pixel width and height of the frame we built.  This will
  // be used to extract icons and build pixels.
  int pixel_width = Get_Build_Frame_Width(shapefile);
  int pixel_height = Get_Build_Frame_Height(shapefile);

  // Find the width and height in icons, adjust these by half an
  // icon because the artists may be sloppy and miss the edge of an
  // icon one way or the other.
  int icon_width = (pixel_width + 12) / 24;
  int icon_height = (pixel_height + 12) / 24;

  // If we have been told to build as many frames as possible, then
  // find out how many frames there are to build.
  if (frames == -1) {
    frames = Get_Build_Frame_Count(shapefile);
  }

  // Allocate a position to store our icons.  If the alloc fails then
  // we don't add these icons to the set.
  auto result =
      std::make_unique<char[]>(icon_width * icon_height * 9 * frames + 2);
  char* buffer = result.get();
  *buffer++ = static_cast<char>(icon_width);
  *buffer++ = static_cast<char>(icon_height);
  const int val = 24 / zoom_factor;

  for (int frame_num = 0; frame_num < frames; ++frame_num) {
    // Build the current frame.  If the frame can not be built then we
    // just need to skip past this set of icons and try to build the
    // next frame.
    void* ptr =
        Build_Frame(shapefile, static_cast<uint16_t>(shape_num + frame_num),
                    SysMemPage.Get_Buffer());
    if (ptr != nullptr) {
      ptr = Get_Shape_Header_Data(ptr);

      // Loop through the icon width and the icon height building icons
      // into the buffer pointer.  When the getx or gety falls outside of
      // the width and height of the shape, just insert transparent pixels.
      for (int icon_y = 0; icon_y < icon_height; icon_y++) {
        for (int icon_x = 0; icon_x < icon_width; icon_x++) {
          for (int y = 0; y < zoom_factor; y++) {
            for (int x = 0; x < zoom_factor; x++) {
              int getx = icon_x * 24 + x * val + zoom_factor / 2;
              int gety = icon_y * 24 + y * val + zoom_factor / 2;
              if (getx < pixel_width && gety < pixel_height) {
                char pixel = 0;
                for (int lp = 0; lp < 9; ++lp) {
                  pixel = *(static_cast<char*>(ptr) +
                            static_cast<base::ssize>(gety - off_y[lp]) *
                                pixel_width +
                            getx - off_x[lp]);

                  if (pixel == LTGREEN) {
                    pixel = 0;
                  }
                  if (pixel) {
                    break;
                  }
                }
                *buffer++ = pixel;
              } else {
                *buffer++ = 0;
              }
            }
          }
        }
      }
    } else {
      buffer += static_cast<base::ssize>(icon_width * icon_height) * 9;
    }
  }
  return result;
}

void CC_Draw_Shape(const void* shapefile, const int shape_num, const int x,
                   const int y, const WindowNumberType window,
                   ShapeFlags_Type flags, const void* fading_data,
                   const void* ghostdata, const DirType rotation,
                   const long scale) {
  // Special kludge for E3 to prevent crashes
  //
  // Callers that ask for ghosting or fading without supplying the table get
  // the display class's default rather than a null dereference.
  if (flags & SHAPE_GHOST && !ghostdata) {
    ghostdata = DisplayClass::SpecialGhost;
  }
  if (flags & SHAPE_FADING && !fading_data) {
    fading_data = DisplayClass::FadingShade;
  }

  static unsigned char* x_buffer = nullptr;

  if (!x_buffer) {
    x_buffer = new unsigned char[kShapeBufferSize];
  }

  if (shapefile != nullptr && shape_num != -1) {
    int width = Get_Build_Frame_Width(shapefile);
    int height = Get_Build_Frame_Height(shapefile);

    // In WIn95, build shape returns a pointer to the shape not its size
    void* shape_pointer =
        Build_Frame(shapefile, static_cast<uint16_t>(shape_num), _ShapeBuffer);
    if (shape_pointer) {
      GraphicViewPortClass draw_window(
          LogicPage->Get_Graphic_Buffer(),
          WindowList[window][WINDOWX] + LogicPage->Get_XPos(),
          WindowList[window][WINDOWY] + LogicPage->Get_YPos(),
          WindowList[window][WINDOWWIDTH], WindowList[window][WINDOWHEIGHT]);
      auto* buffer = static_cast<unsigned char*>(shape_pointer);

      UseOldShapeDraw = false;
      // Rotation and scale handler.
      // 0x0100 is 1.0 in the 24.8 fixed point scale, so this is "no rotation
      // and no scaling" -- the common case, which skips the slow path below.
      if (rotation != DIR_N || scale != 0x0100) {
        // Get the raw shape data without the new header and flag to use the old
        // shape drawing
        UseOldShapeDraw = true;
        buffer =
            static_cast<unsigned char*>(Get_Shape_Header_Data(shape_pointer));

        BitmapClass bm(width, height, buffer);
        width *= 2;
        height *= 2;
        memset(x_buffer, '\0', kShapeBufferSize);
        GraphicBufferClass gb(width, height, x_buffer);
        TPoint2D pt(width / 2, height / 2);

        gb.Scale_Rotate(bm, pt, static_cast<int32_t>(scale),
                        static_cast<uint8_t>(256 - rotation + 64));
        buffer = x_buffer;
      }

      // Special shadow drawing code (used for aircraft and bullets). Both bits
      // together mean a shadow; either one alone means something else.
      constexpr auto kShadowMask = SHAPE_FADING | SHAPE_PREDATOR;
      const auto shadow_bits = flags & kShadowMask;
      if (shadow_bits == kShadowMask) {
        flags = flags & ~kShadowMask;
        flags = flags | SHAPE_GHOST;
        ghostdata = DisplayClass::SpecialGhost;
      }

      // The predator (cloaking) effect samples the screen at an offset that
      // walks with the frame counter, which is what makes it shimmer. Objects
      // on the right half of the window walk it the other way, so that two
      // cloaked objects side by side do not ripple in lockstep.
      int pred_offset = static_cast<int>(Frame);

      if (x > WindowList[window][WINDOWWIDTH] << 2) {
        pred_offset = -pred_offset;
      }

      if (draw_window.Lock()) {
        constexpr auto kGhostFadeMask = SHAPE_GHOST | SHAPE_FADING;
        const auto ghost_fade_bits = flags & kGhostFadeMask;
        if (ghost_fade_bits == kGhostFadeMask) {
          Buffer_Frame_To_Page(x, y, width, height, buffer, draw_window,
                               flags | SHAPE_TRANS, ghostdata, fading_data, 1,
                               pred_offset);
        } else {
          if (flags & SHAPE_FADING) {
            Buffer_Frame_To_Page(x, y, width, height, buffer, draw_window,
                                 flags | SHAPE_TRANS, fading_data, 1,
                                 pred_offset);
          } else {
            if (flags & SHAPE_PREDATOR) {
              Buffer_Frame_To_Page(x, y, width, height, buffer, draw_window,
                                   flags | SHAPE_TRANS, pred_offset);
            } else {
              Buffer_Frame_To_Page(x, y, width, height, buffer, draw_window,
                                   flags | SHAPE_TRANS, ghostdata, pred_offset);
            }
          }
        }
        draw_window.Unlock();
      }
    }
  }
}

void CC_Draw_Shape(const std::span<const std::byte> shapefile,
                   const int shape_num, const int x, const int y,
                   const WindowNumberType window, const ShapeFlags_Type flags,
                   const void* fading_data, const void* ghostdata,
                   const DirType rotation, const long scale) {
  CC_Draw_Shape(shapefile.data(), shape_num, x, y, window, flags, fading_data,
                ghostdata, rotation, scale);
}

Rect Shape_Dimensions(const void* shapedata, const int shape_num) {
  Rect rect;

  if (shapedata == nullptr || shape_num < 0 ||
      shape_num > Get_Build_Frame_Count(shapedata)) {
    return rect;
  }

  void* sh =
      Build_Frame(shapedata, static_cast<uint16_t>(shape_num), _ShapeBuffer);
  if (sh == nullptr) {
    return rect;
  }
  const char* shape = static_cast<char*>(Get_Shape_Header_Data(sh));

  int width = Get_Build_Frame_Width(shapedata);
  int height = Get_Build_Frame_Height(shapedata);

  // Four scans, one per edge, each narrowing the search area for the next:
  // the top scan also gives a first guess at the left edge, the bottom scan
  // hands the right scan a starting column, and so on.
  rect.X = 0;
  rect.Y = 0;
  int x_limit = width - 1;
  int y_limit = height - 1;

  // Find top edge of the shape.
  for (int y = 0; y <= y_limit; y++) {
    for (int x = 0; x <= x_limit; x++) {
      if (shape[y * width + x] != 0) {
        rect.Y = y;
        rect.X = x;
        // Pushing y past the limit breaks the outer loop too -- the first row
        // holding any pixel is the top edge, so there is nothing left to scan.
        y = y_limit + 1;
        break;
      }
    }
  }

  // Find bottom edge of the shape.
  for (int y = y_limit; y >= rect.Y; y--) {
    for (int x = x_limit; x >= 0; x--) {
      if (shape[y * width + x] != 0) {
        rect.Height = y - rect.Y + 1;
        x_limit = x;
        y = rect.Y - 1;
        break;
      }
    }
  }

  // Find left edge of the shape.
  for (int x = 0; x < rect.X; x++) {
    for (int y = rect.Y; y < rect.Y + rect.Height; y++) {
      if (shape[y * width + x] != 0) {
        rect.X = x;
        x = rect.X;
        break;
      }
    }
  }

  // Find the right edge of the shape.
  for (int x = width - 1; x >= x_limit; x--) {
    for (int y = rect.Y; y < rect.Y + rect.Height; y++) {
      if (shape[y * width + x] != 0) {
        rect.Width = x - rect.X + 1;
        x = x_limit - 1;
        break;
      }
    }
  }

  // Normalize the rectangle around the center of the shape.
  rect.X -= width / 2;
  rect.Y -= height / 2;

  // Return with the minimum rectangle that encloses the shape.
  return rect;
}

const TechnoTypeClass* Fetch_Techno_Type(const RTTIType type, const int id) {
  switch (type) {
    case RTTI_UNITTYPE:
    case RTTI_UNIT:
      return &UnitTypeClass::As_Reference(static_cast<UnitType>(id));

    case RTTI_VESSELTYPE:
    case RTTI_VESSEL:
      return &VesselTypeClass::As_Reference(static_cast<VesselType>(id));

    case RTTI_BUILDINGTYPE:
    case RTTI_BUILDING:
      return &BuildingTypeClass::As_Reference(static_cast<StructType>(id));

    case RTTI_INFANTRYTYPE:
    case RTTI_INFANTRY:
      return &InfantryTypeClass::As_Reference(static_cast<InfantryType>(id));

    case RTTI_AIRCRAFTTYPE:
    case RTTI_AIRCRAFT:
      return &AircraftTypeClass::As_Reference(static_cast<AircraftType>(id));

    // Everything else either has no TechnoTypeClass or is not a type at all.
    case RTTI_NONE:
    case RTTI_ANIM:
    case RTTI_ANIMTYPE:
    case RTTI_BULLET:
    case RTTI_BULLETTYPE:
    case RTTI_CELL:
    case RTTI_FACTORY:
    case RTTI_HOUSE:
    case RTTI_HOUSETYPE:
    case RTTI_OVERLAY:
    case RTTI_OVERLAYTYPE:
    case RTTI_SMUDGE:
    case RTTI_SMUDGETYPE:
    case RTTI_SPECIAL:
    case RTTI_TEAM:
    case RTTI_TEAMTYPE:
    case RTTI_TEMPLATE:
    case RTTI_TEMPLATETYPE:
    case RTTI_TERRAIN:
    case RTTI_TERRAINTYPE:
    case RTTI_TRIGGER:
    case RTTI_TRIGGERTYPE:
    case RTTI_COUNT:
    default:
      break;
  }
  return nullptr;
}

long VQ_Call_Back(unsigned char*, long) {
  int key = 0;
  if (Keyboard->Check()) {
    key = Keyboard->Get();
    Keyboard->Clear();
  }
  Check_VQ_Palette_Set();
  if (IsVQ640) {
    VQ640.Blit(SeenBuff);
  } else {
    Interpolate_2X_Scale(&SysMemPage, &SeenBuff, nullptr);
  }
  // Call_Back() is deliberately not invoked here. The VQA player drives audio
  // itself while a movie runs, and the game logic it would service is stopped.

  if ((BreakoutAllowed || Debug_Flag) && key == KN_ESC) {
    Keyboard->Clear();
    Brokeout = true;
    return true;
  }

  if (!GameInFocus) {
    VQA_PauseAudio();
    while (!GameInFocus) {
      Check_For_Focus_Loss();
    }
  }
  Video_End_Frame();
  return false;
}

long VQ_Event_Handler(const unsigned long event, void* /*buffer*/,
                      long /*n_bytes*/) {
  // vsync while waiting for frame
  if (event == VQAEVENT_SYNC) {
    Video_End_Frame();
  }
  return 0;
}

// Handles the player's numbered unit groups. See the declaration in conquer.h.
//
// AllowVoice is cleared once something has been selected so that picking a
// group of ten units produces one acknowledgement rather than ten.
void Handle_Team(const int team, const int action) {
  int index;

  // Recording support
  if (Session.Record) {
    TeamNumber = static_cast<char>(team);
    TeamEvent = static_cast<char>(action + 1);
  }

  AllowVoice = true;
  switch (action) {
    // Toggle the team selection. If the team is selected, then merely unselect
    // it. If the team is not selected, then unselect all others before
    // selecting this team.
    case 3:
    case 0:

      // If a non team member is currently selected, then deselect all
      // objects before selecting this team.
      if (CurrentObject.Count()) {
        if (CurrentObject[0]->Is_Foot() &&
            dynamic_cast<FootClass*>(CurrentObject[0])->Group != team) {
          Unselect_All();
        }
      }
      for (index = 0; index < Vessels.Count(); index++) {
        VesselClass* obj = Vessels.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->Group == team &&
            obj->House->IsPlayerControl) {
          if (!obj->IsSelected) {
            obj->Select();
            AllowVoice = false;
          }
        }
      }
      for (index = 0; index < Units.Count(); index++) {
        UnitClass* obj = Units.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->Group == team &&
            obj->House->IsPlayerControl) {
          if (!obj->IsSelected) {
            obj->Select();
            AllowVoice = false;
          }
        }
      }
      for (index = 0; index < Infantry.Count(); index++) {
        InfantryClass* obj = Infantry.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->Group == team &&
            obj->House->IsPlayerControl) {
          if (!obj->IsSelected) {
            obj->Select();
            AllowVoice = false;
          }
        }
      }
      for (index = 0; index < Aircraft.Count(); index++) {
        AircraftClass* obj = Aircraft.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->Group == team &&
            obj->House->IsPlayerControl) {
          if (!obj->IsSelected) {
            obj->Select();
            AllowVoice = false;
          }
        }
      }

      // Center the map around the team if the ALT key was pressed too.
      if (action == 3) {
        Map.Center_Map();
        Map.Flag_To_Redraw(true);
      }
      break;

    // Additive selection of team.
    case 1:
      for (index = 0; index < Units.Count(); index++) {
        UnitClass* obj = Units.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->Group == team &&
            obj->House->IsPlayerControl) {
          if (!obj->IsSelected) {
            obj->Select();
            AllowVoice = false;
          }
        }
      }
      for (index = 0; index < Vessels.Count(); index++) {
        VesselClass* obj = Vessels.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->Group == team &&
            obj->House->IsPlayerControl) {
          if (!obj->IsSelected) {
            obj->Select();
            AllowVoice = false;
          }
        }
      }
      for (index = 0; index < Infantry.Count(); index++) {
        InfantryClass* obj = Infantry.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->Group == team &&
            obj->House->IsPlayerControl) {
          if (!obj->IsSelected) {
            obj->Select();
            AllowVoice = false;
          }
        }
      }
      for (index = 0; index < Aircraft.Count(); index++) {
        AircraftClass* obj = Aircraft.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->Group == team &&
            obj->House->IsPlayerControl) {
          if (!obj->IsSelected) {
            obj->Select();
            AllowVoice = false;
          }
        }
      }
      break;

    // Create the team.
    case 2: {
      // Seeded inverted so the first member examined replaces both bounds.
      long minx = 0x7FFFFFFFL, miny = 0x7FFFFFFFL;
      long maxx = 0, maxy = 0;
      TeamSpeed[team] = SPEED_WHEEL;
      TeamMaxSpeed[team] = MPH_LIGHT_SPEED;
      for (index = 0; index < Units.Count(); index++) {
        UnitClass* obj = Units.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House->IsPlayerControl) {
          if (obj->Group == team) {
            obj->Group = kNoGroup;
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
            obj->Mark(MARK_CHANGE);
            long xc = Cell_X(Coord_Cell(obj->Center_Coord()));
            long yc = Cell_Y(Coord_Cell(obj->Center_Coord()));
            minx = std::min(xc, minx);
            maxx = std::max(xc, maxx);
            miny = std::min(yc, miny);
            maxy = std::max(yc, maxy);
            if (obj->Class->MaxSpeed < TeamMaxSpeed[team]) {
              TeamMaxSpeed[team] = obj->Class->MaxSpeed;
              TeamSpeed[team] = obj->Class->Speed;
            }
          }
        }
      }

      for (index = 0; index < Vessels.Count(); index++) {
        VesselClass* obj = Vessels.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House->IsPlayerControl) {
          if (obj->Group == team) {
            obj->Group = kNoGroup;
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
            obj->Mark(MARK_CHANGE);
            long xc = Cell_X(Coord_Cell(obj->Center_Coord()));
            long yc = Cell_Y(Coord_Cell(obj->Center_Coord()));
            minx = std::min(xc, minx);
            maxx = std::max(xc, maxx);
            miny = std::min(yc, miny);
            maxy = std::max(yc, maxy);
            if (obj->Class->MaxSpeed < TeamMaxSpeed[team]) {
              TeamMaxSpeed[team] = obj->Class->MaxSpeed;
              TeamSpeed[team] = obj->Class->Speed;
            }
          }
        }
      }

      for (index = 0; index < Infantry.Count(); index++) {
        InfantryClass* obj = Infantry.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House->IsPlayerControl) {
          if (obj->Group == team) {
            obj->Group = kNoGroup;
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
            obj->Mark(MARK_CHANGE);
            long xc = Cell_X(Coord_Cell(obj->Center_Coord()));
            long yc = Cell_Y(Coord_Cell(obj->Center_Coord()));
            minx = std::min(xc, minx);
            maxx = std::max(xc, maxx);
            miny = std::min(yc, miny);
            maxy = std::max(yc, maxy);
            TeamMaxSpeed[team] =
                std::min(obj->Class->MaxSpeed, TeamMaxSpeed[team]);
          }
        }
      }
      for (index = 0; index < Aircraft.Count(); index++) {
        AircraftClass* obj = Aircraft.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House->IsPlayerControl) {
          if (obj->Group == team) {
            obj->Group = kNoGroup;
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
            obj->Mark(MARK_CHANGE);
          }
        }
      }

      for (index = 0; index < Units.Count(); index++) {
        UnitClass* obj = Units.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House->IsPlayerControl &&
            obj->Group == team && obj->IsSelected) {
          // When a team is first created, they're created without a
          // formation offset, so they will not be created in
          // formation.  Later, if they're assigned a formation, the
          // XFormOffset & YFormOffset numbers will change to valid
          // offsets, and they'll move in formation.
          obj->XFormOffset = obj->YFormOffset = kNoFormationOffset;
        }
      }

      for (index = 0; index < Infantry.Count(); index++) {
        InfantryClass* obj = Infantry.Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House->IsPlayerControl) {
          if (obj->Group == team) {
            obj->Group = kNoGroup;
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
          }
          if (obj->Group == team && obj->IsSelected) {
            obj->XFormOffset = obj->YFormOffset = kNoFormationOffset;
          }
        }
      }
      break;
    }

    default:
      break;
  }
  AllowVoice = true;
}

// Bookmarks are stored biased by half a screen -- eight rows down and ten
// columns across from the tactical corner -- so the cell recorded is the middle
// of what the player was looking at, not its top left corner.
void Handle_View(const int view, const int action) {
  if (static_cast<unsigned>(view) < std::ssize(Scen.Views)) {
    if (action == 0) {
      Map.Set_Tactical_Position(Coord_Whole(Cell_Coord(
          static_cast<CELL>(Scen.Views[view] - MAP_CELL_W * 8 - 10))));

      // Win95 scrolling logic cant handle just jumps in screen position so
      // redraw the lot.
      Map.Flag_To_Redraw(true);
    } else {
      Scen.Views[view] = static_cast<CELL>(Coord_Cell(Map.TacticalCoord) +
                                           MAP_CELL_W * 8 + 10);
    }
  }
}

// There are no removable discs to check: the data is installed on disk, so
// this reports the DVD unconditionally. The original scanned the drive's
// volume label against kCdNames.
int Get_CD_Index(int /*cd_drive*/, int /*timeout*/) {
  return 5;  // we uh, magically have the DVD
}

// Disc identifiers, matching the order of kCdNames below. CD_SOVIET and
// CD_ALLIED are unreferenced by name but fix the numbering the later values
// depend on, and are the values Get_CD_Index() returns for those discs.
namespace {
enum CD_VOLUME {
  CD_LOCAL = -2,
  CD_ANY = -1,
  CD_SOVIET [[maybe_unused]] = 0,
  CD_ALLIED [[maybe_unused]],
  CD_COUNTERSTRIKE,
  CD_AFTERMATH,
  CD_CS_OR_AM,
  CD_DVD,
};

// Index of the DVD's name in kCdNames. The table has no entry for the
// CD_CS_OR_AM request, so the names stop lining up with CD_VOLUME there.
constexpr int kDvdName = 4;
}  // namespace

bool Force_CD_Available(int cd_desired)  // ajw
{
  static int _last = -1;
  static const void* font;
  // Disc names as printed on the localized releases, in the language this
  // build was compiled for.
  static constexpr std::array<const char*, 5> kCdNames = [] {
    if constexpr (config::kBuildLanguage == config::BuildLanguage::French) {
      return std::array{
          "ALERTE ROUGE CD1",   "ALERTE ROUGE CD2", "CD Missions Taiga",
          "CD Missions M.A.D.", "ALERTE ROUGE DVD",
      };
    } else if constexpr (config::kBuildLanguage ==
                         config::BuildLanguage::German) {
      return std::array{
          "ALARMSTUFE ROT CD1",       "ALARMSTUFE ROT CD2",
          "CD Gegenangriff einlegen", "CD TRANS einlegen",
          "ALARMSTUFE ROT DVD",
      };
    } else {
      return std::array{
          "RED ALERT DISK 1", "RED ALERT DISK 2", "CounterStrike CD",
          "Aftermath CD",     "RED ALERT DVD",
      };
    }
  }();

  int new_cd_drive = 0;

  // If the required CD is set to -2 then it means that the file is present
  // on the local hard drive and we shouldn't have to worry about it.
  if (cd_desired == CD_LOCAL) {
    return true;
  }

  // Find out if the CD in the current drive is the one we are looking for
  const int current_drive = CCFileClass::Get_CD_Drive();
  int cd_current = Get_CD_Index(current_drive, 1 * 60);

  if (Using_DVD()) {
    // The DVD release carries every disc's content, so any disc request is
    // satisfied by it.
    cd_desired = CD_DVD;
  }

  if (cd_current >= 0) {
    if (cd_desired == CD_CS_OR_AM) {
      // If the current cd is CS or AM then change request to whatever
      // is present.
      if (cd_current == CD_COUNTERSTRIKE || cd_current == CD_AFTERMATH) {
        cd_desired = cd_current;
      }
    }
    // If the current CD is requested or any CD will work
    if (cd_desired == cd_current || cd_desired == CD_ANY) {
      // The required CD is still in the CD drive we used last time, so the
      // content is already reachable.
      return true;
    }
  }

  // Flag that we will have to restart the theme
  Theme.Stop();

  // Check the last drive
  if (!new_cd_drive) {
    // Check the last CD drive we used if it's different from the current one
    int last_drive = CCFileClass::Get_Last_CD_Drive();

    // Make sure the last drive is valid and it isn't the current drive
    // Skipped when it is the current drive, which the search above already
    // covered.
    if (last_drive && last_drive != CCFileClass::Get_CD_Drive()) {
      // Find out if there is a C&C cd in the last drive and if so is it the one
      // we are looking for
      // Give it a nice big timeout so the CD changer has time to swap the discs
      cd_current = Get_CD_Index(last_drive, 10 * 60);

      if (cd_current >= 0) {
        if (cd_desired == CD_CS_OR_AM) {
          // If the cd is CS or AM then change request to whatever
          // is present.
          if (cd_current == CD_COUNTERSTRIKE || cd_current == CD_AFTERMATH) {
            cd_desired = cd_current;
          }
        }
        // If the cd is present or any cd will work
        if (cd_desired == cd_current || cd_desired == CD_ANY) {
          // The required CD is in the CD drive we used last time
          new_cd_drive = last_drive;
        }
      }
    }
  }

  // Lordy.  No sign of that blimming CD anywhere. Search all the CD drives
  // then if we still can't find it prompt the user to insert it.
  if (!new_cd_drive) {
    // Small timeout for the first pass through the drives
    int drive_search_timeout = 2 * 60;

    for (;;) {
      char buffer[128];
      // Search all present CD drives for the required disc.
      for (int i = 0; i < CDList.Get_Number_Of_Drives(); i++) {
        int cd_drive = CDList.Get_Next_CD_Drive();
        cd_current = Get_CD_Index(cd_drive, drive_search_timeout);

        if (cd_current >= 0) {
          // We found a C&C cd - lets see if it was the one we were looking for
          // Require CS or AM
          if (cd_desired == CD_CS_OR_AM) {
            // If the cd is CS or AM then change request to whatever
            // is present.
            if (cd_current == CD_COUNTERSTRIKE || cd_current == CD_AFTERMATH) {
              cd_desired = cd_current;
            }
          }

          if (cd_desired == cd_current || cd_desired == CD_ANY) {
            // Woohoo! The disk was in a different cd drive. Refresh the search
            // path list and return.
            new_cd_drive = cd_drive;
            break;
          }
        }
      }

      // A new disc has become available so break
      if (new_cd_drive) {
        break;
      }

      // Increase the timeout for subsequent drive searches.
      drive_search_timeout = 5 * 60;

      // Prompt to insert the CD into the drive.
      // V.Grippi
      if (cd_desired == CD_CS_OR_AM) {
        cd_desired = CD_AFTERMATH;
      }

      // The wording is fixed by the language this build was compiled for; only
      // the disc name varies.
      auto insert_prompt = [&buffer](const char* disc_name) {
        if constexpr (config::kBuildLanguage == config::BuildLanguage::French) {
          snprintf(buffer, sizeof(buffer), "Insèrez le %s", disc_name);
        } else if constexpr (config::kBuildLanguage ==
                             config::BuildLanguage::German) {
          snprintf(buffer, sizeof(buffer), "Bitte %s", disc_name);
        } else {
          snprintf(buffer, sizeof(buffer), "Please insert the %s", disc_name);
        }
      };

      if (cd_desired == CD_DVD) {
        insert_prompt(kCdNames[kDvdName]);
      } else if (cd_desired == CD_COUNTERSTRIKE || cd_desired == CD_AFTERMATH) {
        insert_prompt(kCdNames[cd_desired]);
      } else {
        // These prompts come from the localized string table, so verify the
        // translation still takes a %d followed by a %s before using it.
        const int text =
            cd_desired == CD_ANY ? TXT_CD_DIALOG_1 : TXT_CD_DIALOG_2;  // 0 or 1
        auto format = absl::ParsedFormat<'d', 's'>::New(Text_String(text));
        if (format != nullptr) {
          port::SafeCopy(buffer, absl::StrFormat(*format, cd_desired + 1,
                                                 kCdNames[cd_desired])
                                     .c_str());
        }
      }

      GraphicViewPortClass* old_page = Set_Logic_Page(SeenBuff);
      Theme.Stop();
      int hidden = Get_Mouse_State();
      font = FontPtr;

      // Only set the palette if necessary.
      if (PaletteClass::CurrentPalette[1].Red_Component() +
              PaletteClass::CurrentPalette[1].Blue_Component() +
              PaletteClass::CurrentPalette[1].Green_Component() ==
          0) {
        GamePalette.Set();
      }

      Keyboard->Clear();

      while (Get_Mouse_State()) {
        Show_Mouse();
      }

      if (WWMessageBox().Process(buffer, TXT_OK, TXT_CANCEL, TXT_NONE, true) ==
          1) {
        Set_Logic_Page(old_page);
        while (hidden--) {
          Hide_Mouse();
        }
        return false;
      }

      while (hidden--) {
        Hide_Mouse();
      }
      Set_Font(font);
      Set_Logic_Page(old_page);
    }
  }

  CurrentCD = cd_current;

  CCFileClass::Set_CD_Drive(new_cd_drive);
  CCFileClass::Refresh_Search_Drives();

  // If it broke out of the query for CD-ROM loop, then this means that the
  // CD-ROM has been inserted.
  // CD_CS_OR_AM is a request, not a disc that exists; narrow it to Aftermath
  // now that a real disc has been found, so the cache check below compares
  // like with like.
  if (cd_desired == 4) {
    cd_desired--;
  }

  // Re-register the secondary mix files from the disc that was just found,
  // but only when the disc actually changed.
  //
  // The cd_desired != CD_DVD condition is ajw's: on the DVD build this ran
  // before Init_Secondary_Mixfiles() and corrupted the mixfile system. Skipping
  // it there is safe, because the DVD is the only disc that can ever be
  // requested when Using_DVD(), and cd_desired can never be CD_DVD otherwise.
  if (cd_desired > -1 && _last != cd_desired && cd_desired != 5) {
    _last = cd_desired;

    Theme.Stop();

    delete MoviesMix;
    delete GeneralMix;
    delete ScoreMix;
    delete MainMix;

    MainMix = MFCD::Register("MAIN.MIX", &FastKey, &CryptRandom);
    assert(MainMix != nullptr);
    if (CCFileClass("MOVIES1.MIX").Is_Available()) {
      MoviesMix = MFCD::Register("MOVIES1.MIX", &FastKey, &CryptRandom);
    } else {
      MoviesMix = MFCD::Register("MOVIES2.MIX", &FastKey, &CryptRandom);
    }
    assert(MoviesMix != nullptr);
    GeneralMix = MFCD::Register("GENERAL.MIX", &FastKey, &CryptRandom);
    ScoreMix = MFCD::Register("SCORES.MIX", &FastKey, &CryptRandom);
    ThemeClass::Scan();
  }

  return true;
}

void* Hires_Load(char* name) {
  char filename[30];

  sprintf(filename, "H%s", name);
  CCFileClass file(filename);

  if (file.Is_Available()) {
    const int length = static_cast<int>(file.Size());
    void* return_ptr = new char[length];
    file.Read(return_ptr, length);
    return return_ptr;
  }
  return nullptr;
}

CrateType Crate_From_Name(const char* name) {
  if (name != nullptr) {
    for (CrateType crate = CRATE_FIRST; crate < CRATE_COUNT; ++crate) {
      if (stricmp(name, CrateNames[crate]) == 0) {
        return crate;
      }
    }
  }
  return CRATE_MONEY;
}

int Owner_From_Name(const char* text) {
  // Accumulated unsigned: this is a bit pattern, and the house masks reach
  // the sign bit once enough houses are set.
  unsigned ownable = 0;
  if (stricmp(text, "soviet") == 0) {
    ownable |= static_cast<unsigned>(kHouseFlagSoviet);
  } else {
    if (stricmp(text, "allies") == 0 || stricmp(text, "allied") == 0) {
      ownable |= static_cast<unsigned>(kHouseFlagAllies);
    } else {
      HousesType h = HouseTypeClass::From_Name(text);
      if (h != HOUSE_NONE && (h < HOUSE_MULTI1 || h > HOUSE_MULTI8)) {
        ownable |= 1U << static_cast<unsigned>(h);
      }
    }
  }
  return static_cast<int>(ownable);
}

// Copies the screen two pixels up or two pixels down, one step per game tick,
// for twice the requested number of shakes.
void Shake_The_Screen(int shakes) {
  shakes += shakes;

  Hide_Mouse();
  SeenBuff.Blit(HidPage);
  // TODO: old_y_off is never reassigned, so the reject test below only ever
  // rejects 0. The offset is therefore always +/-2 and never centre, which
  // makes the new_y_off == 0 case unreachable. Intent was presumably to reject
  // a repeat of the previous offset.
  int new_y_off = 0;
  while (shakes-- != 0) {
    constexpr int old_y_off = 0;
    // Hold each offset for exactly one tick, so the shake runs at game speed
    // rather than as fast as the machine can blit.
    int64_t x = TickCount.Value();
    do {
      new_y_off = Sim_Random_Pick(0, 2) - 1;
    } while (new_y_off == old_y_off);
    switch (new_y_off) {
      case -1:
        HidPage.Blit(SeenBuff, 0, 2, 0, 0, 640, 398);
        break;
      case 1:
        HidPage.Blit(SeenBuff, 0, 0, 0, 2, 640, 398);
        break;
      default:
        HidPage.Blit(SeenBuff);
        break;
    }
    while (x == TickCount.Value()) {
      Video_End_Frame();
    }
  }
  HidPage.Blit(SeenBuff);
  Show_Mouse();
}

void List_Copy(const short* source, int len, short* dest) {
  if (source == nullptr || dest == nullptr) {
    return;
  }

  while (len > 0) {
    *dest = *source;
    if (*dest == kRefreshEol) {
      break;
    }
    dest++;
    source++;
    len--;
  }
}

// Returns the registry subkey, under HKEY_LOCAL_MACHINE, that the Windows
// installer wrote this game's settings to. The key name is language specific
// because each localized release installed as a separate product.
const char* Game_Registry_Key() {
  if constexpr (config::kBuildLanguage == config::BuildLanguage::French) {
    return "SOFTWARE\\Westwood\\Alerte Rouge version Windows 95";
  } else if constexpr (config::kBuildLanguage ==
                       config::BuildLanguage::German) {
    return "SOFTWARE\\Westwood\\Alarmstufe Rot Windows 95 Edition";
  } else {
    return "SOFTWARE\\Westwood\\Red Alert Windows 95 Edition";
  }
}

bool ReadInstallerFlag(const char* value_name) {
  return port::ReadRegistryDword(HKEY_LOCAL_MACHINE, Game_Registry_Key(),
                                 value_name)
             .value_or(0) != 0;
}

// Reports whether the Counterstrike expansion is installed, by reading the flag
// the installer left in the registry. The answer is cached: it cannot change
// while the game is running.
//
// Off Windows there is no registry and no separate expansion installer, so the
// content is simply assumed to be present. Before the installer wrote a
// registry flag, this was decided by probing for EXPAND.MIX.
bool Is_Counterstrike_Installed() {
  if constexpr (port::kIsWindows) {
    static const bool installed = ReadInstallerFlag("CStrikeInstalled");
    return installed;
  } else {
    return true;
  }
}

// Reports whether the Aftermath expansion is installed. See
// Is_Counterstrike_Installed() above; this works the same way, and the probe
// it replaced was for EXPAND2.MIX.
bool Is_Aftermath_Installed() {
  if constexpr (port::kIsWindows) {
    static const bool installed = ReadInstallerFlag("AftermathInstalled");
    return installed;
  } else {
    return true;
  }
}

// The body -- set SecretUnitsEnabled, drop the phase tank and the helicarrier
// to tech level 10, refresh every building's buildables -- was disabled before
// release. The chat trigger is still recognized so the message is relayed the
// same way on every machine.
void Enable_Secret_Units() {}

bool Force_Scenario_Available(const char* name) {
  // Calls Force_CD_Available based on type of scenario. szName is assumed to
  // be an official scenario here.
  if (IsMissionCounterstrike(name)) {
    return Force_CD_Available(4);
  }
  if (IsMissionAftermath(name)) {
    return Force_CD_Available(3);
  }
  return true;
}
