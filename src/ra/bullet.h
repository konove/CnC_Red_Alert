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

#ifndef CNC_RED_ALERT_RA_BULLET_H_
#define CNC_RED_ALERT_RA_BULLET_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "ra/ccptr.h"
#include "ra/defines.h"
#include "ra/face.h"
#include "ra/facing.h"
#include "ra/fly.h"
#include "ra/fuse.h"
#include "ra/object.h"
#include "ra/type.h"

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
  TechnoClass* Payback = nullptr;

  /*
  **	This is the facing that the projectile is traveling.
  */
  FacingClass PrimaryFacing;

 public:
  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);
  BulletClass(BulletType id, TARGET target,
              TechnoClass* Payback ABSL_ATTRIBUTE_LIFETIME_BOUND, int strength,
              WarheadType warhead, int speed);
  ~BulletClass() override;
  BulletClass(const BulletClass&) = delete;
  BulletClass& operator=(const BulletClass&) = delete;
  BulletClass(BulletClass&&) = delete;
  BulletClass& operator=(BulletClass&&) = delete;
  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator BulletType() const { return Class->Type; }

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  static void Init();

  bool Is_Forced_To_Explode(COORDINATE& coord) const;
  void Bullet_Explodes(bool forced);
  [[nodiscard]] int Shape_Number() const;
  [[nodiscard]] LayerType In_Which_Layer() const override;
  [[nodiscard]] COORDINATE Sort_Y() const override;
  virtual void Assign_Target(TARGET target) { TarCom = target; }
  bool Unlimbo(COORDINATE /*coord*/ /*unused*/, DirType dir = DIR_N) override;
  [[nodiscard]] const ObjectTypeClass& Class_Of() const override {
    return *Class;
  }
  void Detach(TARGET target, bool all) override;
  void Draw_It(int x, int y, WindowNumberType window) const override;
  bool Mark(MarkType mark = MARK_CHANGE) override;
  void AI() override;
  [[nodiscard]] std::span<const int16_t> Occupy_List(
      bool /*placement*/ = false) const override;
  [[nodiscard]] std::span<const int16_t> Overlap_List(
      bool /*redraw*/ = false) const override {
    return Occupy_List(false);
  }
  [[nodiscard]] COORDINATE Target_Coord() const override;

  /*
  **	File I/O.
  */
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	If this bullet is forced to be inaccurate because of some outside means.
  *A tank *	firing while moving is a good example.
  */
  bool IsInaccurate : 1 = false;

 private:
  // Crude animation flag.
  bool IsToAnimate : 1 = false;

  /*
  ** Is this missile allowed to come in from out of bounds?
  */
  bool IsLocked : 1 = true;

  /*
  **	This is the target of the projectile. It is especially significant for
  *those projectiles *	that home in on a target.
  */
  TARGET TarCom = kTargetNone;

  /*
  **	The speed of this projectile.
  */
  int MaxSpeed = 0;

  /*
  **	The warhead of this projectile.
  */
  WarheadType Warhead = WARHEAD_NONE;

  // Shell for TFixedIHeapClass::Load; Serialize() supplies every value.
  BulletClass() = default;
  friend class TFixedIHeapClass<BulletClass>;
};

class ArchiveReader;
class ArchiveWriter;
extern template void BulletClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void BulletClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_RA_BULLET_H_
