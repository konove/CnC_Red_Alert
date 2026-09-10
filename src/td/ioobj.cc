/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\ioobj.cpv   2.18   16 Oct 1995 16:51:22
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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
 *                  Last Update : January 16, 1995   [BR] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * All object-related loading/saving routines should go in this module, so it
 *can be overlayed.*
 *---------------------------------------------------------------------------------------------*
 * Functions: * TeamTypeClass::Serialize -- Read/write team definitions. *
 *   TeamClass::Serialize -- Read/write team state. *
 *   TriggerClass::Serialize -- Read/write saved fields. *
 *   AircraftClass::Load -- Reads from a save game file. * AircraftClass::Save
 *-- Write to a save game file.                                         *
 *   AircraftClass::Code_Pointers -- codes class's pointers for load/save *
 *   AircraftClass::Decode_Pointers -- decodes pointers for load/save *
 *   AnimClass::Load -- Reads from a save game file. * AnimClass::Save -- Write
 *to a save game file.                                             *
 *   AnimClass::Code_Pointers -- codes class's pointers for load/save *
 *   AnimClass::Decode_Pointers -- decodes pointers for load/save *
 *   BuildingClass::Load -- Reads from a save game file. * BuildingClass::Save
 *-- Write to a save game file.                                         *
 *   BuildingClass::Code_Pointers -- codes class's pointers for load/save *
 *   BuildingClass::Decode_Pointers -- decodes pointers for load/save *
 *   BulletClass::Load -- Reads from a save game file. * BulletClass::Save --
 *Write to a save game file.                                           *
 *   BulletClass::Code_Pointers -- codes class's pointers for load/save *
 *   BulletClass::Decode_Pointers -- decodes pointers for load/save *
 *   InfantryClass::Load -- Reads from a save game file. * InfantryClass::Save
 *-- Write to a save game file.                                         *
 *   InfantryClass::Code_Pointers -- codes class's pointers for load/save *
 *   InfantryClass::Decode_Pointers -- decodes pointers for load/save *
 *   OverlayClass::Load -- Reads from a save game file. * OverlayClass::Save --
 *Write to a save game file.                                          *
 *   OverlayClass::Code_Pointers -- codes class's pointers for load/save *
 *   OverlayClass::Decode_Pointers -- decodes pointers for load/save *
 *   ReinforcementClass::Load -- Reads from a save game file. *
 *   ReinforcementClass::Save -- Write to a save game file. *
 *   ReinforcementClass::Code_Pointers -- codes class's pointers for load/save *
 *   ReinforcementClass::Decode_Pointers -- decodes pointers for load/save *
 *   SmudgeClass::Load -- Reads from a save game file. * SmudgeClass::Save --
 *Write to a save game file.                                           *
 *   SmudgeClass::Code_Pointers -- codes class's pointers for load/save *
 *   SmudgeClass::Decode_Pointers -- decodes pointers for load/save *
 *   TemplateClass::Load -- Reads from a save game file. * TemplateClass::Save
 *-- Write to a save game file.                                         *
 *   TemplateClass::Code_Pointers -- codes class's pointers for load/save *
 *   TemplateClass::Decode_Pointers -- decodes pointers for load/save *
 *   TerrainClass::Load -- Reads from a save game file. * TerrainClass::Save --
 *Write to a save game file.                                          *
 *   TerrainClass::Code_Pointers -- codes class's pointers for load/save *
 *   TerrainClass::Decode_Pointers -- decodes pointers for load/save *
 *   UnitClass::Load -- Reads from a save game file. * UnitClass::Save -- Write
 *to a save game file.                                             *
 *   UnitClass::Code_Pointers -- codes class's pointers for load/save *
 *   UnitClass::Decode_Pointers -- decodes pointers for load/save *
 *   FactoryClass::Serialize -- Read/write saved fields. *
 *   LayerClass::Load -- Reads from a save game file. * LayerClass::Save --
 *Write to a save game file.                                            *
 *   LayerClass::Code_Pointers -- codes class's pointers for load/save *
 *   LayerClass::Decode_Pointers -- decodes pointers for load/save *
 *   HouseClass::Serialize -- Read/write house state. *
 *   ScoreClass::Load -- Reads from a save game file. * ScoreClass::Save --
 *Write to a save game file.                                            *
 *   ScoreClass::Code_Pointers -- codes class's pointers for load/save *
 *   ScoreClass::Decode_Pointers -- decodes pointers for load/save *
 *   FlyClass::Code_Pointers -- codes class's pointers for load/save *
 *   FlyClass::Decode_Pointers -- decodes pointers for load/save *
 *   FuseClass::Code_Pointers -- codes class's pointers for load/save *
 *   FuseClass::Decode_Pointers -- decodes pointers for load/save *
 *   TarComClass::Code_Pointers -- codes class's pointers for load/save *
 *   TarComClass::Decode_Pointers -- decodes pointers for load/save *
 *   TurretClass::Code_Pointers -- codes class's pointers for load/save *
 *   TurretClass::Decode_Pointers -- decodes pointers for load/save *
 *   DriveClass::Code_Pointers -- codes class's pointers for load/save *
 *   DriveClass::Decode_Pointers -- decodes pointers for load/save *
 *   FootClass::Code_Pointers -- codes class's pointers for load/save *
 *   FootClass::Decode_Pointers -- decodes pointers for load/save *
 *   RadioClass::Code_Pointers -- codes class's pointers for load/save *
 *   RadioClass::Decode_Pointers -- decodes pointers for load/save *
 *   TechnoClass::Code_Pointers -- codes class's pointers for load/save *
 *   TechnoClass::Decode_Pointers -- decodes pointers for load/save *
 *   FlasherClass::Code_Pointers -- codes class's pointers for load/save *
 *   FlasherClass::Decode_Pointers -- decodes pointers for load/save *
 *   CargoClass::Code_Pointers -- codes class's pointers for load/save *
 *   CargoClass::Decode_Pointers -- decodes pointers for load/save *
 *   MissionClass::Code_Pointers -- codes class's pointers for load/save *
 *   MissionClass::Decode_Pointers -- decodes pointers for load/save *
 *   ObjectClass::Code_Pointers -- codes class's pointers for load/save *
 *   ObjectClass::Decode_Pointers -- decodes pointers for load/save *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include <cstdint>

#include "td/abstract.h"
#include "td/aircraft.h"
#include "td/anim.h"
#include "td/building.h"
#include "td/bullet.h"
#include "td/cargo.h"
#include "td/defines.h"
#include "td/door.h"
#include "td/drive.h"
#include "td/externs.h"
#include "td/factory.h"
#include "td/flasher.h"
#include "td/fly.h"
#include "td/foot.h"
#include "td/fuse.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/layer.h"
#include "td/mission.h"
#include "td/object.h"
#include "td/overlay.h"
#include "td/radio.h"
#include "td/saveload.h"
#include "td/score.h"
#include "td/serialize.h"
#include "td/smudge.h"
#include "td/stage.h"
#include "td/support.h"
#include "td/tarcom.h"
#include "td/target.h"
#include "td/team.h"
#include "td/teamtype.h"
#include "td/techno.h"
#include "td/template.h"
#include "td/terrain.h"
#include "td/trigger.h"
#include "td/turret.h"
#include "td/type.h"
#include "td/unit.h"
#include "td/vector.h"
#include "tech/archive.h"
#include "tech/wwfile.h"

/***********************************************************************************************
 * AircraftClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool AircraftClass::Load(ArchiveReader& file) {
  return Read_Object(this, sizeof(AbstractClass), sizeof(*this), file, VTable);
}

/***********************************************************************************************
 * AircraftClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool AircraftClass::Save(ArchiveWriter& file) {
  return Write_Object(this, sizeof(*this), file);
}

/***********************************************************************************************
 * AircraftClass::Code_Pointers -- codes class's pointers for load/save *
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
void AircraftClass::Code_Pointers() {
  /*
  ------------------------------ Code 'Class' ------------------------------
  */
  (AircraftTypeClass*&)Class = (AircraftTypeClass*)Class->Type;

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  FootClass::Code_Pointers();
  FlyClass::Code_Pointers();
}

/***********************************************************************************************
 * AircraftClass::Decode_Pointers -- decodes pointers for load/save *
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
void AircraftClass::Decode_Pointers() {
  /*
  ----------------------------- Decode 'Class' -----------------------------
  */
  Class = &AircraftTypeClass::As_Reference(
      static_cast<AircraftType>((uintptr_t)Class));
  Check_Ptr(Class);

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  FootClass::Decode_Pointers();
  FlyClass::Decode_Pointers();
}

/***********************************************************************************************
 * AnimClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool AnimClass::Load(ArchiveReader& file) {
  return Read_Object(this, sizeof(AbstractClass), sizeof(*this), file, VTable);
}

/***********************************************************************************************
 * AnimClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool AnimClass::Save(ArchiveWriter& file) {
  return Write_Object(this, sizeof(*this), file);
}

/***********************************************************************************************
 * AnimClass::Code_Pointers -- codes class's pointers for load/save *
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
void AnimClass::Code_Pointers() {
  /*
  ------------------------------ Code 'Class' ------------------------------
  */
  (AnimTypeClass*&)Class = (AnimTypeClass*)Class->Type;

  /*
  ----------------------------- Code 'Object' ------------------------------
  */
  if (Object) {
    Object = (ObjectClass*)Object->As_Target();
  }

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  ObjectClass::Code_Pointers();
  StageClass::Code_Pointers();
}

/***********************************************************************************************
 * AnimClass::Decode_Pointers -- decodes pointers for load/save *
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
void AnimClass::Decode_Pointers() {
  /*
  ----------------------------- Decode 'Class' -----------------------------
  */
  Class = &AnimTypeClass::As_Reference(static_cast<AnimType>((uintptr_t)Class));
  Check_Ptr(Class);

  /*
  ---------------------------- Decode 'Object' -----------------------------
  */
  if (Object) {
    Object = As_Object(static_cast<TARGET>((uintptr_t)Object));
    Check_Ptr(Object);
  }

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  ObjectClass::Decode_Pointers();
  StageClass::Decode_Pointers();
}

/***********************************************************************************************
 * BuildingClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool BuildingClass::Load(ArchiveReader& file) {
  return Read_Object(this, sizeof(AbstractClass), sizeof(*this), file, VTable);
}

/***********************************************************************************************
 * BuildingClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool BuildingClass::Save(ArchiveWriter& file) {
  return Write_Object(this, sizeof(*this), file);
}

/***********************************************************************************************
 * BuildingClass::Code_Pointers -- codes class's pointers for load/save *
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
void BuildingClass::Code_Pointers() {
  /*
  ------------------------------ Code 'Class' ------------------------------
  */
  Class = (BuildingTypeClass*)Class->Type;

  /*------------------------------------------------------------------------
  Code the Factory value; there's not target conversion routine for factories,
  so just use its Array ID, plus 1 so it doesn't look like a NULL value when
  it's converted back
  ------------------------------------------------------------------------*/
  if (Factory) {
    Factory = (FactoryClass*)(Factories.ID(Factory) + 1);
  }

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  TechnoClass::Code_Pointers();
}

/***********************************************************************************************
 * BuildingClass::Decode_Pointers -- decodes pointers for load/save *
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
void BuildingClass::Decode_Pointers() {
  /*
  ----------------------------- Decode 'Class' -----------------------------
  */
  Class = &BuildingTypeClass::As_Reference(
      static_cast<StructType>((uintptr_t)Class));
  Check_Ptr(Class);

  /*------------------------------------------------------------------------
  Decode the Factory value, subtracting off the '1' we added when coding it
  ------------------------------------------------------------------------*/
  if (Factory) {
    Factory = Factories.Raw_Ptr(static_cast<int>((intptr_t)Factory - 1));
    Check_Ptr(Factory);
  }

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  TechnoClass::Decode_Pointers();
}

/***********************************************************************************************
 * BulletClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool BulletClass::Load(ArchiveReader& file) {
  return Read_Object(this, sizeof(AbstractClass), sizeof(*this), file, VTable);
}

/***********************************************************************************************
 * BulletClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool BulletClass::Save(ArchiveWriter& file) {
  return Write_Object(this, sizeof(*this), file);
}

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
  ------------------------------ Code 'Class' ------------------------------
  */
  (BulletTypeClass*&)Class = (BulletTypeClass*)Class->Type;

  /*
  ----------------------------- Code 'Payback' -----------------------------
  */
  if (Payback) {
    Payback = (TechnoClass*)Payback->As_Target();
  }

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  ObjectClass::Code_Pointers();
  FlyClass::Code_Pointers();
  FuseClass::Code_Pointers();
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
  ----------------------------- Decode 'Class' -----------------------------
  */
  Class =
      &BulletTypeClass::As_Reference(static_cast<BulletType>((uintptr_t)Class));
  Check_Ptr(Class);

  /*
  ---------------------------- Decode 'Payback' ----------------------------
  */
  if (Payback) {
    Payback = As_Techno(static_cast<TARGET>((uintptr_t)Payback));
    Check_Ptr(Payback);
  }

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  ObjectClass::Decode_Pointers();
  FlyClass::Decode_Pointers();
  FuseClass::Decode_Pointers();
}

/***********************************************************************************************
 * InfantryClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool InfantryClass::Load(ArchiveReader& file) {
  return Read_Object(this, sizeof(AbstractClass), sizeof(*this), file, VTable);
}

/***********************************************************************************************
 * InfantryClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool InfantryClass::Save(ArchiveWriter& file) {
  return Write_Object(this, sizeof(*this), file);
}

/***********************************************************************************************
 * InfantryClass::Code_Pointers -- codes class's pointers for load/save *
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
void InfantryClass::Code_Pointers() {
  /*
  ------------------------------ Code 'Class' ------------------------------
  */
  (InfantryTypeClass*&)Class = (InfantryTypeClass*)Class->Type;

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  FootClass::Code_Pointers();
}

/***********************************************************************************************
 * InfantryClass::Decode_Pointers -- decodes pointers for load/save *
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
void InfantryClass::Decode_Pointers() {
  /*
  ----------------------------- Decode 'Class' -----------------------------
  */
  Class = &InfantryTypeClass::As_Reference(
      static_cast<InfantryType>((uintptr_t)Class));
  Check_Ptr(Class);

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  FootClass::Decode_Pointers();
}

/***********************************************************************************************
 * OverlayClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool OverlayClass::Load(ArchiveReader& file) {
  return Read_Object(this, sizeof(AbstractClass), sizeof(*this), file, VTable);
}

/***********************************************************************************************
 * OverlayClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool OverlayClass::Save(ArchiveWriter& file) {
  return Write_Object(this, sizeof(*this), file);
}

/***********************************************************************************************
 * OverlayClass::Code_Pointers -- codes class's pointers for load/save *
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
void OverlayClass::Code_Pointers() {
  /*
  ------------------------------ Code 'Class' ------------------------------
  */
  (OverlayTypeClass*&)Class = (OverlayTypeClass*)Class->Type;

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  ObjectClass::Code_Pointers();
}

/***********************************************************************************************
 * OverlayClass::Decode_Pointers -- decodes pointers for load/save *
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
void OverlayClass::Decode_Pointers() {
  /*
  ----------------------------- Decode 'Class' -----------------------------
  */
  Class = &OverlayTypeClass::As_Reference(
      static_cast<OverlayType>((uintptr_t)Class));
  Check_Ptr(Class);

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  ObjectClass::Decode_Pointers();
}

/***********************************************************************************************
 * SmudgeClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool SmudgeClass::Load(ArchiveReader& file) {
  return Read_Object(this, sizeof(AbstractClass), sizeof(*this), file, VTable);
}

/***********************************************************************************************
 * SmudgeClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool SmudgeClass::Save(ArchiveWriter& file) {
  return Write_Object(this, sizeof(*this), file);
}

/***********************************************************************************************
 * SmudgeClass::Code_Pointers -- codes class's pointers for load/save *
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
void SmudgeClass::Code_Pointers() {
  /*
  ------------------------------ Code 'Class' ------------------------------
  */
  Class = (SmudgeTypeClass*)Class->Type;

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  ObjectClass::Code_Pointers();
}

/***********************************************************************************************
 * SmudgeClass::Decode_Pointers -- decodes pointers for load/save *
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
void SmudgeClass::Decode_Pointers() {
  /*
  ----------------------------- Decode 'Class' -----------------------------
  */
  Class =
      &SmudgeTypeClass::As_Reference(static_cast<SmudgeType>((uintptr_t)Class));
  Check_Ptr(Class);

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  ObjectClass::Decode_Pointers();
}

/***********************************************************************************************
 * TemplateClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool TemplateClass::Load(ArchiveReader& file) {
  return Read_Object(this, sizeof(AbstractClass), sizeof(*this), file, VTable);
}

/***********************************************************************************************
 * TemplateClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool TemplateClass::Save(ArchiveWriter& file) {
  return Write_Object(this, sizeof(*this), file);
}

/***********************************************************************************************
 * TemplateClass::Code_Pointers -- codes class's pointers for load/save *
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
void TemplateClass::Code_Pointers() {
  /*
  ------------------------------ Code 'Class' ------------------------------
  */
  (TemplateTypeClass*&)Class = (TemplateTypeClass*)Class->Type;

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  ObjectClass::Code_Pointers();
}

/***********************************************************************************************
 * TemplateClass::Decode_Pointers -- decodes pointers for load/save *
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
void TemplateClass::Decode_Pointers() {
  /*
  ----------------------------- Decode 'Class' -----------------------------
  */
  Class = &TemplateTypeClass::As_Reference(
      static_cast<TemplateType>((uintptr_t)Class));
  Check_Ptr(Class);

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  ObjectClass::Decode_Pointers();
}

/***********************************************************************************************
 * TerrainClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool TerrainClass::Load(ArchiveReader& file) {
  return Read_Object(this, sizeof(AbstractClass), sizeof(*this), file, VTable);
}

/***********************************************************************************************
 * TerrainClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool TerrainClass::Save(ArchiveWriter& file) {
  return Write_Object(this, sizeof(*this), file);
}

/***********************************************************************************************
 * TerrainClass::Code_Pointers -- codes class's pointers for load/save *
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
void TerrainClass::Code_Pointers() {
  /*
  ------------------------------ Code 'Class' ------------------------------
  */
  (TerrainTypeClass*&)Class = (TerrainTypeClass*)Class->Type;

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  ObjectClass::Code_Pointers();
  StageClass::Code_Pointers();
}

/***********************************************************************************************
 * TerrainClass::Decode_Pointers -- decodes pointers for load/save *
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
void TerrainClass::Decode_Pointers() {
  /*
  ----------------------------- Decode 'Class' -----------------------------
  */
  Class = &TerrainTypeClass::As_Reference(
      static_cast<TerrainType>((uintptr_t)Class));
  Check_Ptr(Class);

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  ObjectClass::Decode_Pointers();
  StageClass::Decode_Pointers();
}

/***********************************************************************************************
 * UnitClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool UnitClass::Load(ArchiveReader& file) {
  return Read_Object(this, sizeof(AbstractClass), sizeof(*this), file, VTable);
}

/***********************************************************************************************
 * UnitClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool UnitClass::Save(ArchiveWriter& file) {
  return Write_Object(this, sizeof(*this), file);
}

/***********************************************************************************************
 * UnitClass::Code_Pointers -- codes class's pointers for load/save *
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
void UnitClass::Code_Pointers() { TarComClass::Code_Pointers(); }

/***********************************************************************************************
 * UnitClass::Decode_Pointers -- decodes pointers for load/save *
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
void UnitClass::Decode_Pointers() { TarComClass::Decode_Pointers(); }

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
bool LayerClass::Load(ArchiveReader& file) {
  int count;
  int i;
  ObjectClass* ptr;

  /*
  ---------------------- Read # elements in the layer ----------------------
  */
  file.Bytes(&count, sizeof(count));
  if (!file.ok()) {
    return false;
  }

  /*
  ---------------------------- Clear the array -----------------------------
  */
  Clear();

  /*
  ----------------------- Read in all array elements -----------------------
  */
  for (i = 0; i < count; i++) {
    file.Bytes(static_cast<void*>(&ptr), sizeof(ObjectClass*));
    if (!file.ok()) {
      return false;
    }
    Add(ptr);
  }

  return file.ok();
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
bool LayerClass::Save(ArchiveWriter& file) {
  int count;
  int i;
  ObjectClass* ptr;

  /*
  ------------------------- Save # array elements --------------------------
  */
  count = static_cast<int>(Count());
  file.Bytes(&count, sizeof(count));

  /*
  --------------------------- Save all elements ----------------------------
  */
  for (i = 0; i < count; i++) {
    ptr = (*this)[i];
    file.Bytes(static_cast<const void*>(&ptr), sizeof(ObjectClass*));
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
  ObjectClass* obj;

  for (int i = 0; i < Count(); i++) {
    obj = (*this)[i];
    (*this)[i] = (ObjectClass*)obj->As_Target();
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
  TARGET target;

  for (int i = 0; i < Count(); i++) {
    target = static_cast<TARGET>((uintptr_t)(*this)[i]);
    (*this)[i] = As_Object(target);
    Check_Ptr((*this)[i]);
  }
}

/***********************************************************************************************
 * ScoreClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool ScoreClass::Load(ArchiveReader& file) {
  return Read_Object(this, sizeof(*this), sizeof(*this), file, nullptr);
}

/***********************************************************************************************
 * ScoreClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool ScoreClass::Save(ArchiveWriter& file) {
  return Write_Object(this, sizeof(*this), file);
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
void ScoreClass::Code_Pointers() {}

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
void ScoreClass::Decode_Pointers() {}

/***********************************************************************************************
 * FlyClass::Code_Pointers -- codes class's pointers for load/save *
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
void FlyClass::Code_Pointers() {}

/***********************************************************************************************
 * FlyClass::Decode_Pointers -- decodes pointers for load/save *
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
void FlyClass::Decode_Pointers() {}

/***********************************************************************************************
 * FuseClass::Code_Pointers -- codes class's pointers for load/save *
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
void FuseClass::Code_Pointers() {}

/***********************************************************************************************
 * FuseClass::Decode_Pointers -- decodes pointers for load/save *
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
void FuseClass::Decode_Pointers() {}

/***********************************************************************************************
 * TarComClass::Code_Pointers -- codes class's pointers for load/save *
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
void TarComClass::Code_Pointers() { TurretClass::Code_Pointers(); }

/***********************************************************************************************
 * TarComClass::Decode_Pointers -- decodes pointers for load/save *
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
void TarComClass::Decode_Pointers() { TurretClass::Decode_Pointers(); }

/***********************************************************************************************
 * TurretClass::Code_Pointers -- codes class's pointers for load/save *
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
void TurretClass::Code_Pointers() { DriveClass::Code_Pointers(); }

/***********************************************************************************************
 * TurretClass::Decode_Pointers -- decodes pointers for load/save *
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
void TurretClass::Decode_Pointers() { DriveClass::Decode_Pointers(); }

/***********************************************************************************************
 * DriveClass::Code_Pointers -- codes class's pointers for load/save *
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
void DriveClass::Code_Pointers() {
  /*
  ------------------------------ Code 'Class' ------------------------------
  */
  (UnitTypeClass*&)Class = (UnitTypeClass*)Class->Type;

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  FootClass::Code_Pointers();
}

/***********************************************************************************************
 * DriveClass::Decode_Pointers -- decodes pointers for load/save *
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
void DriveClass::Decode_Pointers() {
  /*
  ----------------------------- Decode 'Class' -----------------------------
  */
  Class = &UnitTypeClass::As_Reference(static_cast<UnitType>((uintptr_t)Class));
  Check_Ptr(Class);

  /*
  ---------------------------- Chain to parent -----------------------------
  */
  FootClass::Decode_Pointers();
}

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
  if (Team) {
    Team = (TeamClass*)Team->As_Target();
  }

  if (Member) {
    Member = (FootClass*)Member->As_Target();
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
  if (Team) {
    Team = As_Team(static_cast<TARGET>((uintptr_t)Team));
    Check_Ptr(Team);
  }

  if (Member) {
    Member = dynamic_cast<FootClass*>(As_Techno((TARGET)(uintptr_t)Member));
    Check_Ptr(Member);
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
  ------------------------------ Code 'Radio' ------------------------------
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
  ----------------------------- Decode 'Radio' -----------------------------
  */
  if (Radio) {
    Radio = As_Techno(static_cast<TARGET>((uintptr_t)Radio));
    Check_Ptr((void*)Radio);
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
  /*
  ------------------------------ Code 'House' ------------------------------
  */
  House = (HouseClass*)House->Class->House;

  FlasherClass::Code_Pointers();
  StageClass::Code_Pointers();
  CargoClass::Code_Pointers();
  DoorClass::Code_Pointers();

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
  /*
  ----------------------------- Decode 'House' -----------------------------
  */
  House = HouseClass::As_Pointer(static_cast<HousesType>((uintptr_t)House));
  Check_Ptr((void*)House);

  FlasherClass::Decode_Pointers();
  StageClass::Decode_Pointers();
  CargoClass::Decode_Pointers();
  DoorClass::Decode_Pointers();

  RadioClass::Decode_Pointers();
}

/***********************************************************************************************
 * FlasherClass::Code_Pointers -- codes class's pointers for load/save *
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
void FlasherClass::Code_Pointers() {}

/***********************************************************************************************
 * FlasherClass::Decode_Pointers -- decodes pointers for load/save *
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
void FlasherClass::Decode_Pointers() {}

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
  ---------------------------- Code 'CargoHold' ----------------------------
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
  --------------------------- Decode 'CargoHold' ---------------------------
  */
  if (CargoHold) {
    CargoHold =
        dynamic_cast<FootClass*>(As_Techno((TARGET)(uintptr_t)CargoHold));
    Check_Ptr((void*)CargoHold);
  }
}

/***********************************************************************************************
 * MissionClass::Code_Pointers -- codes class's pointers for load/save *
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
void MissionClass::Code_Pointers() { ObjectClass::Code_Pointers(); }

/***********************************************************************************************
 * MissionClass::Decode_Pointers -- decodes pointers for load/save *
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
void MissionClass::Decode_Pointers() { ObjectClass::Decode_Pointers(); }

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

  if (Trigger) {
    Trigger = (TriggerClass*)Trigger->As_Target();
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
    Next = As_Object(static_cast<TARGET>((uintptr_t)Next));
    Check_Ptr((void*)Next);
  }

  if (Trigger) {
    Trigger = As_Trigger(static_cast<TARGET>((uintptr_t)Trigger));
    Check_Ptr((void*)Trigger);
  }
}

// These objects stay live while the remaining raw heaps code their pointers.
// House IDs and heap TARGETs avoid looking through any coded type pointers.
template <class Archive>
void FactoryClass::Serialize(Archive& ar) {
  StageClass::Serialize(ar);
  bool active = IsActive;
  bool suspended = IsSuspended;
  bool different = IsDifferent;
  ar(active, suspended, different, Balance, OriginalBalance, ObjectPtr(Object),
     SpecialItem, House);
  if constexpr (Archive::kIsReading) {
    IsActive = active;
    IsSuspended = suspended;
    IsDifferent = different;
    if (!active || House < HOUSE_NONE || House >= HOUSE_COUNT ||
        ((Object != nullptr || SpecialItem != SPC_NONE) &&
         House == HOUSE_NONE)) {
      ar.Fail("invalid factory state");
    }
  }
}
template void FactoryClass::Serialize(ArchiveWriter&);
template void FactoryClass::Serialize(ArchiveReader&);

template <class Archive>
void TriggerClass::Serialize(Archive& ar) {
  bool active = IsActive;
  ar(TeamTypePtr(Team), active, IsPersistant, AttachCount, Event, Action, House,
     Data, DataCopy, Name);
  if constexpr (Archive::kIsReading) {
    IsActive = active;
    if (!active || IsPersistant < VOLATILE || IsPersistant > PERSISTANT ||
        AttachCount < 0 || Event < EVENT_NONE || Event >= EVENT_COUNT ||
        Action < ACTION_NONE || Action >= ACTION_COUNT || House < HOUSE_NONE ||
        House >= HOUSE_COUNT || Name[4] != '\0') {
      ar.Fail("invalid trigger state");
    }
  }
}
template void TriggerClass::Serialize(ArchiveWriter&);
template void TriggerClass::Serialize(ArchiveReader&);

template <class Archive>
void TeamTypeClass::Serialize(Archive& ar) {
  AbstractTypeClass::Serialize(ar);
  bool active = IsActive, roundabout = IsRoundAbout, learning = IsLearning;
  bool suicide = IsSuicide, autocreate = IsAutocreate, mercenary = IsMercenary;
  bool prebuilt = IsPrebuilt, reinforcable = IsReinforcable,
       transient = IsTransient;
  ar(active, roundabout, learning, suicide, autocreate, mercenary, prebuilt,
     reinforcable, transient, RecruitPriority, InitNum, MaxAllowed, Fear, House,
     MissionCount, ClassCount);
  if constexpr (Archive::kIsReading) {
    IsActive = active;
    IsRoundAbout = roundabout;
    IsLearning = learning;
    IsSuicide = suicide;
    IsAutocreate = autocreate;
    IsMercenary = mercenary;
    IsPrebuilt = prebuilt;
    IsReinforcable = reinforcable;
    IsTransient = transient;
    if (!active || MissionCount < 0 || MissionCount > MAX_TEAM_MISSIONS ||
        ClassCount > MAX_TEAM_CLASSCOUNT || House < HOUSE_NONE ||
        House >= HOUSE_COUNT) {
      ar.Fail("invalid team type state");
      return;
    }
  }
  // Unused array tails have no gameplay meaning and are defaulted on load.
  for (int i = 0; i < MissionCount; ++i) {
    ar(MissionList[i]);
  }
  for (int i = 0; i < ClassCount; ++i) {
    ar(TechnoTypePtr(Class[i]), DesiredNum[i]);
    if constexpr (Archive::kIsReading) {
      if (Class[i] == nullptr) {
        ar.Fail("missing team member type");
      }
    }
  }
}
template void TeamTypeClass::Serialize(ArchiveWriter&);
template void TeamTypeClass::Serialize(ArchiveReader&);

template <class Archive>
void TeamClass::Serialize(Archive& ar) {
  AbstractClass::Serialize(ar);
  bool forced = IsForcedActive, has_been = IsHasBeen, full = IsFullStrength;
  bool under = IsUnderStrength, reforming = IsReforming, lagging = IsLagging;
  bool altered = IsAltered, moving = IsMoving, next = IsNextMission,
       suspended = Suspended;
  ar(TeamTypePtr(Class), HousePtr(House), forced, has_been, full, under,
     reforming, lagging, altered, moving, next, suspended, Center,
     ObjectiveCenter, MissionTarget, Target, Total, Risk, SuspendTimer,
     CurrentMission, TimeOut, ObjectPtr(Member), Quantity);
  if constexpr (Archive::kIsReading) {
    IsForcedActive = forced;
    IsHasBeen = has_been;
    IsFullStrength = full;
    IsUnderStrength = under;
    IsReforming = reforming;
    IsLagging = lagging;
    IsAltered = altered;
    IsMoving = moving;
    IsNextMission = next;
    Suspended = suspended;
    // TeamTypes and Houses precede Teams in the stream. Members load later.
    bool known_type = false;
    for (int i = 0; i < TeamTypes.Count(); ++i) {
      if (TeamTypes.Ptr(i) == Class) {
        known_type = true;
        break;
      }
    }
    if (!known_type || Houses.ActivePointers.ID(House) < 0 || Total < 0 ||
        CurrentMission < -1 || CurrentMission >= Class->MissionCount) {
      ar.Fail("invalid team state");
    }
  }
}
template void TeamClass::Serialize(ArchiveWriter&);
template void TeamClass::Serialize(ArchiveReader&);

template <class Archive>
void HouseClass::Serialize(Archive& ar) {
  // Runtime remap pointers are encoded by table identity, including RemapNone.
  const unsigned char* tables[] = {RemapNone,      RemapYellow, RemapRed,
                                   RemapBlueGreen, RemapOrange, RemapGreen,
                                   RemapBlue};
  int32_t remap_id = -1;
  if constexpr (!Archive::kIsReading) {
    for (int i = 0; i < 7; ++i) {
      if (RemapTable == tables[i]) {
        remap_id = i;
      }
    }
  }
  bool saved_IsActive = IsActive;
  bool saved_IsHuman = IsHuman;
  bool saved_IsStarted = IsStarted;
  bool saved_IsAlerted = IsAlerted;
  bool saved_IsDiscovered = IsDiscovered;
  bool saved_IsMaxedOut = IsMaxedOut;
  bool saved_IsDefeated = IsDefeated;
  bool saved_IsToDie = IsToDie;
  bool saved_IsToWin = IsToWin;
  bool saved_IsToLose = IsToLose;
  bool saved_IsCivEvacuated = IsCivEvacuated;
  bool saved_IsRecalcNeeded = IsRecalcNeeded;
  bool saved_IsVisionary = IsVisionary;
  bool saved_IsAirstrikePending = IsAirstrikePending;
  uint8_t saved_NukePieces = NukePieces;
  bool saved_IsFreeHarvester = IsFreeHarvester;
  bool saved_Resigned = Resigned;
  bool saved_IGaveUp = IGaveUp;
  ar(TypePtr(Class), ActLike, saved_IsActive, saved_IsHuman, saved_IsStarted,
     saved_IsAlerted, saved_IsDiscovered, saved_IsMaxedOut, saved_IsDefeated,
     saved_IsToDie, saved_IsToWin, saved_IsToLose, saved_IsCivEvacuated,
     saved_IsRecalcNeeded, saved_IsVisionary, saved_IsAirstrikePending,
     saved_NukePieces, saved_IsFreeHarvester, FreeHarvester, IonCannon,
     AirStrike, NukeStrike, JustBuilt, Blockage, AlertTime, BorrowedTime, BScan,
     ActiveBScan, NewBScan, NewActiveBScan, UScan, ActiveUScan, NewUScan,
     NewActiveUScan, IScan, ActiveIScan, NewIScan, NewActiveIScan, AScan,
     ActiveAScan, NewAScan, NewActiveAScan, CreditsSpent, HarvestedCredits,
     CurUnits, CurBuildings, MaxUnit, MaxBuilding, Tiberium, Credits,
     InitialCredits, Capacity, saved_Resigned, saved_IGaveUp, AircraftFactories,
     InfantryFactories, UnitFactories, BuildingFactories, SpecialFactories,
     Power, Drain, Edge, AircraftFactory, InfantryFactory, UnitFactory,
     BuildingFactory, SpecialFactory, FlagLocation, FlagHome, remap_id,
     RemapColor, Name, UnitsKilled, UnitsLost, BuildingsKilled, BuildingsLost,
     WhoLastHurtMe, Regions, BlitzTime, NukeDest, Allies, DamageTime, TeamTime,
     TriggerTime, SpeakAttackDelay, SpeakPowerDelay, SpeakMoneyDelay,
     SpeakMaxedDelay);
  if constexpr (Archive::kIsReading) {
    IsActive = saved_IsActive;
    IsHuman = saved_IsHuman;
    IsStarted = saved_IsStarted;
    IsAlerted = saved_IsAlerted;
    IsDiscovered = saved_IsDiscovered;
    IsMaxedOut = saved_IsMaxedOut;
    IsDefeated = saved_IsDefeated;
    IsToDie = saved_IsToDie;
    IsToWin = saved_IsToWin;
    IsToLose = saved_IsToLose;
    IsCivEvacuated = saved_IsCivEvacuated;
    IsRecalcNeeded = saved_IsRecalcNeeded;
    IsVisionary = saved_IsVisionary;
    IsAirstrikePending = saved_IsAirstrikePending;
    NukePieces = saved_NukePieces;
    IsFreeHarvester = saved_IsFreeHarvester;
    Resigned = saved_Resigned;
    IGaveUp = saved_IGaveUp;
    if (!ar.ok()) {
      return;
    }
    if (!IsActive || Class == nullptr || ActLike < HOUSE_FIRST ||
        ActLike >= HOUSE_COUNT || saved_NukePieces > 7 ||
        JustBuilt < STRUCT_NONE || JustBuilt >= STRUCT_COUNT ||
        Edge < SOURCE_FIRST || Edge >= SOURCE_COUNT || remap_id < 0 ||
        remap_id >= 7 || RemapColor < REMAP_NONE || RemapColor >= REMAP_COUNT ||
        WhoLastHurtMe < HOUSE_NONE || WhoLastHurtMe >= HOUSE_COUNT ||
        Name[sizeof(Name) - 1] != '\0') {
      ar.Fail("invalid house state");
      return;
    }
    for (int factory : {AircraftFactory, InfantryFactory, UnitFactory,
                        BuildingFactory, SpecialFactory}) {
      if (factory < -1 || factory >= Factories.Length()) {
        ar.Fail("invalid house factory slot");
        return;
      }
    }
    RemapTable = tables[remap_id];
    // The ten runtime-only UnitTracker counters are recreated by the shell
    // ctor.
  }
}
template void HouseClass::Serialize(ArchiveWriter&);
template void HouseClass::Serialize(ArchiveReader&);
