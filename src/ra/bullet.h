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

/* $Header: /CounterStrike/BULLET.H 2     3/06/97 1:46p Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : BULLET.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 23, 1994 *
 *                                                                                             *
 *                  Last Update : April 23, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef BULLET_H
#define BULLET_H

#include <cstddef>

#include "ra/ccptr.h"
#include "ra/defines.h"
#include "ra/face.h"
#include "ra/facing.h"
#include "ra/fly.h"
#include "ra/fuse.h"
#include "ra/object.h"
#include "ra/type.h"
#include "tech/noinit.h"
#include "tech/pipe.h"
#include "tech/straw.h"

class BulletClass : public ObjectClass, public FlyClass, public FuseClass {
 public:
  /*
  **	This specifies exactly what kind of bullet this is. All of the static
  *attributes *	for this bullet is located in the BulletTypeClass pointed to by
  *this variable.
  */
  CCPtr<BulletTypeClass> Class;

 private:
  /*
  **	Records who sent this "present" so that an appropriate "thank you" can
  **	be returned.
  */
  TechnoClass* Payback;

  /*
  **	This is the facing that the projectile is traveling.
  */
  FacingClass PrimaryFacing;

 public:
  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t, void* ptr) noexcept { return ptr; }
  void operator delete(void* ptr);
  BulletClass(BulletType id, TARGET target, TechnoClass* Payback, int strength,
              WarheadType warhead, int speed);
  BulletClass(const NoInitClass& x)
      : ObjectClass(x), Class(x), FlyClass(x), FuseClass(x), PrimaryFacing(x) {}
  ~BulletClass() override;
  operator BulletType() const { return Class->Type; }

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  static void Init();

  bool Is_Forced_To_Explode(COORDINATE& coord) const;
  void Bullet_Explodes(bool forced);
  int Shape_Number() const;
  LayerType In_Which_Layer() const override;
  COORDINATE Sort_Y() const override;
  virtual void Assign_Target(TARGET target) { TarCom = target; }
  bool Unlimbo(COORDINATE, DirType facing = DIR_N) override;
  const ObjectTypeClass& Class_Of() const override { return *Class; }
  void Detach(TARGET target, bool all) override;
  void Draw_It(int x, int y, WindowNumberType window) const override;
  bool Mark(MarkType mark = MARK_CHANGE) override;
  void AI() override;
  const short* Occupy_List(bool = false) const override;
  virtual const short* Overlap_List() const { return Occupy_List(false); }
  COORDINATE Target_Coord() const override;

  /*
  **	File I/O.
  */
  bool Load(Straw& file);
  bool Save(Pipe& file) const;
  void Code_Pointers() override;
  void Decode_Pointers() override;

  /*
  **	If this bullet is forced to be inaccurate because of some outside means.
  *A tank *	firing while moving is a good example.
  */
  unsigned IsInaccurate : 1;

 private:
  // Crude animation flag.
  unsigned IsToAnimate : 1;

  /*
  ** Is this missile allowed to come in from out of bounds?
  */
  unsigned IsLocked : 1;

  /*
  **	This is the target of the projectile. It is especially significant for
  *those projectiles *	that home in on a target.
  */
  TARGET TarCom;

  /*
  **	The speed of this projectile.
  */
  int MaxSpeed;

  /*
  **	The warhead of this projectile.
  */
  WarheadType Warhead;
};

#endif
