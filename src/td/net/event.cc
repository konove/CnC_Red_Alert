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

/* $Header:   F:\projects\c&c\vcs\code\event.cpv   2.17   16 Oct 1995 16:50:28
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : EVENT.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 12/09/94 *
 *                                                                                             *
 *                  Last Update : June 25, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * EventClass::EventClass -- Construct an id and cell based event.
 ** EventClass::EventClass -- Construct simple target type event. *
 *   EventClass::EventClass -- Constructor for mission change events. *
 *   EventClass::EventClass -- Constructor for navigation computer events. *
 *   EventClass::EventClass -- Constructor for object types affecting cells
 *event.             * EventClass::EventClass -- Constructor for sidebar build
 *events.                           * EventClass::EventClass -- Constructs event
 *to transfer special flags.                     * EventClass::EventClass --
 *Default constructor for event objects.                          *
 *   EventClass::EventClass -- Event for sequencing animations. *
 *   EventClass::EventClass -- Megamission assigned to unit. *
 *   EventClass::Execute -- Execute a queued command. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

/***************************************************************************
** Table of what data is really used in the EventClass struct for different
** events.  This table must be kept current with the EventType enum.
*/
#include "td/net/event.h"

#include <cstdio>
#include <string>
#include <utility>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/pooled.h"
#include "base/strings/format.h"
#include "game/core/game_clock.h"
#include "game/core/object_heap.h"
#include "td/ai/team.h"
#include "td/assets/audio.h"
#include "td/assets/theme.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/core/special_flags.h"
#include "td/data/text.h"
#include "td/editor/mapedit.h"
#include "td/map/world.h"
#include "td/net/network.h"
#include "td/net/session.h"
#include "td/objects/anim.h"
#include "td/objects/building.h"
#include "td/objects/foot.h"
#include "td/objects/house.h"
#include "td/objects/object.h"
#include "td/objects/techno.h"
#include "td/objects/type.h"
#include "td/screens/goptions.h"
#include "td/startup/conquer.h"
#include "td/startup/game_state.h"
#include "td/startup/winstub.h"
#include "td/ui/msglist.h"

#ifdef _WIN32
#include "td/startup/ccdde.h"
#endif

namespace td {

base::EnumArray<EventClass::EventType, unsigned char,
                static_cast<int>(EventClass::LAST_EVENT)>
    EventClass::EventLength = {
        0,                             // EMPTY
        sizeof(GeneralData),           // ALLY
        sizeof(MegaMissionData),       // MEGAMISSION
        sizeof(TargetData),            // IDLE
        sizeof(TargetData),            // SCATTER
        0,                             // DESTRUCT
        0,                             // DEPLOY
        sizeof(PlaceData),             // PLACE
        0,                             // OPTIONS
        sizeof(GeneralData),           // GAMESPEED
        sizeof(SpecificData),          // PRODUCE
        sizeof(RTTIType),              // SUSPEND
        sizeof(RTTIType),              // ABANDON
        sizeof(TargetData),            // PRIMARY
        sizeof(SpecialData),           // SPECIAL_PLACE
        0,                             // EXIT
        sizeof(AnimData),              // ANIMATION
        sizeof(TargetData),            // REPAIR
        sizeof(TargetData),            // SELL
        sizeof(OptionsData),           // SPECIAL
        0,                             // FRAMESYNC
        0,                             //	MESSAGE
        sizeof(FrameInfoData::Delay),  // RESPONSE_TIME
        sizeof(FrameInfoData),         // FRAMEINFO
        sizeof(TimingData),            // TIMING
        sizeof(ProcessTimeData),       // PROCESS_TIME
};

base::EnumArray<EventClass::EventType, const char*,
                static_cast<int>(EventClass::LAST_EVENT)>
    EventClass::EventNames = {
        "EMPTY",     "ALLY",         "MEGAMISSION",   "IDLE",
        "SCATTER",   "DESTRUCT",     "DEPLOY",        "PLACE",
        "OPTIONS",   "GAMESPEED",    "PRODUCE",       "SUSPEND",
        "ABANDON",   "PRIMARY",      "SPECIAL_PLACE", "EXIT",
        "ANIMATION", "REPAIR",       "SELL",          "SPECIAL",
        "FRAMESYNC", "MESSAGE",      "RESPONSE_TIME", "FRAMEINFO",
        "TIMING",    "PROCESS_TIME",
};

/***********************************************************************************************
 * EventClass::EventClass -- Constructs event to transfer special flags. *
 *                                                                                             *
 *    This constructs an event that will transfer the special flags. *
 *                                                                                             *
 * INPUT:   data  -- The special flags to be transported to all linked
 *computers.              *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. *
 *=============================================================================================*/
EventClass::EventClass(SpecialFlags data) : EventClass() {
  ID = static_cast<unsigned>(TheObjectHeaps().house().SlotIndexOf(ThePlayer()));
  Type = SPECIAL;
  // Frame is a 26-bit field in the packet; the global counter is long.
  Frame = static_cast<unsigned>(game::CurrentFrame());
  UpdateData<OptionsData>([&](OptionsData& d) { d.Data = data; });
}

/***********************************************************************************************
 * EventClass::EventClass -- Construct simple target type event. *
 *                                                                                             *
 *    This will construct a generic event that needs only a target parameter.
 *The actual       * event and target values are specified as parameters. *
 *                                                                                             *
 * INPUT:   type  -- The event type to construct. *
 *                                                                                             *
 *          target-- The target value that this event is to apply to. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. *
 *=============================================================================================*/
EventClass::EventClass(EventType type, Target target) : EventClass() {
  ID = static_cast<unsigned>(TheObjectHeaps().house().SlotIndexOf(ThePlayer()));
  Type = type;
  // Frame is a 26-bit field in the packet; the global counter is long.
  Frame = static_cast<unsigned>(game::CurrentFrame());
  UpdateData<TargetData>([&](TargetData& d) { d.Whom = target; });
}

/***********************************************************************************************
 * EventClass::EventClass -- Default constructor for event objects. *
 *                                                                                             *
 *    This constructs a simple event object that requires no parameters other
 *than the         * type of event it is. *
 *                                                                                             *
 * INPUT:   type  -- The type of event to construct. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/27/1994 JLB : Created. *
 *=============================================================================================*/
EventClass::EventClass(EventType type) : EventClass() {
  ID = static_cast<unsigned>(TheObjectHeaps().house().SlotIndexOf(ThePlayer()));
  Type = type;
  // Frame is a 26-bit field in the packet; the global counter is long.
  Frame = static_cast<unsigned>(game::CurrentFrame());
}

/***********************************************************************************************
 * EventClass::EventClass -- Constructor for general-purpose-data events. *
 *                                                                                             *
 * INPUT:   type  -- The type of event to construct. * val   -- data value *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/27/1994 JLB : Created. *
 *=============================================================================================*/
EventClass::EventClass(EventType type, int val) : EventClass() {
  ID = static_cast<unsigned>(TheObjectHeaps().house().SlotIndexOf(ThePlayer()));
  Type = type;
  UpdateData<GeneralData>([&](GeneralData& d) { d.Value = val; });
  // Frame is a 26-bit field in the packet; the global counter is long.
  Frame = static_cast<unsigned>(game::CurrentFrame());
}

/***********************************************************************************************
 * EventClass::EventClass -- Event for sequencing animations. *
 *                                                                                             *
 *    This constructor is used for animations that must be created through the
 *event system.   *
 *                                                                                             *
 * INPUT:   anim  -- The animation that will be created. *
 *                                                                                             *
 *          coord -- The location where the animation is to be created. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/19/1995 JLB : Created. *
 *=============================================================================================*/
EventClass::EventClass(AnimType anim, HousesType owner, Coordinate coord)
    : EventClass() {
  ID = static_cast<unsigned>(TheObjectHeaps().house().SlotIndexOf(ThePlayer()));
  Type = ANIMATION;
  // Frame is a 26-bit field in the packet; the global counter is long.
  Frame = static_cast<unsigned>(game::CurrentFrame());
  UpdateData<AnimData>([&](AnimData& d) {
    d.What = anim;
    d.Owner = owner;
    d.Where = coord;
  });
}

/***********************************************************************************************
 * EventClass::EventClass -- Megamission assigned to unit. *
 *                                                                                             *
 *    This is the event that is used to assign most missions to units. It
 *combines both the    * mission and the target (navcom and tarcom). *
 *                                                                                             *
 * INPUT:   src      -- The object that this mission is to apply to. *
 *                                                                                             *
 *          mission  -- The mission to assign to this object. *
 *                                                                                             *
 *          target   -- The target to assign to this object's TarCom. *
 *                                                                                             *
 *          destination -- The destination to assign to this object's NavCom. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/18/1995 JLB : Created. *
 *=============================================================================================*/
EventClass::EventClass(Target src, MissionType mission, Target target,
                       Target destination)
    : EventClass() {
  ID = static_cast<unsigned>(TheObjectHeaps().house().SlotIndexOf(ThePlayer()));
  Type = MEGAMISSION;
  // Frame is a 26-bit field in the packet; the global counter is long.
  Frame = static_cast<unsigned>(game::CurrentFrame());
  UpdateData<MegaMissionData>([&](MegaMissionData& d) {
    d.Whom = src;
    d.Mission = mission;
    d.Target = target;
    d.Destination = destination;
  });
}

/***********************************************************************************************
 * EventClass::EventClass -- Constructor for sidebar build events. *
 *                                                                                             *
 *    This constructor is used for events that deal with an object type and an
 *object ID.      * Typically, this is used exclusively by the sidebar. *
 *                                                                                             *
 * INPUT:   type     -- The event type of this object. *
 *                                                                                             *
 *          object   -- The object type number. *
 *                                                                                             *
 *          id       -- The object sub-type number. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/18/1995 JLB : Created. *
 *=============================================================================================*/
EventClass::EventClass(EventType type, RTTIType object, int id) : EventClass() {
  ID = static_cast<unsigned>(TheObjectHeaps().house().SlotIndexOf(ThePlayer()));
  Type = type;
  // Frame is a 26-bit field in the packet; the global counter is long.
  Frame = static_cast<unsigned>(game::CurrentFrame());
  UpdateData<SpecificData>([&](SpecificData& d) {
    d.Type = object;
    d.ID = id;
  });
}

/***********************************************************************************************
 * EventClass::EventClass -- Constructor for object types affecting cells event.
 **
 *                                                                                             *
 *    This constructor is used for those events that have an object type and
 *associated cell.  * Typically, this is for building placement after
 *construction has completed.              *
 *                                                                                             *
 * INPUT:   type     -- The event type for this object. *
 *                                                                                             *
 *          object   -- The object type number (actual object is probably
 *inferred from the    * sidebar data). *
 *                                                                                             *
 *          cell     -- The cell location where this event is to occur. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/18/1995 JLB : Created. *
 *=============================================================================================*/
EventClass::EventClass(EventType type, RTTIType object, Cell cell)
    : EventClass() {
  ID = static_cast<unsigned>(TheObjectHeaps().house().SlotIndexOf(ThePlayer()));
  Type = type;
  // Frame is a 26-bit field in the packet; the global counter is long.
  Frame = static_cast<unsigned>(game::CurrentFrame());
  UpdateData<PlaceData>([&](PlaceData& d) {
    d.Type = object;
    d.Cell = cell;
  });
}

/***********************************************************************************************
 * EventClass::EventClass -- Construct an id and cell based event. *
 *                                                                                             *
 *    This constructor is used for those events that require an ID number and a
 *cell location. *
 *                                                                                             *
 * INPUT:   type  -- The event type this will be. *
 *                                                                                             *
 *          id    -- The arbitrary id number to assign. *
 *                                                                                             *
 *          cell  -- The location for this event. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/18/1995 JLB : Created. *
 *=============================================================================================*/
EventClass::EventClass(EventType type, int id, Cell cell) : EventClass() {
  ID = static_cast<unsigned>(TheObjectHeaps().house().SlotIndexOf(ThePlayer()));
  Type = type;
  // Frame is a 26-bit field in the packet; the global counter is long.
  Frame = static_cast<unsigned>(game::CurrentFrame());
  UpdateData<SpecialData>([&](SpecialData& d) {
    d.ID = id;
    d.Cell = cell;
  });
}

/***********************************************************************************************
 * EventClass::Execute -- Execute a queued command. *
 *                                                                                             *
 *    This routine executes an event. The even must already have been confirmed
 *by any         * remote machine before calling this routine. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/27/1994 JLB : Created. *
 *=============================================================================================*/
// Not const: applies the event to the game.
// NOLINTNEXTLINE(readability-make-member-function-const)
void EventClass::Execute() {
  TechnoClass* techno = nullptr;
  AnimClass* anim = nullptr;
  const HouseClass* house = nullptr;
  int i = 0;
  // #if (0)
  if (Type > PROCESS_TIME) {
    char tempbuf[128];
    absl::SNPrintF(tempbuf, sizeof(tempbuf), "Packet type %d received\n", Type);
    CCDebugString(tempbuf);

    absl::SNPrintF(tempbuf, sizeof(tempbuf), "	ID = %d\n", ID);
    CCDebugString(tempbuf);

    absl::SNPrintF(tempbuf, sizeof(tempbuf), "	Frame = %d\n", Frame);
    CCDebugString(tempbuf);

    absl::SNPrintF(tempbuf, sizeof(tempbuf), "	MPlayer ID = %d\n", MPlayerID);
    CCDebugString(tempbuf);
  }
  // #endif	//(0)

  switch (Type) {
    /*
    **	Make or break alliance.
    */
    case ALLY:
      house = TheObjectHeaps().house().Slot(data<GeneralData>().Value);
      if (TheObjectHeaps().house().Slot(ID)->Is_Ally(house)) {
        TheObjectHeaps().house().Slot(ID)->Make_Enemy(
            static_cast<HousesType>(data<GeneralData>().Value));
      } else {
        TheObjectHeaps().house().Slot(ID)->Make_Ally(
            static_cast<HousesType>(data<GeneralData>().Value));
      }
      break;

    /*
    **	Special self destruct action requested. This is active in the
    *multiplayer mode.
    */
    case DESTRUCT:
      CCDebugString("C&C95 - Resignation packet received\n");
      TheObjectHeaps().house().Slot(ID)->Flag_To_Die();
      TheObjectHeaps().house().Slot(ID)->Resigned = true;
      break;

    /*
    **	Update the special control flags. This is necessary so that in a
    *multiplay *	game, all machines will agree on the rules. If these
    *options change during *	game play, then all players are informed that
    *options have changed.
    */
    case SPECIAL: {
      TheSpecialFlags() = data<OptionsData>().Data;
      HouseClass* sender = TheObjectHeaps().house().Slot(ID);

      std::string message =
          base::FormatRuntime(Text_String(TXT_SPECIAL_WARNING), sender->Name);
      TheSession().messages().Add_Message(
          message.data(),
          base::At(TheSession().text_colors(),
                   static_cast<int>(sender->RemapColor)),
          FontType::k6PointGradient, TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 1200, 0,
          0);
      TheMap().Flag_To_Redraw(false);
    } break;

    /*
    **	Starts or stops repair on the specified object. This event is triggered
    *by the *	player clicking the repair wrench on a building.
    */
    case REPAIR:
      CCDebugString("C&C95 - Repair packet received\n");
      techno = data<TargetData>().Whom.AsTechno();
      if (techno && techno->IsActive) {
        techno->Repair(-1);
      }
      break;

    /*
    **	Tells a building/unit to sell. This event is triggered by the player
    *clicking the *	sell animating cursor over the building or unit.
    */
    case SELL:
      CCDebugString("C&C95 - Sell packet received\n");
      techno = data<TargetData>().Whom.AsTechno();
      if (techno && techno->IsActive &&
          techno->House == TheObjectHeaps().house().Slot(ID)) {
        techno->Sell_Back(-1);
      } else {
        if (data<TargetData>().Whom.kind() == KIND_CELL) {
          TheObjectHeaps().house().Slot(ID)->Sell_Wall(
              data<TargetData>().Whom.AsCell());
        }
      }
      break;

    /*
    **	This even is used to trigger an animation that is generated as a direct
    **	result of player intervention.
    */
    case ANIMATION:
      anim = base::NewPooled<AnimClass>(data<AnimData>().What,
                                        data<AnimData>().Where);
      if (anim && (data<AnimData>().Owner != HOUSE_NONE &&
                   ThePlayer()->Class->House != data<AnimData>().Owner &&
                   !TheSpecialFlags().visible_target)) {
        anim->Make_Invisible();
      }

      break;

    /*
    **	This event will place the specified object at the specified location.
    **	The event is used to place newly constructed buildings down on the map.
    *The *	object type is specified. From this object type, the house can
    *determine the *	exact factory and real object pointer to use.
    */
    case PLACE:
      CCDebugString("C&C95 - Place packet received\n");
      TheObjectHeaps().house().Slot(ID)->Place_Object(data<PlaceData>().Type,
                                                      data<PlaceData>().Cell);
      break;

    /*
    **	This event starts production of the speicified object type. The house
    *can *	determine from the type and ID value, what object to begin
    *production on and *	what factory to use.
    */
    case PRODUCE:
      CCDebugString("C&C95 - Produce packet received\n");
      TheObjectHeaps().house().Slot(ID)->Begin_Production(
          data<SpecificData>().Type, data<SpecificData>().ID);
      break;

    /*
    **	This event is generated when the player puts production on hold. From
    *the *	object type, the factory can be inferred.
    */
    case SUSPEND:
      CCDebugString("C&C95 - Suspend packet received\n");
      TheObjectHeaps().house().Slot(ID)->Suspend_Production(
          data<SpecificData>().Type);
      break;

    /*
    **	This event is generated when the player cancels production of the
    *specified *	object type. From the object type, the exact factory can
    *be inferred.
    */
    case ABANDON:
      CCDebugString("C&C95 - Abandon packet received\n");
      TheObjectHeaps().house().Slot(ID)->Abandon_Production(
          data<SpecificData>().Type);
      break;

    /*
    **	Toggles the primary factory state of the specified building.
    */
    case PRIMARY: {
      CCDebugString("C&C95 - Primary building packet received\n");
      BuildingClass* building = data<TargetData>().Whom.AsBuilding();
      if (building && building->IsActive) {
        building->Toggle_Primary();
      }
    } break;

    /*
    **	This is the general purpose mission control event. Most player
    **	action routes through this event. It sets a unit's mission, TarCom,
    **	and NavCom to the values specified.
    */
    case MEGAMISSION: {
      const auto order = data<MegaMissionData>();
      techno = order.Whom.AsTechno();
      if (techno && techno->IsActive) {
        /*
        **	Fetch a pointer to the object of the mission.
        */
        ObjectClass* object = nullptr;
        if (order.Target) {
          object = order.Target.AsObject();
        } else {
          object = order.Destination.AsObject();
        }

        /*
        **	Break any existing team contact, since it is now invalid.
        */
        if (!techno->IsTethered) {
          techno->Transmit_Message(RADIO_OVER_OUT);
        }
        switch (techno->What_Am_I()) {
          case RTTI_INFANTRY:
          case RTTI_UNIT:
            if (dynamic_cast<FootClass*>(techno)->Team) {
              dynamic_cast<FootClass*>(techno)->Team->Remove(
                  dynamic_cast<FootClass*>(techno));
            }
            break;
          case RTTIType::RTTI_NONE:
          case RTTIType::RTTI_INFANTRYTYPE:
          case RTTIType::RTTI_UNITTYPE:
          case RTTIType::RTTI_AIRCRAFT:
          case RTTIType::RTTI_AIRCRAFTTYPE:
          case RTTIType::RTTI_BUILDING:
          case RTTIType::RTTI_BUILDINGTYPE:
          case RTTIType::RTTI_TERRAIN:
          case RTTIType::RTTI_ABSTRACTTYPE:
          case RTTIType::RTTI_ANIM:
          case RTTIType::RTTI_ANIMTYPE:
          case RTTIType::RTTI_BULLET:
          case RTTIType::RTTI_BULLETTYPE:
          case RTTIType::RTTI_OVERLAY:
          case RTTIType::RTTI_OVERLAYTYPE:
          case RTTIType::RTTI_SMUDGE:
          case RTTIType::RTTI_SMUDGETYPE:
          case RTTIType::RTTI_TEAM:
          case RTTIType::RTTI_TEMPLATE:
          case RTTIType::RTTI_TEMPLATETYPE:
          case RTTIType::RTTI_TERRAINTYPE:
          case RTTIType::RTTI_OBJECT:
          case RTTIType::RTTI_SPECIAL:
          default:
            break;
        }

        if (object && (ThePlayer()->Is_Ally(techno) ||
                       TheSpecialFlags().visible_target)) {
          object->Clicked_As_Target();
        }

        techno->Assign_Mission(order.Mission);

        /*
        **	Guard area mode is handled with care. The specified target is
        *actually *	assigned as the location that should be guarded. In
        *addition, the *	movement destination is immediately set to this
        *new location.
        */
        if (order.Mission == MISSION_GUARD_AREA &&
            //					Data.MegaMission.Target
            //&&
            (techno->What_Am_I() == RTTI_INFANTRY ||
             techno->What_Am_I() == RTTI_UNIT ||
             techno->What_Am_I() == RTTI_AIRCRAFT)) {
          dynamic_cast<FootClass*>(techno)->ArchiveTarget = order.Target;
          techno->Assign_Target(Target{});
          techno->Assign_Destination(order.Target);
        } else {
          techno->Assign_Target(order.Target);
          techno->Assign_Destination(order.Destination);
        }

#ifdef NEVER
        if ((techno->What_Am_I() == RTTI_UNIT ||
             techno->What_Am_I() == RTTI_INFANTRY) &&
            order.Mission == MISSION_GUARD_AREA) {
          dynamic_cast<FootClass*>(techno)->ArchiveTarget = order.Destination;
        }
#endif
      }
    } break;

    /*
    **	Request that the unit/infantry/aircraft go into idle mode.
    */
    case IDLE:
      techno = data<TargetData>().Whom.AsTechno();
      if (techno && techno->IsActive && !techno->IsInLimbo &&
          !techno->IsTethered) {
        techno->Assign_Destination(Target{});
        techno->Assign_Target(Target{});
        techno->Enter_Idle_Mode();
      }
      break;

    /*
    **	Request that the unit/infantry/aircraft scatter from its current
    *location.
    */
    case SCATTER:
      techno = data<TargetData>().Whom.AsTechno();
      if (techno && techno->IsActive && !techno->IsInLimbo &&
          !techno->IsTethered) {
        techno->Scatter(Coordinate{}, true);
      }
      break;

    /*
    **	If we are placing down the ion cannon blast then lets take
    ** care of it.
    */
    case SPECIAL_PLACE:
      CCDebugString("C&C95 - Special blast packet received\n");
      TheObjectHeaps().house().Slot(ID)->Place_Special_Blast(
          static_cast<SpecialWeaponType>(data<SpecialData>().ID),
          data<SpecialData>().Cell);
      break;

    /*
    **	Exit the game.
    ** Give parting message while palette is fading to black.
    */
    case EXIT:
      CCDebugString("C&C95 - Exit game packet received\n");
      TheTheme().Queue_Song(THEME_NONE);
      Stop_Speaking();
      Speak(VOX_CONTROL_EXIT);
      while (Is_Speaking()) {
        Call_Back();
      }
      TheGameState().active() = false;
      break;

    /*
    **	Process the options menu.
    */
    case OPTIONS:
      TheGameState().special_dialog() = SDLG_OPTIONS;
      break;

    /*
    **	Process the options Game Speed
    */
    case GAMESPEED:
      CCDebugString("C&C95 - Game speed packet received\n");
      TheOptions().GameSpeed = static_cast<unsigned>(data<GeneralData>().Value);
      break;

    /*
    **	Adjust connection timing for multiplayer games
    */
    case RESPONSE_TIME:
      char flip[128];
      absl::SNPrintF(flip, sizeof(flip),
                     "C&C95 - Changing MaxAhead to %d frames\n",
                     data<FrameInfoData>().Delay);
      CCDebugString(flip);
      TheSession().max_ahead() = data<FrameInfoData>().Delay;
      break;

    //
    // This event tells all systems to use new timing values.  It's like
    // RESPONSE_TIME, only it works.  It's only used with the
    // COMM_MULTI_E_COMP protocol.
    //
    case TIMING:
      CCDebugString("C&C95 - Timing packet received\n");
      // #if(TIMING_FIX)
      //
      //  If MaxAhead is about to increase, we're vulnerable to a Packet-
      //  Received-Too-Late error, if any system generates an event after
      //  this TIMING event, but before it executes.  So, record the
      //  period of vulnerability's frame start & end values, so we
      //  can reschedule these events to execute after it's over.
      //
      if (std::cmp_greater(data<TimingData>().MaxAhead,
                           TheSession().max_ahead())) {
        TheNetwork().new_max_ahead_frame1() = Frame;
        TheNetwork().new_max_ahead_frame2() =
            Frame + data<TimingData>().MaxAhead;
      }
      // #endif

      TheSession().desired_frame_rate() = data<TimingData>().DesiredFrameRate;
      TheSession().max_ahead() = data<TimingData>().MaxAhead;

      absl::SNPrintF(flip, sizeof(flip),
                     "C&C95 -  Timing packet: DesiredFrameRate = %d\n",
                     data<TimingData>().DesiredFrameRate);
      CCDebugString(flip);
      absl::SNPrintF(flip, sizeof(flip),
                     "C&C95 -  Timing packet: MaxAhead = %d\n",
                     data<TimingData>().MaxAhead);
      CCDebugString(flip);

      /*
      ** If spawned from WChat then we should be getting poked every minute. If
      *not then
      ** deliberately break the max ahead value
      */
      if (TheSpecialFlags().from_wchat) {
#ifdef _WIN32
        TheSession().max_ahead() +=
            DDEServer.Time_Since_Heartbeat() / (70 * 60);
// if (DDEServer.Time_Since_Heartbeat() >= 70*60) CCDebugString ("C&C95 - Missed
// a heartbeat\n");
#endif
      }
      break;

    //
    // This event tells all systems what the other systems' process
    // timing requirements are; it's used to compute a desired frame rate
    // for the game.
    //
    case PROCESS_TIME:
      for (i = 0; i < TheSession().player_count(); i++) {
        if (MPlayerID == base::At(TheSession().player_ids(), i)) {
          base::At(TheSession().their_process_time(), i) =
              data<ProcessTimeData>().AverageTicks;

          char flip_text[128];
          absl::SNPrintF(flip_text, sizeof(flip_text),
                         "C&C95 - Received PROCESS_TIME packet of %04x ticks\n",
                         data<ProcessTimeData>().AverageTicks);
          CCDebugString(flip_text);

          break;
        }
      }
      break;

    /*
    **	Default: do nothing.
    */
    case EventClass::EMPTY:
    case EventClass::DEPLOY:
    case EventClass::FRAMESYNC:
    case EventClass::MESSAGE:
    case EventClass::FRAMEINFO:
    case EventClass::LAST_EVENT:
    default:
      break;
  }
}

}  // namespace td
