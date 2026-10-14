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

/* $Header: /counterstrike/SESSION.CPP 3     3/10/97 6:23p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SESSION.CPP *
 *                                                                                             *
 *                   Programmer : Bill R. Randolph *
 *                                                                                             *
 *                   Start Date : 11/30/95 *
 *                                                                                             *
 *                  Last Update : September 10, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * SessionClass::SessionClass -- Constructor *
 *   SessionClass::~SessionClass -- Destructor * SessionClass::One_Time --
 *one-time initializations                                        *
 *   SessionClass::Init -- Initializes all values *
 *   SessionClass::Create_Connections -- forms connections to other players *
 *   SessionClass::Am_I_Master -- tells if the local system is the "master" *
 *   SessionClass::Save -- Saves this class to a file * SessionClass::Load --
 *Loads this class from a file                                        *
 *   SessionClass::Read_MultiPlayer_Settings -- reads settings from INI *
 *   SessionClass::Write_MultiPlayer_Settings -- writes settings to INI *
 *   SessionClass::Read_Scenario_Descriptions -- reads scen. descriptions *
 *   SessionClass::Free_Scenario_Descriptions -- frees scen. descriptions *
 *   SessionClass::Trap_Object -- searches for an object, for debugging *
 *   SessionClass::Compute_Unique_ID -- computes unique local ID number *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/net/session.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>  // for station ID computation
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/enum_array.h"
#include "base/strings/number_parse.h"
#include "base/strings/safe_string.h"
#include "base/strings/tokenizer.h"
#include "base/types.h"
#include "engine/file/disk_file.h"
#include "engine/file/file_access.h"
#include "engine/file/game_file.h"
#include "engine/file/ini_file.h"
#include "engine/net/ipxmgr.h"
#include "engine/platform/env.h"
#include "engine/platform/file_system.h"
#include "engine/stream/archive.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/byte_source.h"
#include "game/core/object_heap.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/assets/installation.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/data/mission_id.h"
#include "ra/map/map.h"
#include "ra/map/world.h"
#include "ra/net/network.h"
#include "ra/net/queue.h"
#include "ra/objects/aircraft.h"
#include "ra/objects/anim.h"
#include "ra/objects/building.h"
#include "ra/objects/bullet.h"
#include "ra/objects/house.h"
#include "ra/objects/infantry.h"
#include "ra/objects/unit.h"

namespace ra {

// #include "WolDebug.h"

/***************************** Globals *************************************/
//---------------------------------------------------------------------------
//	This is the array of remap colors.  Each player in a network game is
// assigned one of these colors.  The 'G' is for graphics drawing; the 'T'
// is for text printing (indicates a remap table for the font to use).
//---------------------------------------------------------------------------
// int SessionClass::GColors[MAX_MPLAYER_COLORS] = {
// 5, 			// Yellow
// 127, 			// Red
// 135, 			// BlueGreen
// 26,			// Orange
// 4,				// Green
// 202			// Blue-Grey
//};

// int SessionClass::TColors[MAX_MPLAYER_COLORS] = {
// CC_GDI_COLOR, 			// Yellow
// CC_NOD_COLOR, 			// Red
// CC_BLUE_GREEN, 		// BlueGreen
// CC_ORANGE,				// Orange
// CC_GREEN,				// Green
// CC_BLUE_GREY,			// Blue
//};

/*---------------------------------------------------------------------------
Min & Max unit count values; index0 = bases OFF, index1 = bases ON
---------------------------------------------------------------------------*/
int SessionClass::CountMin[2] = {1, 0};
int SessionClass::CountMax[2] = {50, 12};

//---------------------------------------------------------------------------
//	This is a list of all the names of the multiplayer scenarios
//---------------------------------------------------------------------------
char SessionClass::Descriptions[100][40];

base::EnumArray<DialMethodType, const char*, static_cast<int>(DIAL_METHODS)>
    SessionClass::DialMethodCheck = {"T", "P"};

const char* SessionClass::CallWaitStrings[kCallWaitStringsNum] = {
    "*70,", "70#,", "1170,", "CUSTOM -                "};

/***************************************************************************
 * SessionClass::SessionClass -- Constructor                               *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   11/30/1995 BRR : Created.                                             *
 *=========================================================================*/
SessionClass::SessionClass()
    : PrefColor(magic_enum::enum_values<PlayerColorType>().front()),
      ColorIdx(magic_enum::enum_values<PlayerColorType>().front()),
      MetaSize(MAX_IPX_PACKET_SIZE) {
  Options.ScenarioIndex = 0;
  Options.Bases = 0;
  Options.Credits = 0;
  Options.Tiberium = 0;
  Options.Goodies = 0;
  Options.Ghosts = 0;
  Options.UnitCount = 0;

  Handle[0] = 0;

  LastMessage[0] = 0;

  GameName[0] = 0;

  SerialDefaults.Port = 0x2f8;                  // set from INI file
  SerialDefaults.IRQ = 3;                       // set from INI file
  SerialDefaults.Baud = 9600;                   // set from INI file
  SerialDefaults.DialMethod = DIAL_TOUCH_TONE;  // set from INI file
  SerialDefaults.InitStringIndex = 0;           // set from INI file
  SerialDefaults.CallWaitStringIndex = 0;       // set from INI file
  base::SafeCopy(SerialDefaults.CallWaitString, "");

}  // end of SessionClass

/***************************************************************************
 * SessionClass::~SessionClass -- Destructor                               *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   11/30/1995 BRR : Created.                                             *
 *=========================================================================*/
SessionClass::~SessionClass() {
  Free_Scenario_Descriptions();
}  // end of ~SessionClass

/***************************************************************************
 * SessionClass::One_Time -- one-time initializations                      *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/01/1995 BRR : Created.                                             *
 *=========================================================================*/
void SessionClass::One_Time() {
  Read_MultiPlayer_Settings();
  Read_Scenario_Descriptions();

  UniqueID = static_cast<int>(Compute_Unique_ID());

}  // end of One_Time

/***************************************************************************
 * SessionClass::Init -- Initializes all values                            *
 *                                                                         *
 * This function should be called for every new game played; it only sets
 ** those variables that should be set for a new game.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   11/30/1995 BRR : Created.                                             *
 *=========================================================================*/
void SessionClass::Init() {}  // end of Init

/***************************************************************************
 * SessionClass::Create_Connections -- forms connections to other players  *
 *                                                                         *
 * This routine uses the contents of the Players vector, combined with * that of
 *the Houses array, to create connections to each other player.	* It is assumed
 *that 'Players' contains all the other players to connect	* to, and that
 *the HouseClass's have been filled in with players' data.	*
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = success, 0 = failure
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   11/30/1995 BRR : Created.                                             *
 *=========================================================================*/
int SessionClass::Create_Connections() {
  if (TheSession().Type != GAME_IPX && TheSession().Type != GAME_INTERNET) {
    return 0;
  }

  //------------------------------------------------------------------------
  // Loop through all entries in 'Players'.  To avoid connecting to myself,
  // skip the 1st entry.
  //------------------------------------------------------------------------
  for (int i = 1; i < Players.Count(); i++) {
    //.....................................................................
    // Make sure the name matches before creating the connection
    //.....................................................................
    if (absl::EqualsIgnoreCase(
            Players.at(i)->Name,
            HouseClass::As_Pointer(Players.at(i)->Player.ID)->IniName)) {
      TheNetwork().ipx().Create_Connection(
          static_cast<int>(Players.at(i)->Player.ID), Players.at(i)->Name,
          &Players.at(i)->Address);
      Players.at(i)->Player.ProcessTime = -1;
    } else {
      return 0;
    }
  }

  return 1;

}  // end of Create_Connections

/***************************************************************************
 * SessionClass::Am_I_Master -- tells if the local system is the "master"  *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   11/29/1995 BRR : Created.                                             *
 *=========================================================================*/
bool SessionClass::Am_I_Master() {

  //------------------------------------------------------------------------
  // Check every house; if PlayerPtr points to the first human house, we're
  // the master.
  //------------------------------------------------------------------------
  for (int i = 0; i < TheSession().MaxPlayers; i++) {
    const auto house =
        static_cast<HousesType>(static_cast<int>(HOUSE_MULTI1) + i);
    HouseClass* hptr = HouseClass::As_Pointer(house);
    if (hptr->IsHuman) {
      return ThePlayer() == hptr;
    }
  }

  return false;

}  // end of Am_I_Master

/***************************************************************************
 * SessionClass::Save -- Saves this class to a file                        *
 *                                                                         *
 * Only certain members of this class should be saved into a save-game * file;
 *this routine saves only those members.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		file		file to save to
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/04/1995 BRR : Created.                                             *
 *=========================================================================*/
template <class Archive>
void SessionClass::Serialize(Archive& ar) {
  ar(Type, CommProtocol, MaxAhead, FrameSendRate, DesiredFrameRate,
     PrefColor, ColorIdx, House, NumPlayers, Options.Bases, Options.Credits,
     Options.Tiberium, Options.Goodies, Options.Ghosts, Options.UnitCount,
     Options.AIPlayers, ObiWan, EmergencySave);
  if constexpr (Archive::kIsReading) {
    if (!magic_enum::enum_contains(Type) || NumPlayers < 0 ||
        NumPlayers > MAX_MULTI_NAMES) {
      ar.Fail("invalid saved session");
    }
  }
}
template void SessionClass::Serialize(ArchiveWriter&);
template void SessionClass::Serialize(ArchiveReader&);

template <class Archive>
void SessionClass::SerializePlayers(Archive& ar) {
  auto count = static_cast<int32_t>(Players.Count());
  ar(count);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || count < 0 || count > MAX_MULTI_NAMES) {
      ar.Fail("invalid recording player count");
      return;
    }
    Players.Clear();
    for (int i = 0; i < count; ++i) {
      auto node = std::make_unique<NodeNameType>();
      ar(*node);
      if (!ar.ok()) {
        return;
      }
      Players.Add(std::move(node));
    }
  } else {
    for (int i = 0; i < count; ++i) {
      ar(*Players.at(i));
    }
  }
}
template void SessionClass::SerializePlayers(ArchiveWriter&);
template void SessionClass::SerializePlayers(ArchiveReader&);

int SessionClass::Save(ByteSink& file) {
  ArchiveWriter writer(file);
  Serialize(writer);
  return 1;
}  // end of Save

/***************************************************************************
 * SessionClass::Load -- Loads this class from a file                      *
 *                                                                         *
 * INPUT:                                                                  *
 *		file		file to load from
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/04/1995 BRR : Created.                                             *
 *=========================================================================*/
bool SessionClass::Load(ByteSource& file) {
  ArchiveReader reader(file);
  Serialize(reader);
  return reader.ok();
}  // end of Load

/***************************************************************************
 * SessionClass::Read_MultiPlayer_Settings -- reads settings INI           *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   02/14/1995 BR : Created.                                              *
 *=========================================================================*/
void SessionClass::Read_MultiPlayer_Settings() {
  char buf[128];           // buffer for parsing INI entry
  int i = 0;

  //	GameFile file (kConfigFileName);

  //------------------------------------------------------------------------
  //	Clear the initstring entries
  //------------------------------------------------------------------------
  InitStrings.Clear();

  //	Clear the dialing entries
  PhoneBook.Clear();

  //	Create filename and read the file.
  engine::file::IniFile ini;
  if (const auto fc = OpenDiskFile(kConfigFileName); fc && ini.Load(*fc)) {
    //	Get the player's last-used Handle
    base::SafeCopy(Handle, ini.GetString("MultiPlayer", "Handle", "Noname"));

    //	Get the player's last-used Color
    PrefColor =
        static_cast<PlayerColorType>(ini.GetInt("MultiPlayer", "Color", 0));
    int iSide = ini.GetInt("MultiPlayer", "Side", static_cast<int>(HOUSE_USSR));
    iSide = std::max(2, std::min(6, iSide));
    House = static_cast<HousesType>(iSide);
    CurPhoneIdx = ini.GetInt("MultiPlayer", "PhoneIndex", -1);

    //	Read in default serial settings
    base::SafeCopy(SerialDefaults.ModemName,
                   ini.GetString("SerialDefaults", "ModemName", "NoName"));
    if ((std::string_view(SerialDefaults.ModemName) == "NoName")) {
      SerialDefaults.ModemName[0] = 0;
    }
    SerialDefaults.Port = ini.GetInt("SerialDefaults", "Port", 0);
    SerialDefaults.IRQ = ini.GetInt("SerialDefaults", "IRQ", -1);
    SerialDefaults.Baud = ini.GetInt("SerialDefaults", "Baud", -1);
    SerialDefaults.Compression =
        ini.GetInt("SerialDefaults", "Compression", 0) != 0;
    SerialDefaults.ErrorCorrection =
        ini.GetInt("SerialDefaults", "ErrorCorrection", 0) != 0;
    SerialDefaults.HardwareFlowControl =
        ini.GetInt("SerialDefaults", "HardwareFlowControl", 1) != 0;

    // Only the first character names the method.
    const std::string dial_method =
        ini.GetString("SerialDefaults", "DialMethod", "T").substr(0, 1);

    // find dial method
    for (i = 0; i < static_cast<int>(DIAL_METHODS); i++) {
      if (absl::EqualsIgnoreCase(
              dial_method,
              DialMethodCheck.at(static_cast<DialMethodType>(i)))) {
        SerialDefaults.DialMethod = static_cast<DialMethodType>(i);
        break;
      }
    }

    // if method not found set to touch tone
    if (i == static_cast<int>(DIAL_METHODS)) {
      SerialDefaults.DialMethod = DIAL_TOUCH_TONE;
    }

    SerialDefaults.InitStringIndex =
        ini.GetInt("SerialDefaults", "InitStringIndex", 0);

    SerialDefaults.CallWaitStringIndex =
        ini.GetInt("SerialDefaults", "CallWaitStringIndex", kCallWaitCustom);

    base::SafeCopy(SerialDefaults.CallWaitString,
                   ini.GetString("SerialDefaults", "CallWaitString", ""));

    if (SerialDefaults.IRQ == 0 || SerialDefaults.Baud == 0) {
      SerialDefaults.Port = 0;
      SerialDefaults.IRQ = -1;
      SerialDefaults.Baud = -1;
    }

    const std::vector<engine::file::IniEntry> init_strings =
        ini.Entries("InitStrings");
    for (const engine::file::IniEntry& init : init_strings) {
      auto entry = std::make_unique<InitString>();
      base::SafeCopy(*entry, init.value);
      std::ranges::transform(base::MutableCString(entry->data()), entry->data(),
                             absl::ascii_toupper);
      InitStrings.Add(std::move(entry));
    }

    //	if no entries then have at least one
    if (init_strings.empty()) {
      auto entry = std::make_unique<InitString>();
      base::SafeCopy(*entry, "ATZ");
      InitStrings.Add(std::move(entry));
      SerialDefaults.InitStringIndex = 0;
    }

    //	Read the entry names in
    for (const engine::file::IniEntry& listing : ini.Entries("PhoneBook")) {
      //	Create a new phone book entry
      auto phone = std::make_unique<PhoneEntryClass>();  // a phone book entry

      //	Read the entire entry in
      const std::string_view phone_text = listing.value;

      //	Extract name, phone # & serial port settings
      base::Tokenizer tokens(phone_text, "|");
      std::string_view tokenptr = tokens.Next();
      if (!tokenptr.empty()) {
        base::SafeCopy(phone->Name, tokenptr);
        std::ranges::transform(base::MutableCString(phone->Name), phone->Name,
                               absl::ascii_toupper);
      } else {
        phone->Name[0] = 0;
      }

      tokenptr = tokens.Next();
      if (!tokenptr.empty()) {
        base::SafeCopy(phone->Number, tokenptr);
        std::ranges::transform(base::MutableCString(phone->Number),
                               phone->Number, absl::ascii_toupper);
      } else {
        phone->Number[0] = 0;
      }

      tokenptr = tokens.Next();
      if (!tokenptr.empty()) {
        if (const auto value = base::ParseHex<int>(tokenptr)) {
          phone->Settings.Port = *value;
        }
      } else {
        phone->Settings.Port = 0;
      }

      tokenptr = tokens.Next();
      if (!tokenptr.empty()) {
        phone->Settings.IRQ = base::ParseIntegerOr<int>(tokenptr, 0);
      } else {
        phone->Settings.IRQ = -1;
      }

      tokenptr = tokens.Next();
      if (!tokenptr.empty()) {
        phone->Settings.Baud = base::ParseIntegerOr<int>(tokenptr, 0);
      } else {
        phone->Settings.Baud = -1;
      }

      phone->Settings.Compression = false;
      phone->Settings.ErrorCorrection = false;
      phone->Settings.HardwareFlowControl = true;

      /*
      ** Find out if this phonebook entry has the new settings included. If not
      ** then we need to skip this section.
      */
      tokenptr = tokens.Next();
      if (!tokenptr.empty()) {
        base::SafeCopy(buf, tokenptr);

        // find dial method

        for (i = 0; i < static_cast<int>(DIAL_METHODS); i++) {
          if (absl::EqualsIgnoreCase(
                  buf, DialMethodCheck.at(static_cast<DialMethodType>(i)))) {
            /*
            ** This must be an old phonebook entry
            */
            break;
          }
        }

        /*
        ** Method wasnt found - assume its a new phonebook entry so get the
        *extra settings
        */
        // if method not found set to touch tone

        if (i == static_cast<int>(DIAL_METHODS)) {
          phone->Settings.Compression =
              base::ParseIntegerOr<int>(tokenptr, 0) != 0;

          tokenptr = tokens.Next();
          if (!tokenptr.empty()) {
            phone->Settings.ErrorCorrection =
                base::ParseIntegerOr<int>(tokenptr, 0) != 0;
          }

          tokenptr = tokens.Next();
          if (!tokenptr.empty()) {
            phone->Settings.HardwareFlowControl =
                base::ParseIntegerOr<int>(tokenptr, 0) != 0;
          }

          tokenptr = tokens.Next();
        }
      }

      if (!tokenptr.empty()) {
        base::SafeCopy(buf, tokenptr);

        //	find dial method
        for (i = 0; i < static_cast<int>(DIAL_METHODS); i++) {
          if (absl::EqualsIgnoreCase(
                  buf, DialMethodCheck.at(static_cast<DialMethodType>(i)))) {
            phone->Settings.DialMethod = static_cast<DialMethodType>(i);
            break;
          }
        }

        //	if method not found set to touch tone
        if (i == static_cast<int>(DIAL_METHODS)) {
          phone->Settings.DialMethod = DIAL_TOUCH_TONE;
        }
      } else {
        phone->Settings.DialMethod = DIAL_TOUCH_TONE;
      }

      tokenptr = tokens.Next();
      if (!tokenptr.empty()) {
        phone->Settings.InitStringIndex =
            base::ParseIntegerOr<int>(tokenptr, 0);
      } else {
        phone->Settings.InitStringIndex = 0;
      }

      tokenptr = tokens.Next();
      if (!tokenptr.empty()) {
        phone->Settings.CallWaitStringIndex =
            base::ParseIntegerOr<int>(tokenptr, 0);
      } else {
        phone->Settings.CallWaitStringIndex = kCallWaitCustom;
      }

      tokenptr = tokens.Next();
      if (!tokenptr.empty()) {
        base::SafeCopy(phone->Settings.CallWaitString, tokenptr);
      } else {
        phone->Settings.CallWaitString[0] = 0;
      }

      //	Add it to our list
      PhoneBook.Add(std::move(phone));
    }

    //	Read special recording playback values, to help find sync bugs
    TrapFrame = ini.GetInt("SyncBug", "Frame", 0x7fffffff);

    const std::string trap_type = ini.GetString("SyncBug", "Type", "NONE");

    if (absl::EqualsIgnoreCase(trap_type, "AIRCRAFT")) {
      TrapObjType = RTTI_AIRCRAFT;
    } else if (absl::EqualsIgnoreCase(trap_type, "ANIM")) {
      TrapObjType = RTTI_ANIM;
    } else if (absl::EqualsIgnoreCase(trap_type, "BUILDING")) {
      TrapObjType = RTTI_BUILDING;
    } else if (absl::EqualsIgnoreCase(trap_type, "BULLET")) {
      TrapObjType = RTTI_BULLET;
    } else if (absl::EqualsIgnoreCase(trap_type, "INFANTRY")) {
      TrapObjType = RTTI_INFANTRY;
    } else if (absl::EqualsIgnoreCase(trap_type, "UNIT")) {
      TrapObjType = RTTI_UNIT;
    } else {
      TrapObjType = RTTI_NONE;
    }

    TrapCoord = Coordinate::FromPacked(
        base::ParseHexOr<uint32_t>(ini.GetString("SyncBug", "Coord", "0"), 0));

    TrapTarget =
        Target::FromPacked(static_cast<int32_t>(base::ParseHexOr<uint32_t>(
            ini.GetString("SyncBug", "Target", "0"), 0)));

    const Cell cell = Cell::FromIndex(base::ParseIntegerOr<int16_t>(
        ini.GetString("SyncBug", "Cell", "0"), 0));
    if (cell.index() != 0) {
      TrapCell = &TheMap().at(cell);
    }

    TrapPrintCRC = ini.GetInt("SyncBug", "PrintCRC", 0x7fffffff);
  }
}

/***************************************************************************
 * SessionClass::Write_MultiPlayer_Settings -- writes settings INI         *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   02/14/1995 BR : Created.                                              *
 *=========================================================================*/
void SessionClass::Write_MultiPlayer_Settings() {
  engine::file::IniFile ini;
  bool loaded = false;
  if (const auto file = OpenDiskFile(kConfigFileName)) {
    loaded = ini.Load(*file);
  }
  if (loaded) {
    //	Save the player's last-used Handle & Color
    ini.PutInt("MultiPlayer", "PhoneIndex", CurPhoneIdx);
    ini.PutInt("MultiPlayer", "Color", static_cast<int>(PrefColor));
    ini.PutInt("MultiPlayer", "Side", static_cast<int>(House));
    ini.PutString("MultiPlayer", "Handle", Handle);

    //	Clear all existing Settings.SerialDefault entries.
    ini.Clear("SerialDefaults");

    //	Save default serial settings in opposite order you want to see them
    ini.PutString("SerialDefaults", "CallWaitString",
                  SerialDefaults.CallWaitString);
    ini.PutInt("SerialDefaults", "CallWaitStringIndex",
               SerialDefaults.CallWaitStringIndex ? 1 : 0);
    ini.PutInt("SerialDefaults", "InitStringIndex",
               SerialDefaults.InitStringIndex ? 1 : 0);
    ini.PutString("SerialDefaults", "DialMethod",
                  DialMethodCheck.at(SerialDefaults.DialMethod));
    ini.PutInt("SerialDefaults", "Baud", SerialDefaults.Baud ? 1 : 0);
    ini.PutInt("SerialDefaults", "IRQ", SerialDefaults.IRQ ? 1 : 0);
    ini.PutInt("SerialDefaults", "Port", SerialDefaults.Port);
    ini.PutString("SerialDefaults", "ModemName", SerialDefaults.ModemName);
    ini.PutInt("SerialDefaults", "Compression",
               SerialDefaults.Compression ? 1 : 0);
    ini.PutInt("SerialDefaults", "ErrorCorrection",
               SerialDefaults.ErrorCorrection ? 1 : 0);
    ini.PutInt("SerialDefaults", "HardwareFlowControl",
               SerialDefaults.HardwareFlowControl ? 1 : 0);

    //	Clear all existing InitString entries.
    ini.Clear("InitStrings");

    //	Save all InitString entries.
    for (int index = 0; index < InitStrings.Count(); index++) {
      char buf[10];
      absl::SNPrintF(buf, sizeof(buf), "%03d", index);
      ini.PutString("InitStrings", buf, InitStrings.at(index)->data());
    }

    //	Clear all existing Phone Book entries.
    ini.Clear("PhoneBook");

    //	Save all Phone Book entries.
    //	Format: Entry=Name,PhoneNum,Port,IRQ,Baud,InitString
    for (base::ssize i = PhoneBook.Count() - 1; i >= 0; i--) {
      char buf[128];
      char entrytext[10];
      absl::SNPrintF(buf, sizeof(buf), "%s|%s|%x|%d|%d|%d|%d|%d|%s|%d|%d|%s",
                     PhoneBook.at(i)->Name, PhoneBook.at(i)->Number,
                     static_cast<unsigned int>(PhoneBook.at(i)->Settings.Port),
                     PhoneBook.at(i)->Settings.IRQ,
                     PhoneBook.at(i)->Settings.Baud,
                     PhoneBook.at(i)->Settings.Compression ? 1 : 0,
                     PhoneBook.at(i)->Settings.ErrorCorrection ? 1 : 0,
                     PhoneBook.at(i)->Settings.HardwareFlowControl ? 1 : 0,
                     DialMethodCheck.at(PhoneBook.at(i)->Settings.DialMethod),
                     PhoneBook.at(i)->Settings.InitStringIndex,
                     PhoneBook.at(i)->Settings.CallWaitStringIndex,
                     PhoneBook.at(i)->Settings.CallWaitString);
      absl::SNPrintF(entrytext, sizeof(entrytext), "%03td", i);
      ini.PutString("PhoneBook", entrytext, buf);
    }

    //	Write the INI data out to a file.
    if (const auto file = OpenDiskFile(kConfigFileName, FileAccess::kWrite)) {
      ini.Save(*file);
    }
  }
}

/*
** Certain missions are 126x126 size, and those can't be downloaded to a
** non-Aftermath player, so this function checks to see if the map in
** question is one of those.  We'll know that by the file name: if it's
** K0 -> M9, it's 126x126.
*/
bool Is_Mission_126x126(
    const char* file_name)  //	This is no longer used. ajw
{
  const std::string_view name(file_name == nullptr ? "" : file_name);
  if (name.size() < 6 || isdigit(static_cast<unsigned char>(name.at(5)))) {
    return false;
  }

  if ((name.at(3) >= 'k' && name.at(3) <= 'm') ||
      (name.at(3) >= 'K' && name.at(3) <= 'M')) {
    return true;
  }
  return false;
}

/***************************************************************************
 * SessionClass::Read_Scenario_Descriptions -- reads scen. descriptions    *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   02/14/1995 BR : Created.                                              *
 *   09/10/1996 JLB : Searches using different method.                     *
 *=========================================================================*/
void SessionClass::Read_Scenario_Descriptions() {
  //	Clear the scenario description lists
  Scenarios.Clear();

  /*
  **	Fetch the main multiplayer scenario packet data.
  */
  if (const auto file = OpenGameFile("MISSIONS.PKT")) {
    engine::file::IniFile ini;
    ini.Load(*file);
    for (const engine::file::IniEntry& mission : ini.Entries("Missions")) {
      const std::string fname(mission.name);
      const std::string description(mission.value);
      Scenarios.Add(std::make_unique<MultiMission>(
          fname.c_str(), description.c_str(), nullptr, true,
          IsMissionCounterstrike(fname)));
    }
  /*		//	ajw Copy file for viewing.
                  GameFile fileCopy( "msns_pkt.txt" );
                  file.Seek( 0, SEEK_SET );
                  long lSize = file.Size();
                  char* pData = new char[ lSize + 1 ];
                  file.Read( pData, lSize );
                  fileCopy.Write( pData, lSize );
                  fileCopy.Close();
  */	}

/*
**	Fetch any scenario packet lists and apply them first.
*/
  for (const FoundFile& found : FindFiles("*.PKT")) {
    // debugprint("Found file '%s'.\n", block.cAlternateFileName);
    // debugprint("Found file '%s'.\n", block.cFileName);
    // debugprint( "Found alternate PKT file.\n" );
    engine::file::IniFile ini;
    if (const auto mission_file = OpenGameFile(found.name)) {
      ini.Load(*mission_file);
    }

    for (const engine::file::IniEntry& mission : ini.Entries("Missions")) {
      const std::string fname(mission.name);
      const std::string description(mission.value);

      Scenarios.Add(std::make_unique<MultiMission>(
          fname.c_str(), description.c_str(), nullptr, true,
          IsMissionCounterstrike(fname)));
    }
  }

  /*
  ** Fetch the Counterstrike multiplayer scenario packet data.
  ** Load the scenarios regardless of whether counterstrike's installed,
  ** and at the point of hosting a network game, enable the counterstrike
  ** maps only if they have CS installed.  If they don't, then the maps
  ** are available as a guest, but not as a host, which fixes a multitude
  ** of problems without obviously giving the maps away to non-CS owners.
  */
  if (Is_Counterstrike_Installed()) {
    if (const auto file2 = OpenGameFile("CSTRIKE.PKT")) {
      engine::file::IniFile ini;
      ini.Load(*file2);
      for (const engine::file::IniEntry& mission : ini.Entries("Missions")) {
        const std::string fname(mission.name);
        const std::string description(mission.value);
        Scenarios.Add(std::make_unique<MultiMission>(
            fname.c_str(), description.c_str(), nullptr, true,
            IsMissionCounterstrike(fname)));
      }
      /*ajw Copy file for viewing.
                              GameFile fileCopy( "cs_pkt.txt" );
                              file2.Seek( 0, SEEK_SET );
                              long lSize = file2.Size();
                              char* pData = new char[ lSize + 1 ];
                              file2.Read( pData, lSize );
                              fileCopy.Write( pData, lSize );
                              fileCopy.Close();
      */
    }
  }

  // Aftermath scenarios are now in their own pkt file.
  if (Is_Aftermath_Installed()) {
    if (const auto file2 = OpenGameFile("AFTMATH.PKT")) {
      engine::file::IniFile ini;
      ini.Load(*file2);
      for (const engine::file::IniEntry& mission : ini.Entries("Missions")) {
        const std::string fname(mission.name);
        const std::string description(mission.value);
        Scenarios.Add(std::make_unique<MultiMission>(
            fname.c_str(), description.c_str(), nullptr, true,
            IsMissionCounterstrike(fname)));
      }
    }
  }

  /*
  ** Scan the current directory for any loose .MPR files and build the
  * appropriate entries
  **  into the scenario list list
  */
  for (const FoundFile& found : FindFiles("*.MPR")) {
    // debugprint( "Found MPR '%s'\n", file_name );
    engine::file::IniFile ini;
    if (const auto mission_file = OpenGameFile(found.name)) {
      ini.Load(*mission_file);
    }

    const std::string name = ini.GetString("Basic", "Name", "No Name");
    const std::string digest = ini.GetString("Digest", "1", "No Digest");
    Scenarios.Add(std::make_unique<MultiMission>(
        found.name.c_str(), name.c_str(), digest.c_str(),
        ini.GetBool("Basic", "Official", false), false));
  }
}

/***************************************************************************
 * SessionClass::Free_Scenario_Descriptions -- frees scen. descriptions    *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/05/1995 BRR : Created.                                             *
 *=========================================================================*/
void SessionClass::Free_Scenario_Descriptions() {

  //------------------------------------------------------------------------
  //	Clear the scenario descriptions & filenames
  //------------------------------------------------------------------------
  Scenarios.Clear();
  //	Filenum.Clear();

  //------------------------------------------------------------------------
  //	Clear the initstring entries
  //------------------------------------------------------------------------
  InitStrings.Clear();

  //------------------------------------------------------------------------
  //	Clear the dialing entries
  //------------------------------------------------------------------------
  PhoneBook.Clear();

} /* end of Free_Scenario_Descriptions */

/***************************************************************************
 * SessionClass::Trap_Object -- searches for an object, for debugging *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   06/02/1995 BRR : Created.                                             *
 *=========================================================================*/
void SessionClass::Trap_Object() {

  //------------------------------------------------------------------------
  // Initialize
  //------------------------------------------------------------------------
  TrapObject = nullptr;

  //------------------------------------------------------------------------
  // Search for the object based upon its type, then its coordinate or
  // 'this' pointer value.
  //------------------------------------------------------------------------
  switch (TrapObjType) {
    case RTTI_AIRCRAFT:
      for (int i = 0; i < TheObjectHeaps().aircraft().Count(); i++) {
        if (TheObjectHeaps().aircraft().ActiveAt(i)->Coord == TrapCoord ||
            TheObjectHeaps().aircraft().ActiveAt(i)->As_Target() ==
                TrapTarget) {
          TrapObject = TheObjectHeaps().aircraft().ActiveAt(i);
          break;
        }
      }
      break;

    case RTTI_ANIM:
      for (int i = 0; i < TheObjectHeaps().anim().Count(); i++) {
        if (TheObjectHeaps().anim().ActiveAt(i)->Coord == TrapCoord ||
            TheObjectHeaps().anim().ActiveAt(i)->As_Target() == TrapTarget) {
          TrapObject = TheObjectHeaps().anim().ActiveAt(i);
          break;
        }
      }
      break;

    case RTTI_BUILDING:
      for (int i = 0; i < TheObjectHeaps().building().Count(); i++) {
        if (TheObjectHeaps().building().ActiveAt(i)->Coord == TrapCoord ||
            TheObjectHeaps().building().ActiveAt(i)->As_Target() ==
                TrapTarget) {
          TrapObject = TheObjectHeaps().building().ActiveAt(i);
          break;
        }
      }
      break;

    case RTTI_BULLET:
      for (int i = 0; i < TheObjectHeaps().bullet().Count(); i++) {
        if (TheObjectHeaps().bullet().ActiveAt(i)->Coord == TrapCoord ||
            TheObjectHeaps().bullet().ActiveAt(i)->As_Target() == TrapTarget) {
          TrapObject = TheObjectHeaps().bullet().ActiveAt(i);
          break;
        }
      }
      break;

    case RTTI_INFANTRY:
      for (int i = 0; i < TheObjectHeaps().infantry().Count(); i++) {
        if (TheObjectHeaps().infantry().ActiveAt(i)->Coord == TrapCoord ||
            TheObjectHeaps().infantry().ActiveAt(i)->As_Target() ==
                TrapTarget) {
          TrapObject = TheObjectHeaps().infantry().ActiveAt(i);
          break;
        }
      }
      break;

    case RTTI_UNIT:
      for (int i = 0; i < TheObjectHeaps().unit().Count(); i++) {
        if (TheObjectHeaps().unit().ActiveAt(i)->Coord == TrapCoord ||
            TheObjectHeaps().unit().ActiveAt(i)->As_Target() == TrapTarget) {
          TrapObject = TheObjectHeaps().unit().ActiveAt(i);
          break;
        }
      }
      break;

    //.....................................................................
    // Last-ditch find-the-object-right-now-darnit loop
    //.....................................................................
    case RTTI_NONE:
      for (int i = 0; i < TheObjectHeaps().aircraft().Count(); i++) {
        if (TheObjectHeaps().aircraft().Slot(i)->Coord == TrapCoord ||
            TheObjectHeaps().aircraft().Slot(i)->As_Target() == TrapTarget) {
          TrapObject = TheObjectHeaps().aircraft().Slot(i);
          TrapObjType = RTTI_AIRCRAFT;
          return;
        }
      }
      for (int i = 0; i < TheObjectHeaps().anim().Count(); i++) {
        if (TheObjectHeaps().anim().Slot(i)->Coord == TrapCoord ||
            TheObjectHeaps().anim().Slot(i)->As_Target() == TrapTarget) {
          TrapObject = TheObjectHeaps().anim().Slot(i);
          TrapObjType = RTTI_ANIM;
          return;
        }
      }
      for (int i = 0; i < TheObjectHeaps().building().Count(); i++) {
        if (TheObjectHeaps().building().Slot(i)->Coord == TrapCoord ||
            TheObjectHeaps().building().Slot(i)->As_Target() == TrapTarget) {
          TrapObject = TheObjectHeaps().building().Slot(i);
          TrapObjType = RTTI_BUILDING;
          return;
        }
      }
      for (int i = 0; i < TheObjectHeaps().bullet().Count(); i++) {
        if (TheObjectHeaps().bullet().Slot(i)->Coord == TrapCoord ||
            TheObjectHeaps().bullet().Slot(i)->As_Target() == TrapTarget) {
          TrapObject = TheObjectHeaps().bullet().Slot(i);
          TrapObjType = RTTI_BULLET;
          return;
        }
      }
      for (int i = 0; i < TheObjectHeaps().infantry().Count(); i++) {
        if (TheObjectHeaps().infantry().Slot(i)->Coord == TrapCoord ||
            TheObjectHeaps().infantry().Slot(i)->As_Target() == TrapTarget) {
          TrapObject = TheObjectHeaps().infantry().Slot(i);
          TrapObjType = RTTI_INFANTRY;
          return;
        }
      }
      for (int i = 0; i < TheObjectHeaps().unit().Count(); i++) {
        if (TheObjectHeaps().unit().Slot(i)->Coord == TrapCoord ||
            TheObjectHeaps().unit().Slot(i)->As_Target() == TrapTarget) {
          TrapObject = TheObjectHeaps().unit().Slot(i);
          TrapObjType = RTTI_UNIT;
          return;
        }
      }
      [[fallthrough]];

    case RTTIType::RTTI_AIRCRAFTTYPE:
    case RTTIType::RTTI_ANIMTYPE:
    case RTTIType::RTTI_BUILDINGTYPE:
    case RTTIType::RTTI_BULLETTYPE:
    case RTTIType::RTTI_CELL:
    case RTTIType::RTTI_FACTORY:
    case RTTIType::RTTI_HOUSE:
    case RTTIType::RTTI_HOUSETYPE:
    case RTTIType::RTTI_INFANTRYTYPE:
    case RTTIType::RTTI_OVERLAY:
    case RTTIType::RTTI_OVERLAYTYPE:
    case RTTIType::RTTI_SMUDGE:
    case RTTIType::RTTI_SMUDGETYPE:
    case RTTIType::RTTI_SPECIAL:
    case RTTIType::RTTI_TEAM:
    case RTTIType::RTTI_TEAMTYPE:
    case RTTIType::RTTI_TEMPLATE:
    case RTTIType::RTTI_TEMPLATETYPE:
    case RTTIType::RTTI_TERRAIN:
    case RTTIType::RTTI_TERRAINTYPE:
    case RTTIType::RTTI_TRIGGER:
    case RTTIType::RTTI_TRIGGERTYPE:
    case RTTIType::RTTI_UNITTYPE:
    case RTTIType::RTTI_VESSEL:
    case RTTIType::RTTI_VESSELTYPE:
    default:
      break;
  }
}

/***************************************************************************
 * SessionClass::Compute_Unique_ID -- computes unique local ID number      *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/07/1995 BRR : Created.                                             *
 *=========================================================================*/
uint32_t SessionClass::Compute_Unique_ID() {
  time_t tm = 0;

  //------------------------------------------------------------------------
  // Start with the seconds since Jan 1, 1970 (system local time)
  //------------------------------------------------------------------------
  time(&tm);
  auto id = static_cast<uint32_t>(tm);

  //------------------------------------------------------------------------
  // Now add in the free space on the hard drive
  //------------------------------------------------------------------------
  const auto diskfree = static_cast<uint64_t>(FreeDiskSpace());
  Add_CRC(&id, diskfree & 0xFFFFFFFF);
  Add_CRC(&id, diskfree >> 32);

  //------------------------------------------------------------------------
  // Add in every byte in the user's path environment variable
  //------------------------------------------------------------------------
  if (const auto path = engine::platform::GetEnv("PATH")) {
    for (const char byte : *path) {
      Add_CRC(&id, static_cast<uint32_t>(byte));
    }
  }

  return id;

}  // end of Compute_Unique_ID

MultiMission::MultiMission(const char* filename, const char* description,
                           const char* digest, bool official, bool expansion) {
  Set_Filename(filename);
  Set_Description(description);
  Set_Digest(digest);
  Set_Official(official);
  Set_Expansion(expansion);
}

void MultiMission::Set_Description(const char* description) {
  if (description != nullptr) {
    base::SafeCopy(ScenarioDescription, description);
  }
}

void MultiMission::Set_Filename(const char* filename) {
  if (filename != nullptr) {
    base::SafeCopy(Filename, filename);
  }
}

void MultiMission::Set_Digest(const char* digest) {
  if (digest != nullptr) {
    base::SafeCopy(Digest, digest);
  } else {
    base::SafeCopy(Digest, "NODIGEST");
  }
}

void MultiMission::Set_Official(bool official) { IsOfficial = official; }

void MultiMission::Set_Expansion(bool expansion) { IsExpansion = expansion; }

/************************** end of session.cpp *****************************/

}  // namespace ra
