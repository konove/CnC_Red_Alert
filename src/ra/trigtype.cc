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

/* $Header: /CounterStrike/TRIGTYPE.CPP 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TRIGTYPE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/05/96 *
 *                                                                                             *
 *                  Last Update : July 9, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * TriggerTypeClass::As_Target -- Convert this trigger type object
 *into a target value.      * TriggerTypeClass::Attaches_To -- Determines what
 *trigger can attach to.                   * TriggerTypeClass::Build_INI_Entry
 *-- Construct the INI entry into the buffer specified.   *
 *   TriggerTypeClass::Description -- Build a text description of the trigger
 *type.            * TriggerTypeClass::Detach -- Removes attachments to the
 *target object specified.           * TriggerTypeClass::Draw_It -- Draws this
 *trigger as if it were a line in a list box.       * TriggerTypeClass::Edit --
 *Edit the trigger type through the scenario editor.              *
 *   TriggerTypeClass::Fill_In -- fills in trigger from the given INI entry *
 *   TriggerTypeClass::From_Name -- Convert an ASCII name into a trigger type
 *pointer.         * TriggerTypeClass::Init -- Initialize the trigger type
 *object management system.           * TriggerTypeClass::Read_INI -- reads
 *triggers from the INI file                            *
 *   TriggerTypeClass::TriggerTypeClass -- Constructor for trigger class object.
 ** TriggerTypeClass::Write_INI -- Stores all trigger types to the INI database
 *specified.    * TriggerTypeClass::operator delete -- Returns a trigger type
 *class object back to the pool * TriggerTypeClass::operator new -- Allocates a
 *trigger type class object.                  *
 *   TriggerTypeClass::~TriggerTypeClass -- Deleting a trigger type deletes
 *associated triggers*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/trigtype.h"

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "absl/strings/str_format.h"
#include "base/numeric.h"
#include "magic_enum/magic_enum.hpp"
#include "port/ex_string.h"
#include "port/safe_string.h"
#include "port/tokenizer.h"
#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/const.h"
#include "ra/control.h"
#include "ra/debug.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/drop.h"
#include "ra/edit.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/heap.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/taction.h"
#include "ra/teamtype.h"
#include "ra/tevent.h"
#include "ra/text_ids.h"
#include "ra/textbtn.h"
#include "ra/theme.h"
#include "ra/type.h"
#include "ra/ww_audio.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/mix_archive.h"
#include "tech/number_parse.h"

// The low nibble of the text print flags selects the font.
static constexpr bool Is_Font(const TextPrintType flags,
                              const TextPrintType font) {
  return (static_cast<uint32_t>(flags) & 0x0FU) == static_cast<uint32_t>(font);
}

/***********************************************************************************************
 * TriggerTypeClass::TriggerTypeClass -- Constructor for trigger class object. *
 *                                                                                             *
 *    This is the normal constructor for a trigger object. The trigger starts
 *with no team     * members, no mission, and default values for all settings. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/10/1996 JLB : Created. *
 *=============================================================================================*/
TriggerTypeClass::TriggerTypeClass()
    : AbstractTypeClass(RTTI_TRIGGERTYPE, TriggerTypes.ID(this), TXT_NONE, "x") {
}

/***********************************************************************************************
 * TriggerTypeClass::operator new -- Allocates a trigger type class object. *
 *                                                                                             *
 *    This routine will allocate a block of memory from the special trigger type
 *object        * pool. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the allocated trigger type memory block.
 *If there is     * no more block available in the pool, then NULL is returned.
 **
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
void* TriggerTypeClass::operator new(size_t /*unused*/) {
  void* ptr = TriggerTypes.Allocate();
  if (ptr) {
    static_cast<TriggerTypeClass*>(ptr)->IsActive = true;
  }

  return ptr;
}

/***********************************************************************************************
 * TriggerTypeClass::operator delete -- Returns a trigger type class object back
 *to the pool   *
 *                                                                                             *
 *    This routine will return a previously allocated trigger type object to the
 *private       * memory pool from which it was allocated. *
 *                                                                                             *
 * INPUT:   ptr   -- Pointer to the trigger type class to return to the pool. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
void TriggerTypeClass::operator delete(void* ptr) {
  if (ptr) {
    static_cast<TriggerTypeClass*>(ptr)->IsActive = false;
  }
  TriggerTypes.Free(static_cast<TriggerTypeClass*>(ptr));
}

/***********************************************************************************************
 * TriggerTypeClass::Detach -- Removes attachments to the target object
 *specified.             *
 *                                                                                             *
 *    When an object disappears from the game, it must be detached from all
 *other objects that * may be referring to it. This routine will detach the
 *specified target object from any    * references to it in this trigger type
 *class.                                             *
 *                                                                                             *
 * INPUT:   target   -- The target object to be detached from this trigger type.
 **
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
void TriggerTypeClass::Detach(TARGET target, bool /*unused*/) {
  Action1.Detach(target);
  Action2.Detach(target);
}

// Layout and gadget IDs of the trigger editor dialog.
/*
**	Dialog position and dimensions.
*/
constexpr int kDialogW = 320 + 100;
constexpr int kDialogH = 200 + 20;
constexpr int kDialogX = 0;
constexpr int kDialogY = 0;

/*
**	Event entry list box coordinates and dimensions.
*/
constexpr int kE1X = kDialogX + 45;
constexpr int kE1Y = kDialogY + 65;
constexpr int kE2X = kE1X;
constexpr int kE2Y = kE1Y + 22;
constexpr int kEWidth = 160;
constexpr int kEHeight = 8 * 5;

/*
**	Event optional data entry coordinates and dimensions.
*/
constexpr int kEd1X = kE1X + kEWidth + 20;
constexpr int kEd1Y = kE1Y;
constexpr int kEd2X = kEd1X;
constexpr int kEd2Y = kE2Y;

constexpr int kEdWidth = 95;
constexpr int kEdHeight = 8 * 5;

/*
**	Action entry list box coordinates.
*/
constexpr int kA1X = kE1X;
constexpr int kA1Y = kDialogY + 120;
constexpr int kA2X = kE1X;
constexpr int kA2Y = kA1Y + 22;

/*
**	Action optional data entry coordinates.
*/
constexpr int kAd1X = kA1X + kEWidth + 20;
constexpr int kAd1Y = kA1Y;
constexpr int kAd2X = kAd1X;
constexpr int kAd2Y = kA2Y;

/*
**	Misc control values.
*/
constexpr int kGeneralSize = 10;  // Text length for general data entry fields.
constexpr int kEntrySize =
    35;  // Maximum size of event or action description text.
constexpr int kWaypointSize = 3;  // Text length maximum for waypoint entry.
constexpr int kTeamSize = 10;     // Team name text entry field length.
constexpr int kDescSize =
    35;  // Maximum length of object full name description.

/*
**	Button enumerations:
*/
constexpr int kEventList = 100;      // Primary event list.
constexpr int kEventList2 = 101;     // Secondary event list.
constexpr int kActionList = 102;     // Primary action list.
constexpr int kActionList2 = 103;    // Secondary action list.
constexpr int kNameEdit = 104;       // Trigger name edit field.
constexpr int kDataSpeech1 = 105;    // Primary action speech.
constexpr int kDataSpeech2 = 106;    // Secondary action speech.
constexpr int kDataTheme1 = 107;     // Primary action theme.
constexpr int kDataTheme2 = 108;     // Secondary action theme.
constexpr int kDataMovie1 = 109;     // Primary action movie.
constexpr int kDataMovie2 = 110;     // Secondary action movie.
constexpr int kDataSound1 = 111;     // Primary action sound effect.
constexpr int kDataSound2 = 112;     // Secondary action sound effect.
constexpr int kDataSpecial1 = 113;   // Primary action special weapon.
constexpr int kDataSpecial2 = 114;   // Secondary action special weapon.
constexpr int kDataEdit = 115;       // Primary event waypoint data field.
constexpr int kDataEdit2 = 116;      // Secondary event waypoint data field.
constexpr int kDataEdit3 = 117;      // Primary action waypoint data field.
constexpr int kDataEdit4 = 118;      // Secondary action waypoint data field.
constexpr int kDataHtype1 = 119;     // Primary event house choice list.
constexpr int kDataHtype2 = 120;     // Secondary event house choice list.
constexpr int kDataHtype3 = 121;     // Primary action house choice list.
constexpr int kDataHtype4 = 122;     // Secondary action house choice list.
constexpr int kDataBooltype1 = 123;  // Primary action boolean data list.
constexpr int kDataBooltype2 = 124;  // Secondary action boolean data list.
constexpr int kDataGeneral1 = 125;   // Primary event general data field.
constexpr int kDataGeneral2 = 126;   // Secondary event general data field.
constexpr int kDataGeneral3 = 127;   // Primary action general data field.
constexpr int kDataGeneral4 = 128;   // Secondary action general data field.
constexpr int kDataBtype1 = 129;     // Primary event building type list.
constexpr int kDataBtype2 = 130;     // Secondary event building type list.
constexpr int kDataItype1 = 131;     // Primary event infantry type list.
constexpr int kDataItype2 = 132;     // Secondary event infantry type list.
constexpr int kDataAtype1 = 133;     // Primary event aircraft type list.
constexpr int kDataAtype2 = 134;     // Secondary event aircraft type list.
constexpr int kDataUtype1 = 135;     // Primary event unit type list.
constexpr int kDataUtype2 = 136;     // Secondary event unit type list.
constexpr int kDataTtype1 = 137;     // Primary event team type entry list.
constexpr int kDataTtype2 = 138;     // Secondary event team type entry list.
constexpr int kDataTtype3 = 139;     // Primary action team type entry list.
constexpr int kDataTtype4 = 140;     // Secondary action team type entry list.
constexpr int kDataTrtype1 = 141;    // Primary action trigger list.
constexpr int kDataTrtype2 = 142;    // Secondary action trigger list.
constexpr int kButtonHouse = 143;    // House ownership for this trigger.
constexpr int kButtonPersistance = 144;  // Persistence of this trigger.
constexpr int kButtonOk = 145;           // Ok button - save and exit.
constexpr int kKbuttoncancel = 146;      // Cancel button - just exit.
constexpr int kButtonAction = 147;       // Multiple action control button.
constexpr int kButtonEvent = 148;        // Multiple event control button.

/***********************************************************************************************
 * TriggerTypeClass::Edit -- Edit the trigger type through the scenario editor.
 **
 *                                                                                             *
 *    This is the scenario editor interface to a trigger type class object. It
 *brings up a     * fancy schmancy dialog to allow full edit control of the
 *trigger type.                    *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Was the "OK" button pressed? A false return value indicates
 *that the edits   * to this trigger type class object should be rejected. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
bool TriggerTypeClass::Edit() {


  /*
  **	Dialog variables:
  */
  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();

  /*
  **	Buttons
  */
  ControlClass* commands = nullptr;  // the button list

  /*
  **	List of events allowed.
  */
  char eventtext[kEntrySize] = "";
  TDropListClass<EventChoiceClass*> event1list(
      kEventList, eventtext, sizeof(eventtext), TPF_EFNT | TPF_NOSHADOW, kE1X,
      kE1Y, kEWidth, kEHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char event2text[kEntrySize] = "";
  TDropListClass<EventChoiceClass*> event2list(
      kEventList2, event2text, sizeof(event2text), TPF_EFNT | TPF_NOSHADOW,
      kE2X, kE2Y, kEWidth, kEHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (TEventType event = TEVENT_NONE; event < TEVENT_COUNT; event++) {
    event1list.Add_Item(&EventChoices.at(event));
    event2list.Add_Item(&EventChoices.at(event));
  }

  PBubble_Sort(event1list, event1list.Count());
  PBubble_Sort(event2list, event2list.Count());

  if (Event1.Event == TEVENT_NONE) {
    Event1.Event = TEVENT_NONE;
  }
  event1list.Set_Selected_Index(&EventChoices.at(Event1.Event));
  if (Event2.Event == TEVENT_NONE) {
    Event2.Event = TEVENT_NONE;
  }
  event2list.Set_Selected_Index(&EventChoices.at(Event2.Event));

  /*
  **	List of actions allowed.
  */
  char actiontext[kEntrySize] = "";
  TDropListClass<ActionChoiceClass*> action1list(
      kActionList, actiontext, sizeof(actiontext), TPF_EFNT | TPF_NOSHADOW,
      kA1X, kA1Y, kEWidth, kEHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2text[kEntrySize] = "";
  TDropListClass<ActionChoiceClass*> action2list(
      kActionList2, action2text, sizeof(action2text), TPF_EFNT | TPF_NOSHADOW,
      kA2X, kA2Y, kEWidth, kEHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (TActionType action = TACTION_NONE; action < TACTION_COUNT; action++) {
    action1list.Add_Item(&ActionChoices.at(action));
    action2list.Add_Item(&ActionChoices.at(action));
  }

  PBubble_Sort(action1list, action1list.Count());
  PBubble_Sort(action2list, action2list.Count());

  if (Action1.Action == TACTION_NONE) {
    Action1.Action = TACTION_NONE;
  }
  action1list.Set_Selected_Index(&ActionChoices.at(Action1.Action));
  if (Action2.Action == TACTION_NONE) {
    Action2.Action = TACTION_NONE;
  }
  action2list.Set_Selected_Index(&ActionChoices.at(Action2.Action));

  /*
  **	Optional waypoint entry field.
  */
  char way1[kWaypointSize] = "A";
  EditClass way1data(kDataEdit, way1, sizeof(way1), TPF_EFNT | TPF_NOSHADOW,
                     kEd1X, kEd1Y, kEdWidth, 9, EditClass::kAlpha);
  if (Event_Needs(Event1.Event) == NEED_WAYPOINT) {
    if (Event1.Data.Value < 26) {
      absl::SNPrintF(way1, sizeof(way1), "%c",
                     static_cast<int>(Event1.Data.Value) + 'A');
    } else {
      absl::SNPrintF(way1, sizeof(way1), "%c%c",
                     (Event1.Data.Value / 26) + 'A' - 1,
                     (Event1.Data.Value % 26) + 'A');
    }
  }

  char way2[kWaypointSize] = "A";
  EditClass way2data(kDataEdit2, way2, sizeof(way2), TPF_EFNT | TPF_NOSHADOW,
                     kEd2X, kEd2Y, kEdWidth, 9, EditClass::kAlpha);
  if (Event_Needs(Event2.Event) == NEED_WAYPOINT) {
    if (Event2.Data.Value < 26) {
      absl::SNPrintF(way2, sizeof(way2), "%c",
                     static_cast<int>(Event2.Data.Value) + 'A');
    } else {
      absl::SNPrintF(way2, sizeof(way2), "%c%c",
                     (Event2.Data.Value / 26) + 'A' - 1,
                     (Event2.Data.Value % 26) + 'A');
    }
  }

  char way3[kWaypointSize] = "A";
  EditClass way3data(kDataEdit3, way3, sizeof(way3), TPF_EFNT | TPF_NOSHADOW,
                     kAd1X, kAd1Y, kEdWidth, 9, EditClass::kAlpha);
  if (Action_Needs(Action1.Action) == NEED_WAYPOINT) {
    if (Action1.Data.Value < 26) {
      absl::SNPrintF(way3, sizeof(way3), "%c", Action1.Data.Value + 'A');
    } else {
      absl::SNPrintF(way3, sizeof(way3), "%c%c",
                     (Action1.Data.Value / 26) + 'A' - 1,
                     (Action1.Data.Value % 26) + 'A');
    }
  }

  char way4[kWaypointSize] = "A";
  EditClass way4data(kDataEdit4, way4, sizeof(way4), TPF_EFNT | TPF_NOSHADOW,
                     kAd2X, kAd2Y, kEdWidth, 9, EditClass::kAlpha);
  if (Action_Needs(Action2.Action) == NEED_WAYPOINT) {
    if (Action2.Data.Value < 26) {
      absl::SNPrintF(way4, sizeof(way4), "%c", Action2.Data.Value + 'A');
    } else {
      absl::SNPrintF(way4, sizeof(way4), "%c%c",
                     (Action2.Data.Value / 26) + 'A' - 1,
                     (Action2.Data.Value % 26) + 'A');
    }
  }

  /*
  **	Optional event data entry field.
  */
  char databuf1[kGeneralSize] = "";
  EditClass event1data(kDataGeneral1, databuf1, sizeof(databuf1),
                       TPF_EFNT | TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth, 9,
                       EditClass::kNumeric);
  switch (Event_Needs(Event1.Event)) {
    case NEED_TIME:
    case NEED_NUMBER:
      absl::SNPrintF(databuf1, sizeof(databuf1), "%d",
                     static_cast<int>(Event1.Data.Value));
      break;
    case NeedType::NEED_NONE:
    case NeedType::NEED_THEME:
    case NeedType::NEED_MOVIE:
    case NeedType::NEED_SOUND:
    case NeedType::NEED_SPEECH:
    case NeedType::NEED_INFANTRY:
    case NeedType::NEED_UNIT:
    case NeedType::NEED_AIRCRAFT:
    case NeedType::NEED_STRUCTURE:
    case NeedType::NEED_WAYPOINT:
    case NeedType::NEED_TRIGGER:
    case NeedType::NEED_TEAM:
    case NeedType::NEED_HOUSE:
    case NeedType::NEED_QUARRY:
    case NeedType::NEED_FORMATION:
    case NeedType::NEED_BOOL:
    case NeedType::NEED_SPECIAL:
    case NeedType::NEED_MISSION:
    case NeedType::NEED_HEX_NUMBER:
    default:
      break;
  }

  char databuf2[kGeneralSize] = "";
  EditClass event2data(kDataGeneral2, databuf2, sizeof(databuf2),
                       TPF_EFNT | TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth, 9,
                       EditClass::kNumeric);
  switch (Event_Needs(Event2.Event)) {
    case NEED_TIME:
    case NEED_NUMBER:
      absl::SNPrintF(databuf2, sizeof(databuf2), "%d",
                     static_cast<int>(Event2.Data.Value));
      break;
    case NeedType::NEED_NONE:
    case NeedType::NEED_THEME:
    case NeedType::NEED_MOVIE:
    case NeedType::NEED_SOUND:
    case NeedType::NEED_SPEECH:
    case NeedType::NEED_INFANTRY:
    case NeedType::NEED_UNIT:
    case NeedType::NEED_AIRCRAFT:
    case NeedType::NEED_STRUCTURE:
    case NeedType::NEED_WAYPOINT:
    case NeedType::NEED_TRIGGER:
    case NeedType::NEED_TEAM:
    case NeedType::NEED_HOUSE:
    case NeedType::NEED_QUARRY:
    case NeedType::NEED_FORMATION:
    case NeedType::NEED_BOOL:
    case NeedType::NEED_SPECIAL:
    case NeedType::NEED_MISSION:
    case NeedType::NEED_HEX_NUMBER:
    default:
      break;
  }

  char actionbuf1[kGeneralSize] = "";
  EditClass action1data(kDataGeneral3, actionbuf1, sizeof(actionbuf1),
                        TPF_EFNT | TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth, 9,
                        EditClass::kNumeric);
  if (Action_Needs(Action1.Action) == NEED_NUMBER) {
    absl::SNPrintF(actionbuf1, sizeof(actionbuf1), "%d", Action1.Data.Value);
  }

  char actionbuf2[kGeneralSize] = "";
  EditClass action2data(kDataGeneral4, actionbuf2, sizeof(actionbuf2),
                        TPF_EFNT | TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth, 9,
                        EditClass::kNumeric);
  if (Action_Needs(Action2.Action) == NEED_NUMBER) {
    absl::SNPrintF(actionbuf2, sizeof(actionbuf2), "%d", Action2.Data.Value);
  }

  /*
  **	Optional team entry list.
  */
  char tbuf1[kTeamSize] = "";
  DropListClass ttype1list(kDataTtype1, tbuf1, sizeof(tbuf1),
                           TPF_EFNT | TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char tbuf2[kTeamSize] = "";
  DropListClass ttype2list(kDataTtype2, tbuf2, sizeof(tbuf2),
                           TPF_EFNT | TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char tbuf3[kTeamSize] = "";
  DropListClass ttype3list(kDataTtype3, tbuf3, sizeof(tbuf3),
                           TPF_EFNT | TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char tbuf4[kTeamSize] = "";
  DropListClass ttype4list(kDataTtype4, tbuf4, sizeof(tbuf4),
                           TPF_EFNT | TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (int index = 0; index < TeamTypes.Count(); index++) {
    ttype1list.Add_Item(TeamTypes.Ptr(index)->IniName);
    ttype2list.Add_Item(TeamTypes.Ptr(index)->IniName);
    ttype3list.Add_Item(TeamTypes.Ptr(index)->IniName);
    ttype4list.Add_Item(TeamTypes.Ptr(index)->IniName);
  }

  if (Event1.Team.Is_Valid()) {
    ttype1list.Set_Selected_Index(Event1.Team->IniName);
  } else {
    ttype1list.Set_Selected_Index(0);
  }
  if (Event2.Team.Is_Valid()) {
    ttype2list.Set_Selected_Index(Event2.Team->IniName);
  } else {
    ttype2list.Set_Selected_Index(0);
  }
  if (Action1.Team.Is_Valid()) {
    ttype3list.Set_Selected_Index(Action1.Team->IniName);
  } else {
    ttype3list.Set_Selected_Index(0);
  }
  if (Action2.Team.Is_Valid()) {
    ttype4list.Set_Selected_Index(Action2.Team->IniName);
  } else {
    ttype4list.Set_Selected_Index(0);
  }

  /*
  **	Optional trigger entry list.
  */
  char trbuf1[kTeamSize] = "";
  DropListClass trtype1list(kDataTrtype1, trbuf1, sizeof(trbuf1),
                            TPF_EFNT | TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth,
                            kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                            MixArchive::RetrieveData("EBTN-DN.SHP"));
  char trbuf2[kTeamSize] = "";
  DropListClass trtype2list(kDataTrtype2, trbuf2, sizeof(trbuf2),
                            TPF_EFNT | TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth,
                            kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                            MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (int index = 0; index < TriggerTypes.Count(); index++) {
    trtype1list.Add_Item(TriggerTypes.Ptr(index)->IniName);
    trtype2list.Add_Item(TriggerTypes.Ptr(index)->IniName);
  }

  if (Action1.Trigger.Is_Valid()) {
    trtype1list.Set_Selected_Index(Action1.Trigger->IniName);
  } else {
    trtype1list.Set_Selected_Index(0);
  }
  if (Action2.Trigger.Is_Valid()) {
    trtype2list.Set_Selected_Index(Action2.Trigger->IniName);
  } else {
    trtype2list.Set_Selected_Index(0);
  }

  /*
  **	Optional boolean value list.
  */
  char boolbuf1[kTeamSize] = "";
  DropListClass booltype1list(kDataBooltype1, boolbuf1, sizeof(boolbuf1),
                              TPF_EFNT | TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth,
                              kEdHeight,
                              MixArchive::RetrieveData("EBTN-UP.SHP"),
                              MixArchive::RetrieveData("EBTN-DN.SHP"));
  char boolbuf2[kTeamSize] = "";
  DropListClass booltype2list(kDataBooltype2, boolbuf2, sizeof(boolbuf2),
                              TPF_EFNT | TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth,
                              kEdHeight,
                              MixArchive::RetrieveData("EBTN-UP.SHP"),
                              MixArchive::RetrieveData("EBTN-DN.SHP"));

  booltype1list.Add_Item("OFF");
  booltype1list.Add_Item("ON");
  booltype2list.Add_Item("OFF");
  booltype2list.Add_Item("ON");

  booltype1list.Set_Selected_Index(Action1.Data.Bool ? 1 : 0);
  booltype2list.Set_Selected_Index(Action2.Data.Bool ? 1 : 0);

  /*
  **	Optional musical theme choice list.
  */
  char themebuf1[kDescSize] = "";
  DropListClass themetype1list(
      kDataTheme1, themebuf1, sizeof(themebuf1), TPF_EFNT | TPF_NOSHADOW, kAd1X,
      kAd1Y, kEdWidth, kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char themebuf2[kDescSize] = "";
  DropListClass themetype2list(
      kDataTheme2, themebuf2, sizeof(themebuf2), TPF_EFNT | TPF_NOSHADOW, kAd2X,
      kAd2Y, kEdWidth, kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const ThemeType theme : magic_enum::enum_values<ThemeType>()) {
    themetype1list.Add_Item(ThemeClass::Full_Name(theme));
    themetype2list.Add_Item(ThemeClass::Full_Name(theme));
  }

  if (Action_Needs(Action1.Action) == NEED_THEME) {
    themetype1list.Set_Selected_Index(static_cast<int>(Action1.Data.Theme));
  } else {
    themetype1list.Set_Selected_Index(0);
  }
  if (Action_Needs(Action2.Action) == NEED_THEME) {
    themetype2list.Set_Selected_Index(static_cast<int>(Action2.Data.Theme));
  } else {
    themetype2list.Set_Selected_Index(0);
  }

  /*
  **	Optional movie list.
  */
  char moviebuf1[kDescSize] = "";
  DropListClass movietype1list(
      kDataMovie1, moviebuf1, sizeof(moviebuf1), TPF_EFNT | TPF_NOSHADOW, kAd1X,
      kAd1Y, kEdWidth, kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char moviebuf2[kDescSize] = "";
  DropListClass movietype2list(
      kDataMovie2, moviebuf2, sizeof(moviebuf2), TPF_EFNT | TPF_NOSHADOW, kAd2X,
      kAd2Y, kEdWidth, kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const VQType movie : magic_enum::enum_values<VQType>()) {
    movietype1list.Add_Item(VQName.at(movie));
    movietype2list.Add_Item(VQName.at(movie));
  }

  if (Action_Needs(Action1.Action) == NEED_MOVIE) {
    movietype1list.Set_Selected_Index(static_cast<int>(Action1.Data.Movie));
  } else {
    movietype1list.Set_Selected_Index(0);
  }
  if (Action_Needs(Action2.Action) == NEED_MOVIE) {
    movietype2list.Set_Selected_Index(static_cast<int>(Action2.Data.Movie));
  } else {
    movietype2list.Set_Selected_Index(0);
  }

  /*
  **	Optional sound effect list.
  */
  char soundbuf1[kDescSize] = "";
  DropListClass soundtype1list(
      kDataSound1, soundbuf1, sizeof(soundbuf1), TPF_EFNT | TPF_NOSHADOW, kAd1X,
      kAd1Y, kEdWidth, kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char soundbuf2[kDescSize] = "";
  DropListClass soundtype2list(
      kDataSound2, soundbuf2, sizeof(soundbuf2), TPF_EFNT | TPF_NOSHADOW, kAd2X,
      kAd2Y, kEdWidth, kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const VocType sound : magic_enum::enum_values<VocType>()) {
    soundtype1list.Add_Item(Voc_Name(sound));
    soundtype2list.Add_Item(Voc_Name(sound));
  }

  if (Action_Needs(Action1.Action) == NEED_SOUND) {
    soundtype1list.Set_Selected_Index(static_cast<int>(Action1.Data.Sound));
  } else {
    soundtype1list.Set_Selected_Index(0);
  }
  if (Action_Needs(Action2.Action) == NEED_SOUND) {
    soundtype2list.Set_Selected_Index(static_cast<int>(Action2.Data.Sound));
  } else {
    soundtype2list.Set_Selected_Index(0);
  }

  /*
  **	Optional speech effect list.
  */
  char speechbuf1[kDescSize] = "";
  DropListClass speechtype1list(kDataSpeech1, speechbuf1, sizeof(speechbuf1),
                                TPF_EFNT | TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth,
                                kEdHeight,
                                MixArchive::RetrieveData("EBTN-UP.SHP"),
                                MixArchive::RetrieveData("EBTN-DN.SHP"));
  char speechbuf2[kDescSize] = "";
  DropListClass speechtype2list(kDataSpeech2, speechbuf2, sizeof(speechbuf2),
                                TPF_EFNT | TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth,
                                kEdHeight,
                                MixArchive::RetrieveData("EBTN-UP.SHP"),
                                MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const VoxType speech : magic_enum::enum_values<VoxType>()) {
    speechtype1list.Add_Item(Speech_Name(speech));
    speechtype2list.Add_Item(Speech_Name(speech));
  }

  if (Action_Needs(Action1.Action) == NEED_SPEECH) {
    speechtype1list.Set_Selected_Index(static_cast<int>(Action1.Data.Speech));
  } else {
    speechtype1list.Set_Selected_Index(0);
  }
  if (Action_Needs(Action2.Action) == NEED_SPEECH) {
    speechtype2list.Set_Selected_Index(static_cast<int>(Action2.Data.Speech));
  } else {
    speechtype2list.Set_Selected_Index(0);
  }

  /*
  **	Optional building type entry list.
  */
  char bbuf1[kDescSize] = "";
  DropListClass btype1list(kDataBtype1, bbuf1, sizeof(bbuf1),
                           TPF_EFNT | TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char bbuf2[kDescSize] = "";
  DropListClass btype2list(kDataBtype2, bbuf2, sizeof(bbuf2),
                           TPF_EFNT | TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const StructType ss : magic_enum::enum_values<StructType>()) {
    btype1list.Add_Item(
        Text_String(BuildingTypeClass::As_Reference(ss).Full_Name()));
    btype2list.Add_Item(
        Text_String(BuildingTypeClass::As_Reference(ss).Full_Name()));
  }

  if (Event_Needs(Event1.Event) == NEED_STRUCTURE) {
    btype1list.Set_Selected_Index(static_cast<int>(Event1.Data.Structure));
  } else {
    btype1list.Set_Selected_Index(0);
  }
  if (Event_Needs(Event2.Event) == NEED_STRUCTURE) {
    btype2list.Set_Selected_Index(static_cast<int>(Event2.Data.Structure));
  } else {
    btype2list.Set_Selected_Index(0);
  }

  /*
  **	Optional infantry type entry list.
  */
  char ibuf1[kDescSize] = "";
  DropListClass itype1list(kDataItype1, ibuf1, sizeof(ibuf1),
                           TPF_EFNT | TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char ibuf2[kDescSize] = "";
  DropListClass itype2list(kDataItype2, ibuf2, sizeof(ibuf2),
                           TPF_EFNT | TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const InfantryType ii : magic_enum::enum_values<InfantryType>()) {
    itype1list.Add_Item(
        Text_String(InfantryTypeClass::As_Reference(ii).Full_Name()));
    itype2list.Add_Item(
        Text_String(InfantryTypeClass::As_Reference(ii).Full_Name()));
  }

  if (Event_Needs(Event1.Event) == NEED_INFANTRY) {
    itype1list.Set_Selected_Index(static_cast<int>(Event1.Data.Infantry));
  } else {
    itype1list.Set_Selected_Index(0);
  }
  if (Event_Needs(Event2.Event) == NEED_INFANTRY) {
    itype2list.Set_Selected_Index(static_cast<int>(Event2.Data.Infantry));
  } else {
    itype2list.Set_Selected_Index(0);
  }

  /*
  **	Optional aircraft type entry list.
  */
  char abuf1[kDescSize] = "";
  DropListClass atype1list(kDataAtype1, abuf1, sizeof(abuf1),
                           TPF_EFNT | TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char abuf2[kDescSize] = "";
  DropListClass atype2list(kDataAtype2, abuf2, sizeof(abuf2),
                           TPF_EFNT | TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const AircraftType aa : magic_enum::enum_values<AircraftType>()) {
    atype1list.Add_Item(
        Text_String(AircraftTypeClass::As_Reference(aa).Full_Name()));
    atype2list.Add_Item(
        Text_String(AircraftTypeClass::As_Reference(aa).Full_Name()));
  }

  if (Event_Needs(Event1.Event) == NEED_AIRCRAFT) {
    atype1list.Set_Selected_Index(static_cast<int>(Event1.Data.Aircraft));
  } else {
    atype1list.Set_Selected_Index(0);
  }
  if (Event_Needs(Event2.Event) == NEED_AIRCRAFT) {
    atype2list.Set_Selected_Index(static_cast<int>(Event2.Data.Aircraft));
  } else {
    atype2list.Set_Selected_Index(0);
  }

  /*
  **	Optional unit type entry list.
  */
  char ubuf1[kDescSize] = "";
  DropListClass utype1list(kDataUtype1, ubuf1, sizeof(ubuf1),
                           TPF_EFNT | TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char ubuf2[kDescSize] = "";
  DropListClass utype2list(kDataUtype2, ubuf2, sizeof(ubuf2),
                           TPF_EFNT | TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const UnitType uu : magic_enum::enum_values<UnitType>()) {
    utype1list.Add_Item(
        Text_String(UnitTypeClass::As_Reference(uu).Full_Name()));
    utype2list.Add_Item(
        Text_String(UnitTypeClass::As_Reference(uu).Full_Name()));
  }

  if (Event_Needs(Event1.Event) == NEED_UNIT) {
    utype1list.Set_Selected_Index(static_cast<int>(Event1.Data.Unit));
  } else {
    utype1list.Set_Selected_Index(0);
  }
  if (Event_Needs(Event2.Event) == NEED_UNIT) {
    utype2list.Set_Selected_Index(static_cast<int>(Event2.Data.Unit));
  } else {
    utype2list.Set_Selected_Index(0);
  }

  /*
  **	Optional house type entry list.
  */
  char housebuf1[kDescSize] = "";
  DropListClass htype1list(kDataHtype1, housebuf1, sizeof(housebuf1),
                           TPF_EFNT | TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char housebuf2[kDescSize] = "";
  DropListClass htype2list(kDataHtype2, housebuf2, sizeof(housebuf2),
                           TPF_EFNT | TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char housebuf3[kDescSize] = "";
  DropListClass htype3list(kDataHtype3, housebuf3, sizeof(housebuf3),
                           TPF_EFNT | TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char housebuf4[kDescSize] = "";
  DropListClass htype4list(kDataHtype4, housebuf4, sizeof(housebuf4),
                           TPF_EFNT | TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth,
                           kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const HousesType hh : magic_enum::enum_values<HousesType>()) {
    htype1list.Add_Item(HouseTypeClass::As_Reference(hh).IniName);
    htype2list.Add_Item(HouseTypeClass::As_Reference(hh).IniName);
    htype3list.Add_Item(HouseTypeClass::As_Reference(hh).IniName);
    htype4list.Add_Item(HouseTypeClass::As_Reference(hh).IniName);
  }

  if (Event_Needs(Event1.Event) == NEED_HOUSE) {
    htype1list.Set_Selected_Index(static_cast<int>(Event1.Data.House));
  } else {
    htype1list.Set_Selected_Index(0);
  }
  if (Event_Needs(Event2.Event) == NEED_HOUSE) {
    htype2list.Set_Selected_Index(static_cast<int>(Event2.Data.House));
  } else {
    htype2list.Set_Selected_Index(0);
  }
  if (Action_Needs(Action1.Action) == NEED_HOUSE) {
    htype3list.Set_Selected_Index(static_cast<int>(Action1.Data.House));
  } else {
    htype3list.Set_Selected_Index(0);
  }
  if (Action_Needs(Action2.Action) == NEED_HOUSE) {
    htype4list.Set_Selected_Index(static_cast<int>(Action2.Data.House));
  } else {
    htype4list.Set_Selected_Index(0);
  }

  /*
  **	Optional special weapon list.
  */
  char special1[kDescSize] = "";
  DropListClass spc1(kDataSpecial1, special1, sizeof(special1),
                     TPF_EFNT | TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth, kEdHeight,
                     MixArchive::RetrieveData("EBTN-UP.SHP"),
                     MixArchive::RetrieveData("EBTN-DN.SHP"));
  char special2[kDescSize] = "";
  DropListClass spc2(kDataSpecial2, special2, sizeof(special2),
                     TPF_EFNT | TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth, kEdHeight,
                     MixArchive::RetrieveData("EBTN-UP.SHP"),
                     MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const SpecialWeaponType spec :
       magic_enum::enum_values<SpecialWeaponType>()) {
    spc1.Add_Item(SpecialWeaponName.at(spec));
    spc2.Add_Item(SpecialWeaponName.at(spec));
  }
  if (magic_enum::enum_contains(Action1.Data.Special)) {
    spc1.Set_Selected_Index(static_cast<int>(Action1.Data.Special));
  } else {
    spc1.Set_Selected_Index(0);
  }
  if (magic_enum::enum_contains(Action2.Data.Special)) {
    spc2.Set_Selected_Index(static_cast<int>(Action2.Data.Special));
  } else {
    spc2.Set_Selected_Index(0);
  }

  /*
  **	Optional quarry type.
  */
  char quarry1[kDescSize] = "";
  DropListClass qlist1(kDataSpecial1, quarry1, sizeof(quarry1),
                       TPF_EFNT | TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth,
                       kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                       MixArchive::RetrieveData("EBTN-DN.SHP"));
  char quarry2[kDescSize] = "";
  DropListClass qlist2(kDataSpecial2, quarry2, sizeof(quarry2),
                       TPF_EFNT | TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth,
                       kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                       MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const QuarryType q : magic_enum::enum_values<QuarryType>()) {
    qlist1.Add_Item(QuarryName.at(q));
    qlist2.Add_Item(QuarryName.at(q));
  }
  if (magic_enum::enum_contains(Action1.Data.Quarry)) {
    qlist1.Set_Selected_Index(static_cast<int>(Action1.Data.Quarry));
  } else {
    qlist1.Set_Selected_Index(0);
  }
  if (magic_enum::enum_contains(Action2.Data.Quarry)) {
    qlist2.Set_Selected_Index(static_cast<int>(Action2.Data.Quarry));
  } else {
    qlist2.Set_Selected_Index(0);
  }

  /*
  **	Name of this trigger text edit field.
  */
  char namebuf[5] = "";
  EditClass name_edt(kNameEdit, namebuf, sizeof(namebuf),
                     TPF_EFNT | TPF_NOSHADOW, kDialogX + 40, kDialogY + 30, 40,
                     9, EditClass::kAlphanumeric);
  port::SafeCopy(namebuf, IniName);  // Name

  /*
  **	Create the list of house's allowed for trigger.
  */
  char housetext[kDescSize] = "";
  DropListClass housebtn(kButtonHouse, housetext, sizeof(housetext),
                         TPF_EFNT | TPF_NOSHADOW,
                         name_edt.X + name_edt.Width + 20, name_edt.Y, 95,
                         8 * 5, MixArchive::RetrieveData("EBTN-UP.SHP"),
                         MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const HousesType house : magic_enum::enum_values<HousesType>()) {
    housebtn.Add_Item(HouseTypeClass::As_Reference(house).IniName);
  }
  if (House == HOUSE_NONE) {
    House = HOUSE_GOOD;
  }
  housebtn.Set_Selected_Index(static_cast<int>(House));

  /*
  ** Must match order and number of PersistantType specified in
  **	TriggerTypeClass definition.
  */
  char perstext[kDescSize] = "";
  static const char* _perstext[3] = {"Volatile", "Semi-persistent",
                                     "Persistent"};
  DropListClass persbtn(kButtonPersistance, perstext, sizeof(perstext),
                        TPF_EFNT | TPF_NOSHADOW,
                        housebtn.X + housebtn.Width + 20, housebtn.Y, 105,
                        8 * 5, MixArchive::RetrieveData("EBTN-UP.SHP"),
                        MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (auto& i : _perstext) {
    persbtn.Add_Item(i);
  }
  persbtn.Set_Selected_Index(static_cast<int>(IsPersistant));

  /*
  **	This button controls the existence and relationship of a second trigger
  **	event.
  */
  int eventflag = static_cast<int>(EventControl);
  TextButtonClass eventbtn(kButtonEvent, TXT_TRIGGER_JUST_EVENT, kTpfEButton,
                           event1list.X, event1list.Y + 11, 100, 9);

  /*
  **	This button controls the existence of a secondary action.
  */
  bool actionflag = ActionControl != MULTI_ONLY;
  TextButtonClass actionbtn(kButtonAction, TXT_TRIGGER_JUST_ACTION, kTpfEButton,
                            action1list.X, action1list.Y + 11, 100, 9);

  /*
  **	Create the ubiquitous OK and Cancel buttons.
  */
  TextButtonClass okbtn(kButtonOk, TXT_OK, kTpfEButton, kDialogX + 35,
                        kDialogY + kDialogH - 30, 45, 9);
  TextButtonClass cancelbtn(kKbuttoncancel, TXT_CANCEL, kTpfEButton,
                            kDialogX + kDialogW - 80, kDialogY + kDialogH - 30,
                            45, 9);

  /*
  **	Initialize
  */
  Set_Logic_Page(SeenBuff);

  /*
  **	Build the button list
  */
  commands = &okbtn;
  cancelbtn.Add_Tail(*commands);
  event1list.Add_Tail(*commands);
  action1list.Add_Tail(*commands);
  eventbtn.Add_Tail(*commands);
  actionbtn.Add_Tail(*commands);
  name_edt.Add_Tail(*commands);
  persbtn.Add_Tail(*commands);
  housebtn.Add_Tail(*commands);

  /*
  **	Main Processing Loop
  */
  bool display = true;
  bool process = true;
  while (process) {
    /*
    **	Invoke game callback
    */
    ServiceRealTime();

    /*
    **	Refresh display if needed
    */
    if (display /*&& LogicPage->Lock()*/) {
      /*
      **	Display the dialog box
      */
      Hide_Mouse();
      Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
      Draw_Caption(TXT_TRIGGER_EDITOR, kDialogX, kDialogY, kDialogW);

      /*
      **	Draw the captions
      */
      Fancy_Text_Print("Trigger Event:", event1list.X, event1list.Y - 7, scheme,
                       kTBlack, TPF_EFNT | TPF_NOSHADOW);
      Fancy_Text_Print("Action to Perform:", action1list.X, action1list.Y - 7,
                       scheme, kTBlack, TPF_EFNT | TPF_NOSHADOW);
      Fancy_Text_Print("House:", housebtn.X, housebtn.Y - 7, scheme, kTBlack,
                       TPF_EFNT | TPF_NOSHADOW);
      Fancy_Text_Print("Name:", name_edt.X, name_edt.Y - 7, scheme, kTBlack,
                       TPF_EFNT | TPF_NOSHADOW);
      Fancy_Text_Print("Persistence:", persbtn.X, persbtn.Y - 7, scheme,
                       kTBlack, TPF_EFNT | TPF_NOSHADOW);

      if (eventflag == 3) {
        LogicPage->Draw_Line(event1list.X - 1, event1list.Y + 3,
                             event1list.X - 4, event1list.Y + 3, kWhite);
        LogicPage->Draw_Line(event1list.X - 4, event1list.Y + 3,
                             action1list.X - 4, action1list.Y + 3, kWhite);
        LogicPage->Draw_Line(action1list.X - 1, action1list.Y + 3,
                             action1list.X - 4, action1list.Y + 3, kWhite);

        LogicPage->Draw_Line(event2list.X - 1, event2list.Y + 3,
                             event2list.X - 10, event2list.Y + 3, kWhite);
        LogicPage->Draw_Line(event2list.X - 10, event2list.Y + 3,
                             action2list.X - 10, action2list.Y + 3, kWhite);
        LogicPage->Draw_Line(action2list.X - 1, action2list.Y + 3,
                             action2list.X - 10, action2list.Y + 3, kWhite);
      }

      /*
      **	Adjust the button list to match current control settings.
      */
      event2list.Remove();
      switch (eventflag) {
        case 0:
          eventbtn.Set_Text(TXT_TRIGGER_JUST_EVENT);
          break;

        case 1:
          eventbtn.Set_Text(TXT_TRIGGER_AND);
          event2list.Add(*commands);
          break;

        case 2:
          eventbtn.Set_Text(TXT_TRIGGER_OR);
          event2list.Add(*commands);
          break;

        case 3:
          eventbtn.Set_Text(TXT_TRIGGER_LINKED);
          event2list.Add(*commands);
          break;
        default:
          break;
      }

      /*
      **	Prepare the primary event data field.
      */
      htype1list.Remove();
      way1data.Remove();
      event1data.Remove();
      btype1list.Remove();
      itype1list.Remove();
      atype1list.Remove();
      utype1list.Remove();
      ttype1list.Remove();
      switch (Event_Needs(*event1list.Current_Item())) {
        case NEED_HOUSE:
          htype1list.Add(*commands);
          break;

        case NEED_TEAM:
          ttype1list.Add(*commands);
          break;

        case NEED_WAYPOINT:
          way1data.Add(*commands);
          break;

        case NEED_TIME:
        case NEED_NUMBER:
          event1data.Add(*commands);
          break;

        case NEED_STRUCTURE:
          btype1list.Add(*commands);
          break;

        case NEED_INFANTRY:
          itype1list.Add(*commands);
          break;

        case NEED_AIRCRAFT:
          atype1list.Add(*commands);
          break;

        case NEED_UNIT:
          utype1list.Add(*commands);
          break;

        case NeedType::NEED_NONE:
        case NeedType::NEED_THEME:
        case NeedType::NEED_MOVIE:
        case NeedType::NEED_SOUND:
        case NeedType::NEED_SPEECH:
        case NeedType::NEED_TRIGGER:
        case NeedType::NEED_QUARRY:
        case NeedType::NEED_FORMATION:
        case NeedType::NEED_BOOL:
        case NeedType::NEED_SPECIAL:
        case NeedType::NEED_MISSION:
        case NeedType::NEED_HEX_NUMBER:
        default:
          break;
      }

      /*
      **	Prepare the secondary event data field.
      */
      htype2list.Remove();
      way2data.Remove();
      event2data.Remove();
      btype2list.Remove();
      itype2list.Remove();
      atype2list.Remove();
      utype2list.Remove();
      ttype2list.Remove();
      if (commands->Extract_Gadget(kEventList2)) {
        switch (Event_Needs(*event2list.Current_Item())) {
          case NEED_HOUSE:
            htype2list.Add(*commands);
            break;

          case NEED_TEAM:
            ttype2list.Add(*commands);
            break;

          case NEED_WAYPOINT:
            way2data.Add(*commands);
            break;

          case NEED_TIME:
          case NEED_NUMBER:
            event2data.Add(*commands);
            break;

          case NEED_STRUCTURE:
            btype2list.Add(*commands);
            break;

          case NEED_INFANTRY:
            itype2list.Add(*commands);
            break;

          case NEED_AIRCRAFT:
            atype2list.Add(*commands);
            break;

          case NEED_UNIT:
            utype2list.Add(*commands);
            break;

          case NeedType::NEED_NONE:
          case NeedType::NEED_THEME:
          case NeedType::NEED_MOVIE:
          case NeedType::NEED_SOUND:
          case NeedType::NEED_SPEECH:
          case NeedType::NEED_TRIGGER:
          case NeedType::NEED_QUARRY:
          case NeedType::NEED_FORMATION:
          case NeedType::NEED_BOOL:
          case NeedType::NEED_SPECIAL:
          case NeedType::NEED_MISSION:
          case NeedType::NEED_HEX_NUMBER:
          default:
            break;
        }
      }

      /*
      **	Setup the action buttons and associated data entry fields.
      */
      actionbtn.Remove();
      action2list.Remove();
      if (eventflag == 3) {
        action2list.Add(*commands);
      } else {
        actionbtn.Add(*commands);
        if (actionflag) {
          actionbtn.Set_Text(TXT_TRIGGER_AND);
          action2list.Add(*commands);
        } else {
          actionbtn.Set_Text(TXT_TRIGGER_JUST_ACTION);
        }
      }

      qlist1.Remove();
      spc1.Remove();
      htype3list.Remove();
      booltype1list.Remove();
      trtype1list.Remove();
      way3data.Remove();
      action1data.Remove();
      ttype3list.Remove();
      themetype1list.Remove();
      soundtype1list.Remove();
      movietype1list.Remove();
      speechtype1list.Remove();
      switch (Action_Needs(*action1list.Current_Item())) {
        case NEED_MOVIE:
          movietype1list.Add(*commands);
          break;

        case NEED_SPECIAL:
          spc1.Add(*commands);
          break;

        case NEED_HOUSE:
          htype3list.Add(*commands);
          break;

        case NEED_BOOL:
          booltype1list.Add(*commands);
          break;

        case NEED_TRIGGER:
          trtype1list.Add(*commands);
          break;

        case NEED_TEAM:
          ttype3list.Add(*commands);
          break;

        case NEED_NUMBER:
          action1data.Add(*commands);
          break;

        case NEED_WAYPOINT:
          way3data.Add(*commands);
          break;

        case NEED_THEME:
          themetype1list.Add(*commands);
          break;

        case NEED_SOUND:
          soundtype1list.Add(*commands);
          break;

        case NEED_SPEECH:
          speechtype1list.Add(*commands);
          break;

        case NEED_QUARRY:
          qlist1.Add(*commands);
          break;
        case NeedType::NEED_NONE:
        case NeedType::NEED_INFANTRY:
        case NeedType::NEED_UNIT:
        case NeedType::NEED_AIRCRAFT:
        case NeedType::NEED_STRUCTURE:
        case NeedType::NEED_TIME:
        case NeedType::NEED_FORMATION:
        case NeedType::NEED_MISSION:
        case NeedType::NEED_HEX_NUMBER:
        default:
          break;
      }

      qlist2.Remove();
      spc2.Remove();
      htype4list.Remove();
      booltype2list.Remove();
      trtype2list.Remove();
      way4data.Remove();
      action2data.Remove();
      ttype4list.Remove();
      themetype2list.Remove();
      soundtype2list.Remove();
      movietype2list.Remove();
      speechtype2list.Remove();
      if (commands->Extract_Gadget(kActionList2)) {
        switch (Action_Needs(*action2list.Current_Item())) {
          case NEED_MOVIE:
            movietype2list.Add(*commands);
            break;

          case NEED_SPECIAL:
            spc2.Add(*commands);
            break;

          case NEED_HOUSE:
            htype4list.Add(*commands);
            break;

          case NEED_BOOL:
            booltype2list.Add(*commands);
            break;

          case NEED_TRIGGER:
            trtype2list.Add(*commands);
            break;

          case NEED_TEAM:
            ttype4list.Add(*commands);
            break;

          case NEED_NUMBER:
            action2data.Add(*commands);
            break;

          case NEED_WAYPOINT:
            way4data.Add(*commands);
            break;

          case NEED_THEME:
            themetype2list.Add(*commands);
            break;

          case NEED_SOUND:
            soundtype2list.Add(*commands);
            break;

          case NEED_SPEECH:
            speechtype2list.Add(*commands);
            break;

          case NEED_QUARRY:
            qlist2.Add(*commands);
            break;
          case NeedType::NEED_NONE:
          case NeedType::NEED_INFANTRY:
          case NeedType::NEED_UNIT:
          case NeedType::NEED_AIRCRAFT:
          case NeedType::NEED_STRUCTURE:
          case NeedType::NEED_TIME:
          case NeedType::NEED_FORMATION:
          case NeedType::NEED_MISSION:
          case NeedType::NEED_HEX_NUMBER:
          default:
            break;
        }
      }

      /*
      **	Collapse any dropped down list boxes.
      */
      spc1.Collapse();
      spc2.Collapse();
      qlist1.Collapse();
      qlist2.Collapse();
      htype1list.Collapse();
      htype2list.Collapse();
      htype3list.Collapse();
      htype4list.Collapse();
      ttype1list.Collapse();
      ttype2list.Collapse();
      ttype3list.Collapse();
      ttype4list.Collapse();
      btype1list.Collapse();
      btype2list.Collapse();
      utype1list.Collapse();
      utype2list.Collapse();
      itype1list.Collapse();
      itype2list.Collapse();
      atype1list.Collapse();
      atype2list.Collapse();
      trtype1list.Collapse();
      trtype2list.Collapse();
      action1list.Collapse();
      action2list.Collapse();
      event1list.Collapse();
      event2list.Collapse();
      housebtn.Collapse();
      persbtn.Collapse();
      booltype1list.Collapse();
      booltype2list.Collapse();
      themetype1list.Collapse();
      themetype2list.Collapse();
      soundtype1list.Collapse();
      soundtype2list.Collapse();
      movietype1list.Collapse();
      movietype2list.Collapse();
      speechtype1list.Collapse();
      speechtype2list.Collapse();
      commands->Flag_List_To_Redraw();
      Show_Mouse();
      display = false;
      //			LogicPage->Unlock();
    }

    /*
    **	Get user input
    */
    const KeyNumType input = commands->Input();

    /*
    **	Process input
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonEvent):
        eventflag = (eventflag + 1) % 4;
        display = true;
        break;

      case ButtonKey(kButtonAction):
        actionflag = !actionflag;
        display = true;
        break;

      case ButtonKey(kDataSpeech1):
        Speak(VoxType(speechtype1list.Current_Index()));
        display = true;
        break;

      case ButtonKey(kDataSpeech2):
        Speak(VoxType(speechtype2list.Current_Index()));
        display = true;
        break;

      case ButtonKey(kDataSound1):
        Sound_Effect(VocType(soundtype1list.Current_Index()));
        display = true;
        break;

      case ButtonKey(kDataSound2):
        Sound_Effect(VocType(soundtype2list.Current_Index()));
        display = true;
        break;

      /*
      **	Transfer all the necessary values from the edit fields into
      *their *	respective positions within the trigger object.
      */
      case KN_RETURN:
      case ButtonKey(kButtonOk):
        House = HousesType(housebtn.Current_Index());
        IsPersistant = PersistantType(persbtn.Current_Index());
        if (std::string_view(namebuf).empty()) {
          Set_Name("____");
        } else {
          Set_Name(namebuf);
        }

        /*
        **	Primary event specific data retrieval.
        */
        EventControl = MultiStyleType(eventflag);
        Event1.Event = *event1list.Current_Item();
        switch (Event_Needs(Event1.Event)) {
          case NEED_HOUSE:
            Event1.Data.House = HousesType(htype1list.Current_Index());
            break;

          case NEED_TIME:
          case NEED_NUMBER:
            Event1.Data.Value =
                tech::ParseInteger<int>(event1data.Get_Text()).value_or(0);
            break;

          case NEED_STRUCTURE:
            Event1.Data.Structure = StructType(btype1list.Current_Index());
            break;

          case NEED_UNIT:
            Event1.Data.Unit = UnitType(utype1list.Current_Index());
            break;

          case NEED_INFANTRY:
            Event1.Data.Infantry = InfantryType(itype1list.Current_Index());
            break;

          case NEED_AIRCRAFT:
            Event1.Data.Aircraft = AircraftType(atype1list.Current_Index());
            break;

          case NEED_WAYPOINT:
            Event1.Data.Value = toupper(way1[0]) - 'A';
            if (way1[1] != '\0') {
              Event1.Data.Value = (Event1.Data.Value + 1) * 26;
              Event1.Data.Value += toupper(way1[1]) - 'A';
            }
            break;

          case NEED_TEAM:
            Event1.Team = TeamTypeClass::From_Name(ttype1list.Current_Item());
            break;
          case NeedType::NEED_NONE:
          case NeedType::NEED_THEME:
          case NeedType::NEED_MOVIE:
          case NeedType::NEED_SOUND:
          case NeedType::NEED_SPEECH:
          case NeedType::NEED_TRIGGER:
          case NeedType::NEED_QUARRY:
          case NeedType::NEED_FORMATION:
          case NeedType::NEED_BOOL:
          case NeedType::NEED_SPECIAL:
          case NeedType::NEED_MISSION:
          case NeedType::NEED_HEX_NUMBER:
          default:
            break;
        }

        /*
        **	Secondary event specific data retrieval.
        */
        Event2.Event = *event2list.Current_Item();
        switch (Event_Needs(Event2.Event)) {
          case NEED_HOUSE:
            Event2.Data.House = HousesType(htype2list.Current_Index());
            break;

          case NEED_TIME:
          case NEED_NUMBER:
            Event2.Data.Value =
                tech::ParseInteger<int>(event2data.Get_Text()).value_or(0);
            break;

          case NEED_STRUCTURE:
            Event2.Data.Structure = StructType(btype2list.Current_Index());
            break;

          case NEED_UNIT:
            Event2.Data.Unit = UnitType(utype2list.Current_Index());
            break;

          case NEED_INFANTRY:
            Event2.Data.Infantry = InfantryType(itype2list.Current_Index());
            break;

          case NEED_AIRCRAFT:
            Event2.Data.Aircraft = AircraftType(atype2list.Current_Index());
            break;

          case NEED_WAYPOINT:
            Event2.Data.Value = toupper(way2[0]) - 'A';
            if (way2[1] != '\0') {
              Event2.Data.Value = (Event2.Data.Value + 1) * 26;
              Event2.Data.Value += toupper(way2[1]) - 'A';
            }
            break;

          case NEED_TEAM:
            Event2.Team = TeamTypeClass::As_Pointer(ttype2list.Current_Item());
            break;
          case NeedType::NEED_NONE:
          case NeedType::NEED_THEME:
          case NeedType::NEED_MOVIE:
          case NeedType::NEED_SOUND:
          case NeedType::NEED_SPEECH:
          case NeedType::NEED_TRIGGER:
          case NeedType::NEED_QUARRY:
          case NeedType::NEED_FORMATION:
          case NeedType::NEED_BOOL:
          case NeedType::NEED_SPECIAL:
          case NeedType::NEED_MISSION:
          case NeedType::NEED_HEX_NUMBER:
          default:
            break;
        }

        /*
        **	Primary action data retrieval.
        */
        ActionControl = MultiStyleType(actionflag);
        Action1.Action = *action1list.Current_Item();
        switch (Action_Needs(Action1.Action)) {
          case NEED_SPECIAL:
            Action1.Data.Special = SpecialWeaponType(spc1.Current_Index());
            break;

          case NEED_HOUSE:
            Action1.Data.House = HousesType(htype3list.Current_Index());
            break;

          case NEED_TRIGGER:
            Action1.Trigger =
                TriggerTypeClass::From_Name(trtype1list.Current_Item());
            break;

          case NEED_TEAM:
            Action1.Team = TeamTypeClass::From_Name(ttype3list.Current_Item());
            break;

          case NEED_NUMBER:
            Action1.Data.Value =
                tech::ParseInteger<int>(action1data.Get_Text()).value_or(0);
            break;

          case NEED_WAYPOINT:
            Action1.Data.Value = toupper(way3[0]) - 'A';
            if (way3[1] != '\0') {
              Action1.Data.Value = (Action1.Data.Value + 1) * 26;
              Action1.Data.Value += toupper(way3[1]) - 'A';
            }
            break;

          case NEED_BOOL:
            Action1.Data.Bool = booltype1list.Current_Index() != 0;
            break;

          case NEED_THEME:
            Action1.Data.Theme = ThemeType(themetype1list.Current_Index());
            break;

          case NEED_SOUND:
            Action1.Data.Sound = VocType(soundtype1list.Current_Index());
            break;

          case NEED_MOVIE:
            Action1.Data.Movie = VQType(movietype1list.Current_Index());
            break;

          case NEED_SPEECH:
            Action1.Data.Speech = VoxType(speechtype1list.Current_Index());
            break;

          case NEED_QUARRY:
            Action1.Data.Quarry = QuarryType(qlist1.Current_Index());
            break;
          case NeedType::NEED_NONE:
          case NeedType::NEED_INFANTRY:
          case NeedType::NEED_UNIT:
          case NeedType::NEED_AIRCRAFT:
          case NeedType::NEED_STRUCTURE:
          case NeedType::NEED_TIME:
          case NeedType::NEED_FORMATION:
          case NeedType::NEED_MISSION:
          case NeedType::NEED_HEX_NUMBER:
          default:
            break;
        }

        /*
        **	Secondary action data retrieval.
        */
        Action2.Action = *action2list.Current_Item();
        switch (Action_Needs(Action2.Action)) {
          case NEED_SPECIAL:
            Action2.Data.Special = SpecialWeaponType(spc2.Current_Index());
            break;

          case NEED_HOUSE:
            Action2.Data.House = HousesType(htype4list.Current_Index());
            break;

          case NEED_TRIGGER:
            Action2.Trigger =
                TriggerTypeClass::From_Name(trtype2list.Current_Item());
            break;

          case NEED_TEAM:
            Action2.Team = TeamTypeClass::From_Name(ttype4list.Current_Item());
            break;

          case NEED_NUMBER:
            Action2.Data.Value =
                tech::ParseInteger<int>(action2data.Get_Text()).value_or(0);
            break;

          case NEED_WAYPOINT:
            Action2.Data.Value = toupper(way4[0]) - 'A';
            if (way4[1] != '\0') {
              Action2.Data.Value = (Action2.Data.Value + 1) * 26;
              Action2.Data.Value += toupper(way4[1]) - 'A';
            }
            break;

          case NEED_BOOL:
            Action2.Data.Bool = booltype2list.Current_Index() != 0;
            break;

          case NEED_THEME:
            Action2.Data.Theme = ThemeType(themetype2list.Current_Index());
            break;

          case NEED_MOVIE:
            Action2.Data.Movie = VQType(movietype2list.Current_Index());
            break;

          case NEED_SOUND:
            Action2.Data.Sound = VocType(soundtype2list.Current_Index());
            break;

          case NEED_SPEECH:
            Action2.Data.Speech = VoxType(speechtype2list.Current_Index());
            break;

          case NEED_QUARRY:
            Action2.Data.Quarry = QuarryType(qlist1.Current_Index());
            break;
          case NeedType::NEED_NONE:
          case NeedType::NEED_INFANTRY:
          case NeedType::NEED_UNIT:
          case NeedType::NEED_AIRCRAFT:
          case NeedType::NEED_STRUCTURE:
          case NeedType::NEED_TIME:
          case NeedType::NEED_FORMATION:
          case NeedType::NEED_MISSION:
          case NeedType::NEED_HEX_NUMBER:
          default:
            break;
        }
        return true;

      case KN_ESC:
      case ButtonKey(kKbuttoncancel):
        process = false;
        [[fallthrough]];

      /*
      **	Always signal a redraw if any of the buttons were touched. This
      **	can be determined by examining the button bit flag in the input
      **	return value.
      */
      default:
        if (input & KN_BUTTON) {
          display = true;
        }
        break;
    }
  }
  return false;
}

/***********************************************************************************************
 * TriggerTypeClass::Description -- Build a text description of the trigger
 *type.              *
 *                                                                                             *
 *    This will build a (static) text description of the trigger type. Use this
 *description    * when displaying this trigger in a list box. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to a one line text description of this
 *trigger.             *
 *                                                                                             *
 * WARNINGS:   The pointer returned actually points to a static buffer. The
 *pointer is only    * valid until this routine is called again. *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
const char* TriggerTypeClass::Description() const {
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    static char _buffer[128];

    char special = 0;
    switch (EventControl) {
      case MULTI_AND:
        special = '&';
        break;

      case MULTI_OR:
        special = '|';
        break;

      case MULTI_LINKED:
        special = '=';
        break;

      case MultiStyleType::MULTI_ONLY:
      default:
        special = '.';
        break;
    }

    char special2 = '.';
    if (ActionControl == MULTI_AND) {
      special2 = '&';
    }

    char tbuf[32];
    const char* added = "";
    switch (Event_Needs(Event1.Event)) {
      case NEED_NUMBER:
        absl::SNPrintF(tbuf, sizeof(tbuf), "%d",
                       static_cast<int>(Event1.Data.Value));
        added = tbuf;
        break;

      case NEED_UNIT:
        added = Text_String(
            UnitTypeClass::As_Reference(Event1.Data.Unit).Full_Name());
        break;

      case NEED_AIRCRAFT:
        added = Text_String(
            AircraftTypeClass::As_Reference(Event1.Data.Aircraft).Full_Name());
        break;

      case NEED_STRUCTURE:
        added = Text_String(
            BuildingTypeClass::As_Reference(Event1.Data.Structure).Full_Name());
        break;

      case NEED_INFANTRY:
        added = Text_String(
            InfantryTypeClass::As_Reference(Event1.Data.Infantry).Full_Name());
        break;

      case NEED_WAYPOINT:
        if (Event1.Data.Value < 26) {
          absl::SNPrintF(tbuf, sizeof(tbuf), "'%c'",
                         static_cast<int>(Event1.Data.Value) + 'A');
        } else {
          absl::SNPrintF(tbuf, sizeof(tbuf), "'%c%c'",
                         (Event1.Data.Value / 26) + 'A' - 1,
                         (Event1.Data.Value % 26) + 'A');
        }
        added = tbuf;
        break;

      case NeedType::NEED_NONE:
      case NeedType::NEED_THEME:
      case NeedType::NEED_MOVIE:
      case NeedType::NEED_SOUND:
      case NeedType::NEED_SPEECH:
      case NeedType::NEED_TRIGGER:
      case NeedType::NEED_TEAM:
      case NeedType::NEED_HOUSE:
      case NeedType::NEED_TIME:
      case NeedType::NEED_QUARRY:
      case NeedType::NEED_FORMATION:
      case NeedType::NEED_BOOL:
      case NeedType::NEED_SPECIAL:
      case NeedType::NEED_MISSION:
      case NeedType::NEED_HEX_NUMBER:
      default:
        break;
    }

    /*
    **	Persistence indicator value.
    */
    char pers = 'V';
    if (IsPersistant == SEMIPERSISTANT) {
      pers = 'S';
    }
    if (IsPersistant == PERSISTANT) {
      pers = 'P';
    }

    absl::SNPrintF(_buffer, sizeof(_buffer), "%4.4s\t %s %c%c%c  %s%s", IniName,
                   HouseTypeClass::As_Reference(House).Suffix, pers, special,
                   special2, Name_From_Event(Event1.Event), added);
    return _buffer;
  }
  return "";
}

/***********************************************************************************************
 * TriggerTypeClass::Attaches_To -- Determines what trigger can attach to. *
 *                                                                                             *
 *    This routine will examine the trigger events and return with a composit
 *bitfield that    * indicates what this trigger can be attached to. This is
 *used for trigger placement       * and logic processing. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with AttachType bitfield representing what this trigger can
 *be attached    * to. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/30/1995 JLB : Created. *
 *=============================================================================================*/
AttachType TriggerTypeClass::Attaches_To() const {
  AttachType attach = ::Attaches_To(Event1.Event);

  if (EventControl != MULTI_ONLY) {
    attach = attach | ::Attaches_To(Event2.Event);
  }
  return attach;
}

/***********************************************************************************************
 * TriggerTypeClass::Read_INI -- reads triggers from the INI file *
 *                                                                                             *
 *    INI entry format: * Triggername = Eventname, Actionname, Data, Housename,
 *TeamName, IsPersistant           *
 *                                                                                             *
 * This routine reads in the triggers & creates them. Then, other classes can *
 * get pointers to the triggers they're linked to. *
 *                                                                                             *
 * The routine relies on the TeamTypeClasses already being loaded so it can
 *resolve            * references to teams in this function. *
 *                                                                                             *
 * Cell Trigger pointers & IsTrigger flags are set in DisplayClass::Read_INI(),
 ** and cleared in the Map::Init() routine (which clears all cell objects to
 *0's).              *
 *                                                                                             *
 * Object's pointers are set in: * InfantryClass::Read_INI() *
 *      BuildingClass::Read_INI() * UnitClass::Read_INI() *
 *      TerrainClass::Read_INI() * The object trigger pointers are cleared in
 *the ObjectClass constructor.                     *
 *                                                                                             *
 * The House's EMSListOf triggers is set in this routine, and cleared in the *
 * HouseClass::Init() routine. *
 *                                                                                             *
 * INPUT: * buffer      buffer to hold the INI data *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * This function must be called before any other class's Read_INI. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 BR : Created. *
 *=============================================================================================*/
void TriggerTypeClass::Read_INI(CCINIClass& ini) {
  char buf[128];

  const int len = ini.Entry_Count(INI_Name());
  for (int index = 0; index < len; index++) {
    const char* entry = ini.Get_Entry(INI_Name(), index);

    /*
    **	Create a new trigger.
    */
    auto* trigger = new TriggerTypeClass();  // Working trigger pointer.

    /*
    **	Get the trigger entry.
    */
    ini.Get_String(INI_Name(), entry, nullptr, buf, sizeof(buf));

    /*
    **	Fill in the trigger.
    */
    trigger->Fill_In(entry, buf);
  }

  if (NewINIFormat < 2) {
    /*
    **	Fix up the self-referential trigger pointers.
    */
    for (int trig_index = 0; trig_index < TriggerTypes.Count(); trig_index++) {
      TriggerTypeClass* indexed_trigger = TriggerTypes.Ptr(trig_index);

      for (TActionClass* action :
           {&indexed_trigger->Action1, &indexed_trigger->Action2}) {
        if (!action->PendingTriggerName.empty()) {
          action->Trigger = From_Name(action->PendingTriggerName.c_str());
          action->PendingTriggerName.clear();
        }
      }
    }
  }
}

/***********************************************************************************************
 * TriggerTypeClass::Fill_In -- fills in trigger from the given INI entry *
 *                                                                                             *
 * This routine fills in the given trigger with the given name, and values from
 ** the given INI entry. *
 *                                                                                             *
 * (This routine is used by the scenario editor, to import teams from the
 *MASTER.INI file.)    *
 *                                                                                             *
 *    INI entry format: * Triggername = Eventname, Actionname, Data, Housename,
 *TeamName, IsPersistant           *
 *                                                                                             *
 * INPUT: * name      mnemonic for the desired trigger * entry      INI entry to
 *parse                                                          *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 BR : Created. *
 *=============================================================================================*/
void TriggerTypeClass::Fill_In(const char* name, char* entry) {
  assert(TriggerTypes.ID(this) == ID);

  /*
  **	Set its name.
  */
  Set_Name(name);

  port::Tokenizer tokens(entry, ",");
  IsPersistant = static_cast<PersistantType>(
      tech::ParseInteger<int>(tokens.Next()).value_or(0));
  House = static_cast<HousesType>(
      tech::ParseInteger<int>(tokens.Next()).value_or(0));
  EventControl = static_cast<MultiStyleType>(
      tech::ParseInteger<int>(tokens.Next()).value_or(0));
  ActionControl = static_cast<MultiStyleType>(
      tech::ParseInteger<int>(tokens.Next()).value_or(0));

  Event1.Read_INI(tokens);
  Event2.Read_INI(tokens);
  Action1.Read_INI(tokens);
  Action2.Read_INI(tokens);
}

/***********************************************************************************************
 * TriggerTypeClass::Write_INI -- Stores all trigger types to the INI database
 *specified.      *
 *                                                                                             *
 *    This routine will write out all trigger type objects to the INI database.
 *Any existing   * trigger types in the database will be cleared out. *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database to have the trigger types
 *added.            *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
void TriggerTypeClass::Write_INI(CCINIClass& ini) {
  ini.Clear("Triggers");
  ini.Clear(INI_Name());

  /*
  **	Now write all the trigger data out
  */
  for (int index = 0; index < TriggerTypes.Count(); index++) {
    //	for (int index = TriggerTypes.Count()-1; index >= 0; index--) {
    std::string buf;
    TriggerTypeClass* trigger = TriggerTypes.Ptr(index);

    trigger->Build_INI_Entry(buf);
    ini.Put_String(INI_Name(), trigger->IniName, buf.c_str());
  }
}

/***********************************************************************************************
 * TriggerTypeClass::Build_INI_Entry -- Construct the INI entry into the buffer
 *specified.     *
 *                                                                                             *
 *    This low level routine will take the information in this trigger type and
 *store it       * into a buffer such that the resultant string can be stored
 *into an INI database for      * later retrieval. *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the buffer to store the INI entry string. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Be sure the buffer is big enough. Usually 128 bytes is more than
 *sufficient.    *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
// LLVM 23 treats assignment as invalidating the string reference itself. No
// pointer, reference or iterator into its old character storage is retained.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void TriggerTypeClass::Build_INI_Entry(std::string& buffer) const {
  /*
  ** Build the root portion of the trigger event.
  */
  buffer =
      std::format("{},{},{},{},", std::to_underlying(IsPersistant),
                  std::to_underlying(House), std::to_underlying(EventControl),
                  std::to_underlying(ActionControl));

  /*
  **	Append the event and action values.
  */
  Event1.Build_INI_Entry(buffer);
  buffer += ',';

  Event2.Build_INI_Entry(buffer);
  buffer += ',';

  Action1.Build_INI_Entry(buffer);
  buffer += ',';

  Action2.Build_INI_Entry(buffer);
}

/***********************************************************************************************
 * TriggerTypeClass::Draw_It -- Draws this trigger as if it were a line in a
 *list box.         *
 *                                                                                             *
 *    This routine is called when triggers are assigned to a list box and then
 *must be drawn.  * It will display an identifying text string with as much
 *information as is useful.        *
 *                                                                                             *
 * INPUT:   index    -- The index number of this line in the list box. *
 *                                                                                             *
 *          x,y      -- The pixel coordinate of the upper left corner of the
 *text box.         *
 *                                                                                             *
 *          width,height   -- The dimensions of the text box to display the
 *description in.    *
 *                                                                                             *
 *          selected -- Is this a selected line? If so, then it should be
 *highlighted.         *
 *                                                                                             *
 *          flags    -- The text print flags to use to display this text string.
 **
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
void TriggerTypeClass::Draw_It(int /*unused*/, int x, int y, int width,
                               int height, bool selected,
                               TextPrintType flags) const {
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    RemapControlType* scheme = GadgetClass::Get_Color_Scheme();
    static const int _tabs[] = {13, 40};
    if (Is_Font(flags, TPF_6PT_GRAD) || Is_Font(flags, TPF_EFNT)) {
      if (selected) {
        flags = flags | TPF_BRIGHT_COLOR;
        LogicPage->Fill_Rect(x, y, x + width - 1, y + height - 1,
                             scheme->Shadow);
      } else {
        if (!base::Any(flags & TPF_USE_GRAD_PAL)) {
          flags = flags | TPF_MEDIUM_COLOR;
        }
      }

      Conquer_Clip_Text_Print(Description(), x, y, scheme, kTBlack, flags,
                              width, _tabs);
    } else {
      Conquer_Clip_Text_Print(Description(), x, y,
                              (selected ? &ColorRemaps.at(PCOLOR_DIALOG_BLUE)
                                        : &ColorRemaps.at(PCOLOR_GREY)),
                              kTBlack, flags, width, _tabs);
    }
  }
}

/***********************************************************************************************
 * TriggerTypeClass::Init -- Initialize the trigger type object management
 *system.             *
 *                                                                                             *
 *    This routine should be called to initialize the trigger type object
 *system. It should    * be called when clearing out a scenario. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   All trigger types will be destroyed by this routine. *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
void TriggerTypeClass::Init() { TriggerTypes.Free_All(); }

/***********************************************************************************************
 * TriggerTypeClass::From_Name -- Convert an ASCII name into a trigger type
 *pointer.           *
 *                                                                                             *
 *    Given just an ASCII representation of the trigger type, this routine will
 *return with    * a pointer to the trigger type it refers to. Typical use of
 *this is when parsing          * scenario INI files. *
 *                                                                                             *
 * INPUT:   name  -- Pointer to the name to use to identify the trigger type
 *class object to   * be looked up. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the trigger type class object that matches
 *the name      * specified. If no match could be found, then NULL is returned.
 **
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
TriggerTypeClass* TriggerTypeClass::From_Name(const char* name) {
  if (name != nullptr) {
    for (int index = 0; index < TriggerTypes.Count(); index++) {
      if (port::CompareIgnoreCase(TriggerTypes.Ptr(index)->Name(), name) == 0) {
        return TriggerTypes.Ptr(index);
      }
    }
  }
  return nullptr;
}
