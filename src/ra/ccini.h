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

/* $Header: /CounterStrike/CCINI.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CCINI.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/24/96 *
 *                                                                                             *
 *                  Last Update : May 24, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CCINI_H
#define CCINI_H

#include <cstdint>

#include "ra/defines.h"
#include "ra/ini.h"
#include "tech/pipe.h"
#include "tech/straw.h"
#include "tech/wwfile.h"

class TriggerTypeClass;

/*
**	The advanced version of the INI database manager. It handles the C&C
*expansion types and *	identifiers. In addition, it automatically stores a
*message digest with the INI data *	so that verification can occur.
*/
class CCINIClass : public INIClass {
 public:
  CCINIClass() : IsDigestPresent(false) {}

  bool Load(FileClass& file, bool withdigest);
  bool Load(Straw& file, bool withdigest);
  int Save(FileClass& file, bool withdigest) const;
  int Save(Pipe& pipe, bool withdigest) const;

  long Get_Buildings(const char* section, const char* entry,
                     long defvalue) const;
  UnitType Get_UnitType(const char* section, const char* entry,
                        UnitType defvalue) const;
  AnimType Get_AnimType(const char* section, const char* entry,
                        AnimType defvalue) const;
  ArmorType Get_ArmorType(const char* section, const char* entry,
                          ArmorType defvalue) const;
  BulletType Get_BulletType(const char* section, const char* entry,
                            BulletType defvalue) const;
  HousesType Get_HousesType(const char* section, const char* entry,
                            HousesType defvalue) const;
  LEPTON Get_Lepton(const char* section, const char* entry,
                    LEPTON defvalue) const;
  MPHType Get_MPHType(const char* section, const char* entry,
                      MPHType defvalue) const;
  OverlayType Get_OverlayType(const char* section, const char* entry,
                              OverlayType defvalue) const;
  SourceType Get_SourceType(const char* section, const char* entry,
                            SourceType defvalue) const;
  TerrainType Get_TerrainType(const char* section, const char* entry,
                              TerrainType defvalue) const;
  TheaterType Get_TheaterType(const char* section, const char* entry,
                              TheaterType defvalue) const;
  ThemeType Get_ThemeType(const char* section, const char* entry,
                          ThemeType defvalue) const;
  TriggerTypeClass* Get_TriggerType(const char* section,
                                    const char* entry) const;
  VQType Get_VQType(const char* section, const char* entry,
                    VQType defvalue) const;
  VocType Get_VocType(const char* section, const char* entry,
                      VocType defvalue) const;
  WarheadType Get_WarheadType(const char* section, const char* entry,
                              WarheadType defvalue) const;
  WeaponType Get_WeaponType(const char* section, const char* entry,
                            WeaponType defvalue) const;
  long Get_Owners(const char* section, const char* entry, long defvalue) const;
  CrateType Get_CrateType(const char* section, const char* entry,
                          CrateType defvalue) const;

  bool Put_Buildings(const char* section, const char* entry,
                     std::int32_t value);
  bool Put_AnimType(const char* section, const char* entry, AnimType value);
  bool Put_UnitType(const char* section, const char* entry, UnitType value);
  bool Put_ArmorType(const char* section, const char* entry, ArmorType value);
  bool Put_BulletType(const char* section, const char* entry, BulletType value);
  bool Put_HousesType(const char* section, const char* entry, HousesType value);
  bool Put_Lepton(const char* section, const char* entry, LEPTON value);
  bool Put_MPHType(const char* section, const char* entry, MPHType value);
  bool Put_VQType(const char* section, const char* entry, VQType value);
  bool Put_OverlayType(const char* section, const char* entry,
                       OverlayType value);
  bool Put_Owners(const char* section, const char* entry, long value);
  bool Put_SourceType(const char* section, const char* entry, SourceType value);
  bool Put_TerrainType(const char* section, const char* entry,
                       TerrainType value);
  bool Put_TheaterType(const char* section, const char* entry,
                       TheaterType value);
  bool Put_ThemeType(const char* section, const char* entry, ThemeType value);
  bool Put_TriggerType(const char* section, const char* entry,
                       TriggerTypeClass* value);
  bool Put_VocType(const char* section, const char* entry, VocType value);
  bool Put_WarheadType(const char* section, const char* entry,
                       WarheadType value);
  bool Put_WeaponType(const char* section, const char* entry, WeaponType value);
  bool Put_CrateType(const char* section, const char* entry, CrateType value);

  int Get_Unique_ID() const;

 private:
  void Calculate_Message_Digest();
  void Invalidate_Message_Digest();

  bool IsDigestPresent : 1;

  /*
  **	This is the message digest (SHA) of the INI database that was embedded
  *as part of *	the INI file.
  */
  unsigned char Digest[20];
};

#endif
