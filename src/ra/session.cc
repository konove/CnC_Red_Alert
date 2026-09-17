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

#include "ra/session.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>  // for station ID computation
#include <span>
#include <string_view>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/numeric.h"
#include "base/types.h"
#include "magic_enum/magic_enum.hpp"
#include "port/env.h"
#include "port/ex_string.h"
#include "port/safe_string.h"
#include "port/tokenizer.h"
#include "ra/aircraft.h"
#include "ra/anim.h"
#include "ra/building.h"
#include "ra/bullet.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/infantry.h"
#include "ra/ini.h"
#include "ra/installation.h"
#include "ra/ipxmgr.h"
#include "ra/jshell.h"
#include "ra/map.h"
#include "ra/mission_id.h"
#include "ra/queue.h"
#include "ra/unit.h"
#include "sdllib/drawbuff.h"
#include "sdllib/file.h"
#include "sdllib/gbuffer.h"
#include "sdllib/wwstd.h"
#include "tech/archive.h"
#include "tech/byte_sink.h"
#include "tech/byte_source.h"
#include "tech/disk_file.h"
#include "tech/file_sink.h"
#include "tech/file_source.h"
#include "tech/number_parse.h"

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

  RecordFile.SetName("RECORD.BIN");  // always uses this name

  GameName[0] = 0;

  SerialDefaults.Port = 0x2f8;                  // set from INI file
  SerialDefaults.IRQ = 3;                       // set from INI file
  SerialDefaults.Baud = 9600;                   // set from INI file
  SerialDefaults.DialMethod = DIAL_TOUCH_TONE;  // set from INI file
  SerialDefaults.InitStringIndex = 0;           // set from INI file
  SerialDefaults.CallWaitStringIndex = 0;       // set from INI file
  port::SafeCopy(SerialDefaults.CallWaitString, "");

  TrapObject.Ptr.All = nullptr;  // ptr to object being trapped

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

  if (Session.Type != GAME_IPX && Session.Type != GAME_INTERNET) {
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
    if (!port::CompareIgnoreCase(
            Players.at(i)->Name,
            HouseClass::As_Pointer(Players.at(i)->Player.ID)->IniName)) {
      Ipx.Create_Connection(static_cast<int>(Players.at(i)->Player.ID),
                            Players.at(i)->Name, &Players.at(i)->Address);
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
  for (int i = 0; i < Session.MaxPlayers; i++) {
    const auto house =
        static_cast<HousesType>(static_cast<int>(HOUSE_MULTI1) + i);
    HouseClass* hptr = HouseClass::As_Pointer(house);
    if (hptr->IsHuman) {
      return PlayerPtr == hptr;
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
    for (int i = 0; i < Players.Count(); ++i) {
      delete Players.at(i);
    }
    Players.Clear();
    for (int i = 0; i < count; ++i) {
      auto* node = new NodeNameType{};
      ar(*node);
      if (!ar.ok()) {
        delete node;
        return;
      }
      Players.Add(node);
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
int SessionClass::Save(GameFile& file) {
  FileSink pipe(file);
  ArchiveWriter writer(pipe);
  Serialize(writer);
  SerializePlayers(writer);
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
bool SessionClass::Load(GameFile& file) {
  FileSource straw(file);
  ArchiveReader reader(straw);
  Serialize(reader);
  SerializePlayers(reader);
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
  char* entry = nullptr;   // a phone book entry
  char buf[128];           // buffer for parsing INI entry
  int i = 0;

  //	GameFile file (kConfigFileName);

  //------------------------------------------------------------------------
  //	Clear the initstring entries
  //------------------------------------------------------------------------
  for (i = 0; i < InitStrings.Count(); i++) {
    delete[] InitStrings.at(i);
  }
  InitStrings.Clear();

  //	Clear the dialing entries
  for (i = 0; i < PhoneBook.Count(); i++) {
    delete[] PhoneBook.at(i);
  }
  PhoneBook.Clear();

  //	Create filename and read the file.
  INIClass ini;
  DiskFile fc(kConfigFileName);
  if (ini.Load(fc)) {
    //	Get the player's last-used Handle
    ini.Get_String("MultiPlayer", "Handle", "Noname", Handle, sizeof(Handle));

    //	Get the player's last-used Color
    PrefColor =
        static_cast<PlayerColorType>(ini.Get_Int("MultiPlayer", "Color", 0));
    int iSide =
        ini.Get_Int("MultiPlayer", "Side", static_cast<int>(HOUSE_USSR));
    iSide = std::max(2, std::min(6, iSide));
    House = static_cast<HousesType>(iSide);
    CurPhoneIdx = ini.Get_Int("MultiPlayer", "PhoneIndex", -1);
    TrapCheckHeap = ini.Get_Int("MultiPlayer", "CheckHeap", 0);

    //	Read in default serial settings
    ini.Get_String("SerialDefaults", "ModemName", "NoName",
                   SerialDefaults.ModemName, MODEM_NAME_MAX);
    if ((std::string_view(SerialDefaults.ModemName) == "NoName")) {
      SerialDefaults.ModemName[0] = 0;
    }
    SerialDefaults.Port = ini.Get_Int("SerialDefaults", "Port", 0);
    SerialDefaults.IRQ = ini.Get_Int("SerialDefaults", "IRQ", -1);
    SerialDefaults.Baud = ini.Get_Int("SerialDefaults", "Baud", -1);
    SerialDefaults.Compression =
        ini.Get_Int("SerialDefaults", "Compression", 0) != 0;
    SerialDefaults.ErrorCorrection =
        ini.Get_Int("SerialDefaults", "ErrorCorrection", 0) != 0;
    SerialDefaults.HardwareFlowControl =
        ini.Get_Int("SerialDefaults", "HardwareFlowControl", 1) != 0;

    ini.Get_String("SerialDefaults", "DialMethod", "T", buf, 2);


    // find dial method
    for (i = 0; i < static_cast<int>(DIAL_METHODS); i++) {
      if (!port::CompareIgnoreCase(
              buf, DialMethodCheck.at(static_cast<DialMethodType>(i)))) {
        SerialDefaults.DialMethod = static_cast<DialMethodType>(i);
        break;
      }
    }

    // if method not found set to touch tone
    if (i == static_cast<int>(DIAL_METHODS)) {
      SerialDefaults.DialMethod = DIAL_TOUCH_TONE;
    }

    SerialDefaults.InitStringIndex =
        ini.Get_Int("SerialDefaults", "InitStringIndex", 0);

    SerialDefaults.CallWaitStringIndex =
        ini.Get_Int("SerialDefaults", "CallWaitStringIndex", kCallWaitCustom);

    ini.Get_String("SerialDefaults", "CallWaitString", "",
                   SerialDefaults.CallWaitString, CWAITSTRBUF_MAX);

    if (SerialDefaults.IRQ == 0 || SerialDefaults.Baud == 0) {
      SerialDefaults.Port = 0;
      SerialDefaults.IRQ = -1;
      SerialDefaults.Baud = -1;
    }

    const int initcount = ini.Entry_Count("InitStrings");
    for (int index = 0; index < initcount; index++) {
      entry = new char[INITSTRBUF_MAX];
      // entry was allocated above with INITSTRBUF_MAX elements.
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      const std::span entry_buffer(entry, INITSTRBUF_MAX);
      base::At(entry_buffer, 0) = 0;
      ini.Get_String("InitStrings", ini.Get_Entry("InitStrings", index),
                     nullptr, entry_buffer, INITSTRBUF_MAX);
      strupr(entry);
      InitStrings.Add(entry);
    }

    //	if no entries then have at least one
    if (initcount == 0) {
      entry = new char[INITSTRBUF_MAX];
      // The freshly allocated entry contains INITSTRBUF_MAX characters.
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      port::SafeCopy(std::span(entry, INITSTRBUF_MAX), "ATZ");
      InitStrings.Add(entry);
      SerialDefaults.InitStringIndex = 0;
    }

    //	Read the entry names in
    const int phonecount = ini.Entry_Count("PhoneBook");
    for (int index = 0; index < phonecount; index++) {
      //	Create a new phone book entry
      auto* phone = new PhoneEntryClass();  // a phone book entry

      //	Read the entire entry in
      ini.Get_String("PhoneBook", ini.Get_Entry("PhoneBook", index), nullptr,
                     buf, sizeof(buf));

      //	Extract name, phone # & serial port settings
      port::Tokenizer tokens(buf, "|");
      char* tokenptr = tokens.Next();  // ptr to token
      if (tokenptr) {
        port::SafeCopy(phone->Name, tokenptr);
        strupr(phone->Name);
      } else {
        phone->Name[0] = 0;
      }

      tokenptr = tokens.Next();
      if (tokenptr) {
        port::SafeCopy(phone->Number, tokenptr);
        strupr(phone->Number);
      } else {
        phone->Number[0] = 0;
      }

      tokenptr = tokens.Next();
      if (tokenptr) {
        if (const auto value = tech::ParseHex<int>(tokenptr)) {
          phone->Settings.Port = *value;
        }
      } else {
        phone->Settings.Port = 0;
      }

      tokenptr = tokens.Next();
      if (tokenptr) {
        phone->Settings.IRQ = tech::ParseInteger<int>(tokenptr).value_or(0);
      } else {
        phone->Settings.IRQ = -1;
      }

      tokenptr = tokens.Next();
      if (tokenptr) {
        phone->Settings.Baud = tech::ParseInteger<int>(tokenptr).value_or(0);
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
      if (tokenptr) {
        port::SafeCopy(buf, tokenptr);

        // find dial method

        for (i = 0; i < static_cast<int>(DIAL_METHODS); i++) {
          if (!port::CompareIgnoreCase(
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
              tech::ParseInteger<int>(tokenptr).value_or(0) != 0;

          tokenptr = tokens.Next();
          if (tokenptr) {
            phone->Settings.ErrorCorrection =
                tech::ParseInteger<int>(tokenptr).value_or(0) != 0;
          }

          tokenptr = tokens.Next();
          if (tokenptr) {
            phone->Settings.HardwareFlowControl =
                tech::ParseInteger<int>(tokenptr).value_or(0) != 0;
          }

          tokenptr = tokens.Next();
        }
      }

      if (tokenptr) {
        port::SafeCopy(buf, tokenptr);

        //	find dial method
        for (i = 0; i < static_cast<int>(DIAL_METHODS); i++) {
          if (!port::CompareIgnoreCase(
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
      if (tokenptr) {
        phone->Settings.InitStringIndex =
            tech::ParseInteger<int>(tokenptr).value_or(0);
      } else {
        phone->Settings.InitStringIndex = 0;
      }

      tokenptr = tokens.Next();
      if (tokenptr) {
        phone->Settings.CallWaitStringIndex =
            tech::ParseInteger<int>(tokenptr).value_or(0);
      } else {
        phone->Settings.CallWaitStringIndex = kCallWaitCustom;
      }

      tokenptr = tokens.Next();
      if (tokenptr) {
        port::SafeCopy(phone->Settings.CallWaitString, tokenptr);
      } else {
        phone->Settings.CallWaitString[0] = 0;
      }

      //	Add it to our list
      if (!PhoneBook.Add(phone)) {
        delete phone;
      }
    }

    //	Read special recording playback values, to help find sync bugs
    TrapFrame = ini.Get_Int("SyncBug", "Frame", 0x7fffffff);

    ini.Get_String("SyncBug", "Type", "NONE", buf, 80);

    if (!port::CompareIgnoreCase(buf, "AIRCRAFT")) {
      TrapObjType = RTTI_AIRCRAFT;
    } else if (!port::CompareIgnoreCase(buf, "ANIM")) {
      TrapObjType = RTTI_ANIM;
    } else if (!port::CompareIgnoreCase(buf, "BUILDING")) {
      TrapObjType = RTTI_BUILDING;
    } else if (!port::CompareIgnoreCase(buf, "BULLET")) {
      TrapObjType = RTTI_BULLET;
    } else if (!port::CompareIgnoreCase(buf, "INFANTRY")) {
      TrapObjType = RTTI_INFANTRY;
    } else if (!port::CompareIgnoreCase(buf, "UNIT")) {
      TrapObjType = RTTI_UNIT;
    } else {
      TrapObjType = RTTI_NONE;
    }

    ini.Get_String("SyncBug", "Coord", "0", buf, 80);
    TrapCoord = tech::ParseHex<uint32_t>(buf).value_or(0);

    ini.Get_String("SyncBug", "Target", "0", buf, 80);
    TrapTarget = static_cast<TARGET>(tech::ParseHex<uint32_t>(buf).value_or(0));

    ini.Get_String("SyncBug", "Cell", "0", buf, 80);
    CELL const cell = tech::ParseInteger<CELL>(buf).value_or(0);
    if (cell) {
      TrapCell = &Map.at(cell);
    }

    TrapPrintCRC = ini.Get_Int("SyncBug", "PrintCRC", 0x7fffffff);
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

  INIClass ini;
  DiskFile file(kConfigFileName);
  if (ini.Load(file)) {
    //	Save the player's last-used Handle & Color
    ini.Put_Int("MultiPlayer", "PhoneIndex", CurPhoneIdx);
    ini.Put_Int("MultiPlayer", "Color", static_cast<int>(PrefColor));
    ini.Put_Int("MultiPlayer", "Side", static_cast<int>(House));
    ini.Put_String("MultiPlayer", "Handle", Handle);

    //	Clear all existing Settings.SerialDefault entries.
    ini.Clear("SerialDefaults");

    //	Save default serial settings in opposite order you want to see them
    ini.Put_String("SerialDefaults", "CallWaitString",
                   SerialDefaults.CallWaitString);
    ini.Put_Int("SerialDefaults", "CallWaitStringIndex",
                SerialDefaults.CallWaitStringIndex ? 1 : 0);
    ini.Put_Int("SerialDefaults", "InitStringIndex",
                SerialDefaults.InitStringIndex ? 1 : 0);
    ini.Put_String("SerialDefaults", "DialMethod",
                   DialMethodCheck.at(SerialDefaults.DialMethod));
    ini.Put_Int("SerialDefaults", "Baud", SerialDefaults.Baud ? 1 : 0);
    ini.Put_Int("SerialDefaults", "IRQ", SerialDefaults.IRQ ? 1 : 0);
    ini.Put_Int("SerialDefaults", "Port", SerialDefaults.Port, 1);
    ini.Put_String("SerialDefaults", "ModemName", SerialDefaults.ModemName);
    ini.Put_Int("SerialDefaults", "Compression",
                SerialDefaults.Compression ? 1 : 0);
    ini.Put_Int("SerialDefaults", "ErrorCorrection",
                SerialDefaults.ErrorCorrection ? 1 : 0);
    ini.Put_Int("SerialDefaults", "HardwareFlowControl",
                SerialDefaults.HardwareFlowControl ? 1 : 0);

    //	Clear all existing InitString entries.
    ini.Clear("InitStrings");

    //	Save all InitString entries.
    for (int index = 0; index < InitStrings.Count(); index++) {
      char buf[10];
      absl::SNPrintF(buf, sizeof(buf), "%03d", index);
      ini.Put_String("InitStrings", buf, InitStrings.at(index));
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
      ini.Put_String("PhoneBook", entrytext, buf);
    }

    //	Write the INI data out to a file.
    ini.Save(file);
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
  GameFile file("MISSIONS.PKT");
  if (file.IsAvailable()) {
    INIClass ini;
    ini.Load(file);
    const int count = ini.Entry_Count("Missions");
    // debugprint( "Found %i missions in Missions.pkt\n", count );
    for (int index = 0; index < count; index++) {
      const char* fname = ini.Get_Entry("Missions", index);
      char buffer[128];
      ini.Get_String("Missions", fname, "", buffer, sizeof(buffer));
      Scenarios.Add(new MultiMission(fname, buffer, nullptr, true,
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
  FindFileState state{};
  bool found = Find_First_File("*.PKT", state);
  while (found) {
    // debugprint("Found file '%s'.\n", block.cAlternateFileName);
    // debugprint("Found file '%s'.\n", block.cFileName);
    // debugprint( "Found alternate PKT file.\n" );
    GameFile mission_file(state.name);
    INIClass ini;
    ini.Load(mission_file);

    const int count = ini.Entry_Count("Missions");
    for (int index = 0; index < count; index++) {
      const char* fname = ini.Get_Entry("Missions", index);
      char buffer[128];
      ini.Get_String("Missions", fname, "", buffer, sizeof(buffer));

      Scenarios.Add(new MultiMission(fname, buffer, nullptr, true,
                                     IsMissionCounterstrike(fname)));
    }

    found = Find_Next_File(state);
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
    GameFile file2("CSTRIKE.PKT");
    if (file2.IsAvailable()) {
      INIClass ini;
      ini.Load(file2);
      const int count = ini.Entry_Count("Missions");
      // debugprint( "Found %i missions in cstrike.pkt\n", count );
      for (int index = 0; index < count; index++) {
        const char* fname = ini.Get_Entry("Missions", index);
        char buffer[128];
        ini.Get_String("Missions", fname, "", buffer, sizeof(buffer));
        Scenarios.Add(new MultiMission(fname, buffer, nullptr, true,
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
    GameFile file2("AFTMATH.PKT");
    if (file2.IsAvailable()) {
      INIClass ini;
      ini.Load(file2);
      const int count = ini.Entry_Count("Missions");
      // debugprint( "Found %i missions in aftmath.pkt\n", count );
      for (int index = 0; index < count; index++) {
        const char* fname = ini.Get_Entry("Missions", index);
        char buffer[128];
        ini.Get_String("Missions", fname, "", buffer, sizeof(buffer));
        Scenarios.Add(new MultiMission(fname, buffer, nullptr, true,
                                       IsMissionCounterstrike(fname)));
      }
    }
  }

  /*
  ** Scan the current directory for any loose .MPR files and build the
  * appropriate entries
  **  into the scenario list list
  */
  char name_buffer[128];
  char digest_buffer[32];

  found = Find_First_File("*.MPR", state);
  while (found) {
    // debugprint( "Found MPR '%s'\n", file_name );
    GameFile mission_file(state.name);
    INIClass ini;
    ini.Load(mission_file);

    ini.Get_String("Basic", "Name", "No Name", name_buffer,
                   sizeof(name_buffer));
    ini.Get_String("Digest", "1", "No Digest", digest_buffer,
                   sizeof(digest_buffer));
    Scenarios.Add(new MultiMission(state.name, name_buffer, digest_buffer,
                                   ini.Get_Bool("Basic", "Official", false),
                                   false));

    found = Find_Next_File(state);
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
  for (int index = 0; index < Scenarios.Count(); index++) {
    delete Scenarios.at(index);
  }
  Scenarios.Clear();
  //	Filenum.Clear();

  //------------------------------------------------------------------------
  //	Clear the initstring entries
  //------------------------------------------------------------------------
  for (int i = 0; i < InitStrings.Count(); i++) {
    delete[] InitStrings.at(i);
  }
  InitStrings.Clear();

  //------------------------------------------------------------------------
  //	Clear the dialing entries
  //------------------------------------------------------------------------
  for (int i = 0; i < PhoneBook.Count(); i++) {
    delete PhoneBook.at(i);
  }
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
  TrapObject.Ptr.All = nullptr;

  //------------------------------------------------------------------------
  // Search for the object based upon its type, then its coordinate or
  // 'this' pointer value.
  //------------------------------------------------------------------------
  switch (TrapObjType) {
    case RTTI_AIRCRAFT:
      for (int i = 0; i < Aircraft.Count(); i++) {
        if (Aircraft.Ptr(i)->Coord == TrapCoord ||
            Aircraft.Ptr(i)->As_Target() == TrapTarget) {
          TrapObject.Ptr.Aircraft = Aircraft.Ptr(i);
          break;
        }
      }
      break;

    case RTTI_ANIM:
      for (int i = 0; i < Anims.Count(); i++) {
        if (Anims.Ptr(i)->Coord == TrapCoord ||
            Anims.Ptr(i)->As_Target() == TrapTarget) {
          TrapObject.Ptr.Anim = Anims.Ptr(i);
          break;
        }
      }
      break;

    case RTTI_BUILDING:
      for (int i = 0; i < Buildings.Count(); i++) {
        if (Buildings.Ptr(i)->Coord == TrapCoord ||
            Buildings.Ptr(i)->As_Target() == TrapTarget) {
          TrapObject.Ptr.Building = Buildings.Ptr(i);
          break;
        }
      }
      break;

    case RTTI_BULLET:
      for (int i = 0; i < Bullets.Count(); i++) {
        if (Bullets.Ptr(i)->Coord == TrapCoord ||
            Bullets.Ptr(i)->As_Target() == TrapTarget) {
          TrapObject.Ptr.Bullet = Bullets.Ptr(i);
          break;
        }
      }
      break;

    case RTTI_INFANTRY:
      for (int i = 0; i < Infantry.Count(); i++) {
        if (Infantry.Ptr(i)->Coord == TrapCoord ||
            Infantry.Ptr(i)->As_Target() == TrapTarget) {
          TrapObject.Ptr.Infantry = Infantry.Ptr(i);
          break;
        }
      }
      break;

    case RTTI_UNIT:
      for (int i = 0; i < Units.Count(); i++) {
        if (Units.Ptr(i)->Coord == TrapCoord ||
            Units.Ptr(i)->As_Target() == TrapTarget) {
          TrapObject.Ptr.Unit = Units.Ptr(i);
          break;
        }
      }
      break;

    //.....................................................................
    // Last-ditch find-the-object-right-now-darnit loop
    //.....................................................................
    case RTTI_NONE:
      for (int i = 0; i < Aircraft.Count(); i++) {
        if (Aircraft.Raw_Ptr(i)->Coord == TrapCoord ||
            Aircraft.Raw_Ptr(i)->As_Target() == TrapTarget) {
          TrapObject.Ptr.Aircraft = Aircraft.Raw_Ptr(i);
          TrapObjType = RTTI_AIRCRAFT;
          return;
        }
      }
      for (int i = 0; i < Anims.Count(); i++) {
        if (Anims.Raw_Ptr(i)->Coord == TrapCoord ||
            Anims.Raw_Ptr(i)->As_Target() == TrapTarget) {
          TrapObject.Ptr.Anim = Anims.Raw_Ptr(i);
          TrapObjType = RTTI_ANIM;
          return;
        }
      }
      for (int i = 0; i < Buildings.Count(); i++) {
        if (Buildings.Raw_Ptr(i)->Coord == TrapCoord ||
            Buildings.Raw_Ptr(i)->As_Target() == TrapTarget) {
          TrapObject.Ptr.Building = Buildings.Raw_Ptr(i);
          TrapObjType = RTTI_BUILDING;
          return;
        }
      }
      for (int i = 0; i < Bullets.Count(); i++) {
        if (Bullets.Raw_Ptr(i)->Coord == TrapCoord ||
            Bullets.Raw_Ptr(i)->As_Target() == TrapTarget) {
          TrapObject.Ptr.Bullet = Bullets.Raw_Ptr(i);
          TrapObjType = RTTI_BULLET;
          return;
        }
      }
      for (int i = 0; i < Infantry.Count(); i++) {
        if (Infantry.Raw_Ptr(i)->Coord == TrapCoord ||
            Infantry.Raw_Ptr(i)->As_Target() == TrapTarget) {
          TrapObject.Ptr.Infantry = Infantry.Raw_Ptr(i);
          TrapObjType = RTTI_INFANTRY;
          return;
        }
      }
      for (int i = 0; i < Units.Count(); i++) {
        if (Units.Raw_Ptr(i)->Coord == TrapCoord ||
            Units.Raw_Ptr(i)->As_Target() == TrapTarget) {
          TrapObject.Ptr.Unit = Units.Raw_Ptr(i);
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
  const uint64_t diskfree = Disk_Space_Available();
  Add_CRC(&id, diskfree & 0xFFFFFFFF);
  Add_CRC(&id, diskfree >> 32);

  //------------------------------------------------------------------------
  // Add in every byte in the user's path environment variable
  //------------------------------------------------------------------------
  if (const auto path = port::GetEnv("PATH")) {
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

void MultiMission::Draw_It(int /*unused*/, int x, int y, int width, int height,
                           bool selected, TextPrintType flags) const {
  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();
  static const int _tabs[] = {35, 60, 80, 100};
  const TextPrintType point = flags & static_cast<TextPrintType>(0x0F);
  if (point == TPF_6PT_GRAD || point == TPF_EFNT) {
    if (selected) {
      flags = flags | TPF_BRIGHT_COLOR;
      LogicPage->Fill_Rect(x, y, x + width - 1, y + height - 1, scheme->Shadow);
    } else {
      if (!base::Any(flags & TPF_USE_GRAD_PAL)) {
        flags = flags | TPF_MEDIUM_COLOR;
      }
    }

    Conquer_Clip_Text_Print(ScenarioDescription, x, y, scheme, kTBlack, flags,
                            width, _tabs);
  } else {
    Conquer_Clip_Text_Print(ScenarioDescription, x, y,
                            selected ? &ColorRemaps.at(PCOLOR_DIALOG_BLUE)
                                     : &ColorRemaps.at(PCOLOR_GREY),
                            kTBlack, flags, width, _tabs);
  }
}

void MultiMission::Set_Description(const char* description) {
  if (description != nullptr) {
    port::SafeCopy(ScenarioDescription, description);
  }
}

void MultiMission::Set_Filename(const char* filename) {
  if (filename != nullptr) {
    port::SafeCopy(Filename, filename);
  }
}

void MultiMission::Set_Digest(const char* digest) {
  if (digest != nullptr) {
    port::SafeCopy(Digest, digest);
  } else {
    port::SafeCopy(Digest, "NODIGEST");
  }
}

void MultiMission::Set_Official(bool official) { IsOfficial = official; }

void MultiMission::Set_Expansion(bool expansion) { IsExpansion = expansion; }

/************************** end of session.cpp *****************************/
