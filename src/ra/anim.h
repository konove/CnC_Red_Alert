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

/* $Header: /CounterStrike/ANIM.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : ANIM.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 30, 1994 *
 *                                                                                             *
 *                  Last Update : May 30, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_ANIM_H_
#define CNC_RED_ALERT_RA_ANIM_H_

#include <cstddef>

#include "ra/ccptr.h"
#include "ra/defines.h"
#include "ra/object.h"
#include "ra/stage.h"
#include "ra/type.h"
#include "tech/fixed.h"
#include "tech/wwfile.h"

/**********************************************************************************************
**	This is the class that controls the shape animation objects. Shape
*animation objects are *	displayed over the top of the game map.
*Typically, they are used for explosion and fire *	effects.
*/
class AnimClass final : public ObjectClass, public StageClass {
  /*
  **	This points to the type of animation object this is.
  */
  CCPtr<AnimTypeClass> Class;

 public:
  AnimClass(AnimType animnum, COORDINATE coord, unsigned char timedelay = 0,
            unsigned char loop = 1);
  ~AnimClass() override;
  AnimClass(const AnimClass&) = delete;
  AnimClass& operator=(const AnimClass&) = delete;
  AnimClass(AnimClass&&) = delete;
  AnimClass& operator=(AnimClass&&) = delete;

  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator AnimType() const { return Class->Type; }

  void* operator new(size_t size) noexcept;
  void* operator new(size_t /*unused*/, void* ptr) noexcept { return ptr; }
  void operator delete(void* ptr);

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  static void Init();

  void Attach_To(ObjectClass* obj);
  void Make_Invisible() { IsInvisible = true; }
  static void Do_Atom_Damage(HousesType ownerhouse, CELL cell);

  [[nodiscard]] bool Can_Place_Here(COORDINATE /*unused*/) const {
    return true;
  }
  bool Mark(MarkType mark = MARK_CHANGE) override;
  bool Render(bool forced) override;  // const;
  [[nodiscard]] COORDINATE Center_Coord() const override;
  [[nodiscard]] COORDINATE Sort_Y() const override;
  [[nodiscard]] LayerType In_Which_Layer() const override;
  [[nodiscard]] const ObjectTypeClass& Class_Of() const override {
    return *Class;
  }
  [[nodiscard]] const short* Occupy_List(
      bool /*placement*/ = false) const override;
  [[nodiscard]] const short* Overlap_List(
      bool /*redraw*/ = false) const override;
  void Draw_It(int x, int y, WindowNumberType window) const override;
  void AI() override;
  void Detach(TARGET target, bool all) override;

  /*
  **	File I/O.
  */
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	If this animation is attached to an object, then this points to that
  *object. An *	animation that is attached will follow that object as it moves.
  *This is important *	for animations such as flames and smoke.
  */
  TARGET xObject = kTargetNone;

  /*
  **	If this animation has an owner, then it will be recorded here. An owner
  **	is used when damage is caused by this animation during the middle of its
  **	animation.
  */
  HousesType OwnerHouse = HOUSE_NONE;

  /*
  **	This counter tells how many more times the animation should loop before
  *it *	terminates.
  */
  unsigned char Loops = 1;

 protected:
  void Middle();
  void Start();

 private:
  /*
  **	Delete this animation at the next opportunity. This is flagged when the
  **	animation is to be prematurely ended as a result of some outside event.
  */
  unsigned IsToDelete : 1 = false;

  /*
  **	If the animation has just been created, then don't do any animation
  **	processing until it has been through the render loop at least once.
  */
  unsigned IsBrandNew : 1 = true;

  /*
  **	If this animation is invisible, then this flag will be true. An
  *invisible *	animation is one that is created for the sole purpose of keeping
  *all *	machines synchronized. It will not be displayed.
  */
  unsigned IsInvisible : 1 = false;

  /*
  **	Is this animation in a temporary suspended state?  If so, then it won't
  **	be rendered until this value is zero. The flag will be set to false
  **	after the first countdown timer reaches 0.
  */
  int Delay = 0;

  /*
  **	If this is an animation that damages whatever it is attached to, then
  *this *	value holds the accumulation of fractional damage points. When
  *the accumulated *	fractions reach 256, then one damage point is applied to
  *the attached object.
  */
  fixed Accum;

  // Shell for TFixedIHeapClass::Load; Serialize() supplies every value.
  AnimClass() = default;
  friend class TFixedIHeapClass<AnimClass>;
};

void Shorten_Attached_Anims(ObjectClass* obj);
AnimType Anim_From_Name(const char* name);

class ArchiveReader;
class ArchiveWriter;
extern template void AnimClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void AnimClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_RA_ANIM_H_
