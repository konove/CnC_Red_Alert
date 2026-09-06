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

/* $Header: /CounterStrike/IOOBJ.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : IOOBJ.CPP *
 *                                                                                             *
 *                   Programmer : Bill Randolph *
 *                                                                                             *
 *                   Start Date : January 16, 1995 *
 *                                                                                             *
 *                  Last Update : May 13, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * BulletClass::Code_Pointers -- codes class's pointers for
 *load/save                        * BulletClass::Decode_Pointers -- decodes
 *pointers for load/save                            * CargoClass::Code_Pointers
 *-- codes class's pointers for load/save                         *
 *   CargoClass::Decode_Pointers -- decodes pointers for load/save *
 *   FactoryClass::Code_Pointers -- codes class's pointers for load/save *
 *   FactoryClass::Decode_Pointers -- decodes pointers for load/save *
 *   FootClass::Code_Pointers -- codes class's pointers for load/save *
 *   FootClass::Decode_Pointers -- decodes pointers for load/save *
 *   HouseClass::Code_Pointers -- codes class's pointers for load/save *
 *   HouseClass::Decode_Pointers -- decodes pointers for load/save *
 *   LayerClass::Code_Pointers -- codes class's pointers for load/save *
 *   LayerClass::Decode_Pointers -- decodes pointers for load/save *
 *   LayerClass::Load -- Reads from a save game file. * LayerClass::Save --
 *Write to a save game file.                                            *
 *   ObjectClass::Code_Pointers -- codes class's pointers for load/save *
 *   ObjectClass::Decode_Pointers -- decodes pointers for load/save *
 *   RadioClass::Code_Pointers -- codes class's pointers for load/save *
 *   RadioClass::Decode_Pointers -- decodes pointers for load/save *
 *   ReinforcementClass::Code_Pointers -- codes class's pointers for load/save *
 *   ReinforcementClass::Decode_Pointers -- decodes pointers for load/save *
 *   ScoreClass::Code_Pointers -- codes class's pointers for load/save *
 *   ScoreClass::Decode_Pointers -- decodes pointers for load/save *
 *   TeamClass::Code_Pointers -- codes class's pointers for load/save *
 *   TeamClass::Decode_Pointers -- decodes pointers for load/save *
 *   TeamTypeClass::Code_Pointers -- codes class's pointers for load/save *
 *   TeamTypeClass::Decode_Pointers -- decodes pointers for load/save *
 *   TechnoClass::Code_Pointers -- codes class's pointers for load/save *
 *   TechnoClass::Decode_Pointers -- decodes pointers for load/save *
 *   TriggerClass::Code_Pointers -- codes class's pointers for load/save *
 *   TriggerClass::Decode_Pointers -- decodes pointers for load/save *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */
#include <bit>
#include <cassert>
#include <cstdint>

#include "ra/bullet.h"
#include "ra/cargo.h"
#include "ra/defines.h"
#include "ra/factory.h"
#include "ra/serialize.h"
#include "ra/foot.h"
#include "ra/house.h"
#include "ra/jshell.h"
#include "ra/layer.h"
#include "ra/mission.h"
#include "ra/object.h"
#include "ra/radio.h"
#include "ra/score.h"
#include "ra/taction.h"
#include "ra/target.h"
#include "ra/team.h"
#include "ra/teamtype.h"
#include "ra/techno.h"
#include "ra/trigger.h"
#include "ra/trigtype.h"
#include "ra/type.h"
#include "ra/vector.h"
#include "tech/ftimer.h"
#include "tech/pipe.h"
#include "tech/straw.h"

template <class Archive>
void TeamMemberClass::Serialize(Archive& ar) {
  ar(Quantity, TechnoTypePtr(Class));
}
template void TeamMemberClass::Serialize(ArchiveWriter&);
template void TeamMemberClass::Serialize(ArchiveReader&);

template <class Archive>
void TeamTypeClass::Serialize(Archive& ar) {
  AbstractTypeClass::Serialize(ar);
  bool is_active = IsActive;
  bool is_round_about = IsRoundAbout;
  bool is_suicide = IsSuicide;
  bool is_autocreate = IsAutocreate;
  bool is_prebuilt = IsPrebuilt;
  bool is_reinforcable = IsReinforcable;
  bool is_transient = IsTransient;
  ar(is_active, is_round_about, is_suicide, is_autocreate, is_prebuilt,
     is_reinforcable, is_transient, RecruitPriority, InitNum, MaxAllowed, Fear,
     House, Trigger, Origin, Number, MissionCount, MissionList, ClassCount,
     Members);
  IsActive = is_active;
  IsRoundAbout = is_round_about;
  IsSuicide = is_suicide;
  IsAutocreate = is_autocreate;
  IsPrebuilt = is_prebuilt;
  IsReinforcable = is_reinforcable;
  IsTransient = is_transient;
}
template void TeamTypeClass::Serialize(ArchiveWriter&);
template void TeamTypeClass::Serialize(ArchiveReader&);

template <class Archive>
void TeamClass::Serialize(Archive& ar) {
  AbstractClass::Serialize(ar);
  bool is_forced_active = IsForcedActive;
  bool is_has_been = IsHasBeen;
  bool is_full_strength = IsFullStrength;
  bool is_under_strength = IsUnderStrength;
  bool is_reforming = IsReforming;
  bool is_lagging = IsLagging;
  bool is_altered = IsAltered;
  bool just_altered = JustAltered;
  bool is_moving = IsMoving;
  bool is_next_mission = IsNextMission;
  bool is_leave_map = IsLeaveMap;
  bool suspended = Suspended;
  ar(Class, House, is_forced_active, is_has_been, is_full_strength,
     is_under_strength, is_reforming, is_lagging, is_altered, just_altered,
     is_moving, is_next_mission, is_leave_map, suspended, Zone, ClosestMember,
     MissionTarget, Target, Total, Risk, Formation, SuspendTimer, Trigger,
     CurrentMission, TimeOut, ObjectPtr(Member), Quantity);
  IsForcedActive = is_forced_active;
  IsHasBeen = is_has_been;
  IsFullStrength = is_full_strength;
  IsUnderStrength = is_under_strength;
  IsReforming = is_reforming;
  IsLagging = is_lagging;
  IsAltered = is_altered;
  JustAltered = just_altered;
  IsMoving = is_moving;
  IsNextMission = is_next_mission;
  IsLeaveMap = is_leave_map;
  Suspended = suspended;
}
template void TeamClass::Serialize(ArchiveWriter&);
template void TeamClass::Serialize(ArchiveReader&);

template <class Archive>
void TriggerClass::Serialize(Archive& ar) {
  bool is_active = IsActive;
  ar(RTTI, ID, Class, Event1, Event2, is_active, AttachCount, Cell);
  IsActive = is_active;
}
template void TriggerClass::Serialize(ArchiveWriter&);
template void TriggerClass::Serialize(ArchiveReader&);

template <class Archive>
void TriggerTypeClass::Serialize(Archive& ar) {
  AbstractTypeClass::Serialize(ar);
  bool is_active = IsActive;
  ar(is_active, IsPersistant, House, Event1, Event2, EventControl, Action1,
     Action2, ActionControl);
  IsActive = is_active;
}
template void TriggerTypeClass::Serialize(ArchiveWriter&);
template void TriggerTypeClass::Serialize(ArchiveReader&);

/***********************************************************************************************
 * BulletClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void BulletClass::Code_Pointers() {
  /*
  **	Code 'Payback'
  */
  if (Payback) {
    Payback = (TechnoClass*)Payback->As_Target();
  }

  /*
  **	Chain to parent
  */
  ObjectClass::Code_Pointers();
}

/***********************************************************************************************
 * BulletClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void BulletClass::Decode_Pointers() {
  /*
  **	Decode 'Payback'
  */
  if (Payback) {
    Payback = As_Techno(static_cast<TARGET>((intptr_t)Payback));
    assert(Payback != nullptr);
  }

  /*
  **	Chain to parent
  */
  ObjectClass::Decode_Pointers();
}

template <class Archive>
void FactoryClass::Serialize(Archive& ar) {
  StageClass::Serialize(ar);
  bool is_active = IsActive;
  bool is_suspended = IsSuspended;
  bool is_different = IsDifferent;
  ar(RTTI, ID, is_active, is_suspended, is_different, Balance, OriginalBalance,
     ObjectPtr(Object), SpecialItem, House);
  IsActive = is_active;
  IsSuspended = is_suspended;
  IsDifferent = is_different;
}
template void FactoryClass::Serialize(ArchiveWriter&);
template void FactoryClass::Serialize(ArchiveReader&);

/***********************************************************************************************
 * LayerClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool LayerClass::Load(Straw& file) {
  /*
  **	Read # elements in the layer
  */
  int32_t count;
  if (file.Get(&count, sizeof(count)) != sizeof(count)) {
    return false;
  }

  /*
  **	Clear the array
  */
  Clear();

  /*
  **	Read in all array elements
  */
  for (int index = 0; index < count; index++) {
    ObjectClass* ptr;
    if (file.Get(static_cast<void*>(&ptr), sizeof(ObjectClass*)) !=
        sizeof(ObjectClass*)) {
      return false;
    }
    Add(ptr);
  }

  return true;
}

/***********************************************************************************************
 * LayerClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool LayerClass::Save(Pipe& file) const {
  /*
  **	Save # array elements
  */
  // Same width as Load reads; base::ssize is 8 bytes here and Load read 4.
  int32_t count = static_cast<int32_t>(Count());
  file.Put(&count, sizeof(count));

  /*
  **	Save all elements
  */
  for (int index = 0; index < count; index++) {
    ObjectClass* ptr = (*this)[index];
    file.Put(static_cast<const void*>(&ptr), sizeof(ObjectClass*));
  }

  return true;
}

/***********************************************************************************************
 * LayerClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void LayerClass::Code_Pointers() {
  for (int index = 0; index < Count(); index++) {
    ObjectClass* obj = (*this)[index];
    assert(obj != nullptr);
    (*this)[index] = (ObjectClass*)obj->As_Target();
  }
}

/***********************************************************************************************
 * LayerClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void LayerClass::Decode_Pointers() {
  for (int index = 0; index < Count(); index++) {
    TARGET target = static_cast<TARGET>((intptr_t)(*this)[index]);
    (*this)[index] = As_Object(target);
    assert((*this)[index] != nullptr);
  }
}

/***********************************************************************************************
 * HouseClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void HouseClass::Code_Pointers() {}

/***********************************************************************************************
 * HouseClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void HouseClass::Decode_Pointers() {
  /*
  ** Re-assign the house's remap table (for multiplayer game loads)
  ** Loading the house from disk will have over-written the house's RemapTable,
  *so
  ** Init_Data() is called to reset it to a valid pointer.
  */
  Init_Data(RemapColor, ActLike, static_cast<int>(Credits));
}

/***********************************************************************************************
 * ScoreClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void ScoreClass::Code_Pointers() { RealTime.Stop(); }

/***********************************************************************************************
 * ScoreClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void ScoreClass::Decode_Pointers() { RealTime.Start(); }

/***********************************************************************************************
 * FootClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void FootClass::Code_Pointers() {
  if (Member != nullptr && Member->IsActive) {
    Member = (FootClass*)Member->As_Target();
  } else {
    Member = (FootClass*)kTargetNone;
  }

  TechnoClass::Code_Pointers();
}

/***********************************************************************************************
 * FootClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void FootClass::Decode_Pointers() {
  if (static_cast<TARGET>((intptr_t)Member) != kTargetNone) {
    Member = dynamic_cast<FootClass*>(As_Techno((TARGET)(intptr_t)Member));
    assert(Member != nullptr);
  }

  TechnoClass::Decode_Pointers();
}

/***********************************************************************************************
 * RadioClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void RadioClass::Code_Pointers() {
  /*
  **	Code 'Radio'
  */
  if (Radio) {
    Radio = (RadioClass*)Radio->As_Target();
  }

  MissionClass::Code_Pointers();
}

/***********************************************************************************************
 * RadioClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void RadioClass::Decode_Pointers() {
  /*
  **	Decode 'Radio'
  */
  if (Radio) {
    Radio = As_Techno(static_cast<TARGET>((intptr_t)Radio));
    assert(Radio != nullptr);
  }

  MissionClass::Decode_Pointers();
}

/***********************************************************************************************
 * TechnoClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void TechnoClass::Code_Pointers() {
  CargoClass::Code_Pointers();
  RadioClass::Code_Pointers();
}

/***********************************************************************************************
 * TechnoClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void TechnoClass::Decode_Pointers() {
  CargoClass::Decode_Pointers();
  RadioClass::Decode_Pointers();
}

/***********************************************************************************************
 * CargoClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void CargoClass::Code_Pointers() {
  /*
  **	Code 'CargoHold'
  */
  if (CargoHold) {
    CargoHold = (FootClass*)CargoHold->As_Target();
  }
}

/***********************************************************************************************
 * CargoClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void CargoClass::Decode_Pointers() {
  /*
  **	Decode 'CargoHold'
  */
  if (CargoHold) {
    CargoHold =
        dynamic_cast<FootClass*>(As_Techno((TARGET)(intptr_t)CargoHold));
    assert(CargoHold != nullptr);
  }
}

/***********************************************************************************************
 * ObjectClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void ObjectClass::Code_Pointers() {
  if (Next) {
    Next = (ObjectClass*)Next->As_Target();
  }
}

/***********************************************************************************************
 * ObjectClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void ObjectClass::Decode_Pointers() {
  if (Next) {
    Next = As_Object(static_cast<TARGET>(std::bit_cast<intptr_t>(Next)));
    assert(Next != nullptr);
  }
}
