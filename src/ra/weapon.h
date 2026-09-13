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

/* $Header: /CounterStrike/WEAPON.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : WEAPON.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/17/96 *
 *                                                                                             *
 *                  Last Update : May 17, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_WEAPON_H_
#define CNC_RED_ALERT_RA_WEAPON_H_

#include <cstddef>

#include "absl/base/attributes.h"
#include "ra/ccini.h"
#include "ra/defines.h"
#include "ra/type.h"
#include "ra/warhead.h"

/**********************************************************************
**	This is the constant data associated with a weapon. Some objects
**	can have multiple weapons and this class is used to isolate and
**	specify this data in a convenient and selfcontained way.
*/
class WeaponTypeClass {
 public:
  explicit WeaponTypeClass(const char* name ABSL_ATTRIBUTE_LIFETIME_BOUND);
  ~WeaponTypeClass();
  WeaponTypeClass(const WeaponTypeClass&) = delete;
  WeaponTypeClass& operator=(const WeaponTypeClass&) = delete;
  WeaponTypeClass(WeaponTypeClass&&) = delete;
  WeaponTypeClass& operator=(WeaponTypeClass&&) = delete;

  void* operator new(size_t /*unused*/) noexcept;
  void* operator new(size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* pointer);

  [[nodiscard]] const char* Name() const { return IniName; }
  bool Read_INI(CCINIClass& ini);
  static WeaponTypeClass* As_Pointer(WeaponType weapon);
  [[nodiscard]] ThreatType Allowed_Threats() const;
  [[nodiscard]] bool Is_Wall_Destroyer() const;

  /*
  **	This is both the weapon type number and the index number into
  **	the weapon array.
  */
  int ID;

  /*
  **	This is the identifying name of this weapon.
  */
  const char* IniName;

  /*
  **	Increase the weapon speed if the target is flying.
  */
  bool IsTurboBoosted : 1 = false;

  /*
  **	If potential targets of this weapon should be scanned for
  **	nearby friendly structures and if found, firing upon the target
  **	would be discouraged, then this flag will be true.
  */
  bool IsSupressed : 1 {false};

  /*
  **	If this weapon is equipped with a camera that reveals the
  **	area around the firer, then this flag will be true.
  */
  bool IsCamera : 1 {false};

  /*
  **	If this weapon requires charging before it can fire, then this
  **	flag is true. In actuality, this only applies to the Tesla coil
  **	which has specific charging animation. The normal rate of fire
  **	value suffices for all other cases.
  */
  bool IsElectric : 1 {false};

  /*
  **	This is the number of shots this weapon first (in rapid succession).
  **	The normal value is 1, but for the case of two shooter weapons such as
  **	the double barreled gun turrets of the Mammoth tank, this value will be
  **	set to 2.
  */
  int Burst{1};

  /*
  **	This is the unit class of the projectile fired. A subset of the unit
  *types *	represent projectiles. It is one of these classes that is
  *specified here. *	If this object does not fire anything, then this value
  *will be BULLET_NONE.
  */
  const BulletTypeClass* Bullet{nullptr};

  /*
  **	This is the damage (explosive load) to be assigned to the projectile
  *that *	this object fires. For the rare healing weapon, this value is
  *negative.
  */
  int Attack{0};

  /*
  **	Speed of the projectile launched.
  */
  MPHType MaxSpeed{MPH_IMMOBILE};

  /*
  **	Warhead to attach to the projectile.
  */
  const WarheadTypeClass* WarheadPtr{nullptr};

  /*
  **	Objects that fire (which can be buildings as well) will fire at a
  **	frequency controlled by this value. This value serves as a count
  **	down timer between shots. The smaller the value, the faster the
  **	rate of fire.
  */
  int ROF{0};

  /*
  **	When this object fires, the range at which it's projectiles travel is
  **	controlled by this value. The value represents the number of cells the
  **	projectile will travel. Objects outside of this range will not be fired
  **	upon (in normal circumstances).
  */
  LEPTON Range{0};

  /*
  **	This is the typical sound generated when firing.
  */
  VocType Sound{VOC_NONE};

  /*
  **	This is the animation to display at the firing coordinate.
  */
  AnimType Anim{ANIM_NONE};
};

WeaponType Weapon_From_Name(const char* name);
ArmorType Armor_From_Name(const char* name);

#endif  // CNC_RED_ALERT_RA_WEAPON_H_
