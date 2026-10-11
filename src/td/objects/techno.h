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

/* $Header:   F:\projects\c&c\vcs\code\techno.h_v   2.17   16 Oct 1995 16:46:58
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TECHNO.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 14, 1994 *
 *                                                                                             *
 *                  Last Update : April 14, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_OBJECTS_TECHNO_H_
#define CNC_RED_ALERT_TD_OBJECTS_TECHNO_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "engine/gfx/pixel_buffer.h"
#include "game/sim/door.h"
#include "game/sim/flasher.h"
#include "game/sim/heading.h"
#include "td/core/defines.h"
#include "td/objects/bullet.h"
#include "td/objects/cargo.h"
#include "td/objects/house.h"
#include "td/objects/mission.h"
#include "td/objects/object.h"
#include "td/objects/radio.h"
#include "td/objects/stage.h"
#include "td/objects/type.h"

class ArchiveReader;
class ArchiveWriter;

namespace td {

/****************************************************************************
**	This is the common data between building and units.
*/
class TechnoClass : public ObjectClass,
                    public RadioPeer,
                    public MissionHandler {
 public:
  /*
  **	This flag will be true if the object has been damaged with malace.
  **	Damage received due to friendly fire or wear and tear does not count.
  **	The computer is not allowed to sell a building unless it has been
  **	damaged with malace.
  */
  bool IsTickedOff : 1;

  /*
  **	If this object has inherited the ability to cloak, then this bit will
  **	be set to true.
  */
  bool IsCloakable : 1;

  /*
  **	If this object is designated as special then this flag will be true. For
  **	buildings, this means that it is the primary factory. For units, it
  *means *	that the unit is the team leader.
  */
  bool IsLeader : 1;

  /*
  **	Certain units are flagged as "loaners".  These units are typically
  *transports that *	are created solely for the purpose of delivering
  *reinforcements.  Such "loaner" *	units are not owned by the player and
  *thus cannot be directly controlled.  These *	units will leave the game as
  *soon as they have fulfilled their purpose.
  */
  bool IsALoaner : 1;

  /*
  **	Once a unit enters the map, then this flag is set. This flag is used to
  *make *	sure that a unit doesn't leave the map once it enters the map.
  */
  bool IsLocked : 1;

  /*
  **	Buildings and units with turrets usually have a recoil animation when
  *they *	fire. If this flag is true, then the next rendering of the
  *object will be *	in the "recoil state". The flag will then be cleared
  *pending the next *	firing event.
  */
  bool IsInRecoilState : 1;

  /*
  **	If this unit is "loosely attached" to another unit it is given special
  **	processing. A unit is in such a condition when it is in the process of
  **	unloading from a transport type object. During the unloading process
  **	the transport object must stay still until the unit is free and clear.
  **	At that time it radios the transport object and the "tether" is broken -
  **	freeing both the unit and the transport object.
  */
  bool IsTethered : 1;

  /*
  **	Is this object owned by the player?  If not, then it is owned by the
  *computer *	or remote opponent. This flag facilitates the many logic
  *differences when dealing *	with player's or computer's units or buildings.
  */
  bool IsOwnedByPlayer : 1;

  /*
  **	The more sophisticated game objects must keep track of whether they are
  *discovered *	or not. This is because the state of discovery can often control
  *how the object *	behaves. In addition, this fact is used in radar and
  *user I/O processing.
  */
  bool IsDiscoveredByPlayer : 1;

  /*
  **	This is used to control the computer recognizing this object.
  */
  bool IsDiscoveredByComputer : 1;

  /*
  **	Some game objects can be of the "lemon" variety. This means that they
  *take damage *	even when everything is ok. This adds a little variety
  *to the game.
  */
  bool IsALemon : 1;

  /*
  **	This flag is used to control second shot processing for those units or
  *buildings *	that fire two shots in quick succession. When this flag is true,
  *it indicates that *	the second shot is ready to fire. After this shot is
  *fired, regular rearm timing *	is used rather than the short rearm
  *time.
  */
  bool IsSecondShot : 1;

  /*
  **	This is the house that the unit belongs to.
  */
  HouseClass* House;

  /*
  **	This records the current cloak state for this vehicle.
  */
  game::CloakType Cloak;
  StageCounter CloakingDevice;

  /* (Targeting Computer)
  **	This is the target value for the item that this vehicle should ATTACK.
  *If this *	is a vehicle with a turret, then it may differ from its movement
  *destination.
  */
  Target TarCom;
  Target SuspendedTarCom;

  /*
  **	This is the visible facing for the unit or building.
  */
  game::Heading PrimaryFacing;

  /*
  **	This is the arming countdown. It represents the time necessary
  **	to reload the weapon.
  */
  unsigned char Arm;

  /*
  **	The number of shot this object can fire before running out of ammo. If
  *this *	value is zero, then firing is not allowed. If -1, then there is
  *no ammunition *	limit.
  */
  int Ammo;

  /*
  **	This is the amount of money spent to produce this object. This value
  *really *	only comes into play for the case of buildings that have special
  *"free" *	objects available when purchased at the more expensive rate.
  */
  int PurchasePrice;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  TechnoClass();
  explicit TechnoClass(HousesType house);
  ~TechnoClass() override = default;
  TechnoClass(const TechnoClass&) = delete;
  TechnoClass& operator=(const TechnoClass&) = delete;
  TechnoClass(TechnoClass&&) = delete;
  TechnoClass& operator=(TechnoClass&&) = delete;

  /*
  **	Query functions.
  */
  [[nodiscard]] virtual int Refund_Amount() const;
  virtual Cell Find_Exit_Cell(const TechnoClass* techno) const;
  [[nodiscard]] virtual BuildingClass* Find_Docking_Bay(StructType b,
                                                        bool friendly);
  [[nodiscard]] virtual int Threat_Range(int control) const;
  [[nodiscard]] virtual InfantryType Crew_Type() const;
  [[nodiscard]] const TechnoTypeClass* Techno_Type_Class() const {
    return dynamic_cast<const TechnoTypeClass*>(&Class_Of());
  }
  Cell Nearby_Location(const TechnoClass* from = nullptr) const;
  [[nodiscard]] int Get_Ownable() const override;
  [[nodiscard]] bool Can_Player_Fire() const override;
  [[nodiscard]] bool Can_Player_Move() const override;
  [[nodiscard]] virtual bool Is_Weapon_Equipped() const;
  [[nodiscard]] bool Can_Repair() const override;
  [[nodiscard]] bool Is_Techno() const override;
  [[nodiscard]] HousesType Owner() const override;
  [[nodiscard]] virtual int Risk() const;
  [[nodiscard]] int Value() const override;
  [[nodiscard]] virtual int Rearm_Delay(bool second = true) const;
  ActionType What_Action(ObjectClass* object) override;
  [[nodiscard]] ActionType What_Action(Cell cell) const override;
  [[nodiscard]] virtual int Tiberium_Load() const;
  virtual game::Direction Desired_Load_Dir(ObjectClass* /*unused*/,
                                           Cell& moveto) const;
  [[nodiscard]] virtual int Pip_Count() const;
  [[nodiscard]] virtual game::Direction Fire_Direction() const;

  /*
  **	User I/O.
  */
  void Clicked_As_Target(int count = 7) override;
  bool Select() override;
  virtual void Response_Select();
  virtual void Response_Move();
  virtual void Response_Attack();
  virtual void Player_Assign_Mission(MissionType mission,
                                     Target target = Target{},
                                     Target destination = Target{});

  /*
  **	Combat related.
  */
  // Adds a kill to this object's tally and returns the new total.
  virtual int Made_A_Kill() { return ++kills_; }
  [[nodiscard]] int kills() const { return kills_; }
  void set_kills(int kills) { kills_ = static_cast<uint16_t>(kills); }

  // The missions this object has been given and its place in the current
  // one. Orders go through Assign_Mission() and the functions next to it,
  // which subclasses extend.
  [[nodiscard]] MissionClass& mission_state() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return mission_state_;
  }
  [[nodiscard]] const MissionClass& mission_state() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return mission_state_;
  }

  // The radio set: who this object is in contact with. Messages are sent
  // with Transmit_Message(), which names this object as the sender.
  [[nodiscard]] RadioClass& radio() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return radio_;
  }
  [[nodiscard]] const RadioClass& radio() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return radio_;
  }

  // The flashing that shows the player this object was targeted.
  [[nodiscard]] game::FlasherClass& flasher() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return flasher_;
  }
  [[nodiscard]] const game::FlasherClass& flasher() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return flasher_;
  }

  // The stage counter that drives this object's animation: the frame of a
  // building's or a unit's art, stepped at the rate that was set.
  [[nodiscard]] StageCounter& animation() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return animation_;
  }
  [[nodiscard]] const StageCounter& animation() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return animation_;
  }

  // The passengers this object carries; empty for one that carries none.
  [[nodiscard]] CargoClass& cargo() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return cargo_;
  }
  [[nodiscard]] const CargoClass& cargo() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return cargo_;
  }

  // The door this object opens to load and unload: an APC's or a transport's
  // hatch, a war factory's door. One that has none leaves it closed.
  [[nodiscard]] game::Door& door() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return door_;
  }
  [[nodiscard]] const game::Door& door() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return door_;
  }
  void Base_Is_Attacked(TechnoClass* enemy);
  void Kill_Cargo(TechnoClass* source);
  void Record_The_Kill(TechnoClass* source) override;
  virtual bool Target_Something_Nearby(ThreatType threat = THREAT_NORMAL);
  virtual void Stun();
  [[nodiscard]] bool In_Range(Coordinate coord, int which = 0) const override;
  [[nodiscard]] virtual bool In_Range(Target target, int which = 0) const;
  virtual bool In_Range(const ObjectClass* target, int which = 0) const;
  virtual void Death_Announcement(
      const TechnoClass* source = nullptr) const = 0;
  [[nodiscard]] virtual game::FireErrorType Can_Fire(Target target,
                                                     int which = 0) const;
  [[nodiscard]] virtual Target Greatest_Threat(ThreatType method) const;
  virtual void Assign_Target(Target target);
  // What the object is up to: its current mission, or the queued one while
  // it has none.
  [[nodiscard]] MissionType Get_Mission() const override;
  // Queues `order` to start when the object can change over; see
  // MissionClass::Assign().
  virtual void Assign_Mission(MissionType order);
  // Starts the queued mission. Returns false when there is none.
  bool Commence();
  // Makes `mission` the current one at once, dropping the queued one.
  void Set_Mission(MissionType mission);
  // Sets the mission and target aside and assigns `mission` against
  // `tarcom`; Restore_Mission() brings them back. Returns false from
  // Restore_Mission() when nothing was set aside.
  virtual void Override_Mission(MissionType mission, Target tarcom,
                                Target navcom);
  virtual bool Restore_Mission();

  // MissionHandler. A techno that cannot perform a mission idles for half a
  // minute before it is asked again.
  int Mission_Sleep() override;
  int Mission_Ambush() override;
  int Mission_Attack() override;
  int Mission_Capture() override;
  int Mission_Guard() override;
  int Mission_Guard_Area() override;
  int Mission_Harvest() override;
  int Mission_Hunt() override;
  int Mission_Timed_Hunt() override;
  int Mission_Move() override;
  int Mission_Retreat() override;
  int Mission_Return() override;
  int Mission_Stop() override;
  int Mission_Unload() override;
  int Mission_Enter() override;
  int Mission_Construction() override;
  int Mission_Deconstruction() override;
  int Mission_Repair() override;
  int Mission_Missile() override;
  virtual BulletClass* Fire_At(Target target, int which = 0);
  [[nodiscard]] int Weapon_Range(int which) const override;
  virtual bool Captured(HouseClass* newowner);
  game::ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                               TechnoClass* source) override;
  bool Evaluate_Cell(ThreatType method, uint32_t mask, Cell cell, int range,
                     const TechnoClass** object, int& value) const;
  bool Evaluate_Object(ThreatType method, uint32_t mask, int range,
                       const TechnoClass* object, int& value) const;

  /*
  **	AI.
  */
  void AI() override;
  bool Revealed(HouseClass* house) override;
  // Answers a radio message from `from`, nullptr when it has no sender.
  // Returns RADIO_STATIC for a message that means nothing to this object.
  virtual RadioMessageType Receive_Message(TechnoClass* from,
                                           RadioMessageType message,
                                           int32_t& param);
  // RadioPeer: hands the message to Receive_Message(). Only a techno ever
  // sends one to a techno; any other sender arrives as nullptr.
  RadioMessageType Receive(RadioPeer* from, RadioMessageType message,
                           int32_t& param) final;
  // Sends `message` from this object to `to`, or to the object it is in
  // radio contact with when `to` is nullptr, and returns the answer;
  // RADIO_STATIC when there is nobody to send it to. See RadioClass::Transmit()
  // for what RADIO_HELLO and RADIO_OVER_OUT do to the contact.
  RadioMessageType Transmit_Message(
      RadioMessageType message, int32_t& param = RadioClass::DiscardedParam(),
      TechnoClass* to = nullptr);
  RadioMessageType Transmit_Message(RadioMessageType message, TechnoClass* to);
  // Returns the object this one is in radio contact with, or nullptr when
  // there is no contact.
  [[nodiscard]] TechnoClass* Contact_With_Whom() const;
  // Breaks radio contact: an object in limbo talks to nobody.
  bool Limbo() override;

  /*
  **	File I/O.
  */
  // Field-wise saved-game support.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  virtual std::span<const unsigned char> Remap_Table();
  [[nodiscard]] game::VisualType Visual_Character(bool raw = false) const;
  void Techno_Draw_Object(PixelView& view, std::span<const std::byte> shapefile,
                          int shapenum, int x, int y, WindowNumberType window);
  void Draw_It(PixelView& view, int x, int y, WindowNumberType window) override;
  virtual void Draw_Pips(PixelView& view, int x, int y,
                         WindowNumberType window);
  void Hidden() override;
  bool Mark(MarkType mark) override;
  int Exit_Object(TechnoClass* /*unused*/) override;
  virtual void Do_Uncloak();
  virtual void Do_Cloak();
  void Do_Shimmer() override;

  /*
  **	Movement and animation.
  */
  virtual void Random_Animate();
  virtual void Assign_Destination(Target target);
  void Scatter(Coordinate source = {}, bool forced = false) override;
  virtual void Per_Cell_Process(bool /*unused*/);
  virtual void Enter_Idle_Mode(bool initial = false);

  /*
  **	Map entry and exit logic.
  */
  bool Unlimbo(Coordinate /*coord*/ /*unused*/,
               game::Direction dir = game::Direction::kNorth) override;
  void Detach(Target target, bool all) override;

  /*
  **	Facing translation tables that fix the flaw with 3D studio when
  **	it renders 45 degree angles.
  */
  static const int BodyShape[32];
  //		static int const TurretShape[32];

 private:
  // Kills this object has scored. It is 16 bits wide because the save game
  // stores it that way; the tally wraps past 65535.
  uint16_t kills_{0};

  // The answer to a message no techno class handles itself: the radio's
  // answer to RADIO_HELLO and RADIO_OVER_OUT, a redraw for RADIO_REDRAW,
  // RADIO_STATIC otherwise.
  RadioMessageType Default_Radio_Answer(TechnoClass* from,
                                        RadioMessageType message);

  // Runs the current mission's function when it is due. An object with no
  // strength left performs none.
  void Run_Mission();

  MissionClass mission_state_;
  RadioClass radio_;
  game::FlasherClass flasher_;
  StageCounter animation_;
  CargoClass cargo_;
  game::Door door_;
};

extern template void TechnoClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TechnoClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_OBJECTS_TECHNO_H_
