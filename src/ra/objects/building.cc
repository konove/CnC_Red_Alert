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

/* $Header: /CounterStrike/BUILDING.CPP 5     3/13/97 5:18p Joe_b $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : BUILDING.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : October 27, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * BuildingClass::AI -- Handles non-graphic AI processing for
 *   buildings. * BuildingClass::Active_Click_With -- Handles cell selection for
 *   buildings. * BuildingClass::Animation_AI -- Handles normal building
 *   animation processing. * BuildingClass::Assign_Target -- Assigns a target to
 *   the building. * BuildingClass::Begin_Mode -- Begins an animation mode for
 *   the building. * BuildingClass::BuildingClass -- Constructor for buildings.
 * * BuildingClass::Can_Demolish -- Can the player demolish (sell back) the
 *   building? * BuildingClass::Can_Enter_Cell -- Determines if building can be
 *   placed down. * BuildingClass::Can_Fire -- Determines if this building can
 *   fire. * BuildingClass::Can_Player_Move -- Can this building be moved? *
 *   BuildingClass::Captured -- Captures the building. *
 *   BuildingClass::Center_Coord -- Fetches the center coordinate for the
 *   building. * BuildingClass::Charging_AI -- Handles the special charging
 * logic for Tesla coils. * BuildingClass::Check_Point -- Fetches the landing
 *   checkpoint for the given flight pattern.* BuildingClass::Click_With --
 *   Handles clicking on the map while the building is selected. *
 *   BuildingClass::Crew_Type -- This determines the crew that this object
 *   generates. * BuildingClass::Death_Announcement -- Announce the death of
 * this building. * BuildingClass::Detach -- Handles target removal from the
 * game system. * BuildingClass::Detach_All -- Possibly abandons production
 *   according to factory type. * BuildingClass::Docking_Coord -- Fetches the
 *   coordinate to use for docking. * BuildingClass::Draw_It -- Displays the
 *   building at the location specified. * BuildingClass::Drop_Debris -- Drops
 *   rubble when building is destroyed. * BuildingClass::Enter_Idle_Mode -- The
 *   building will enter its idle mode. * BuildingClass::Exit_Coord --
 * Determines location where object will leave it. * BuildingClass::Exit_Object
 * -- Initiates an object to leave the building. * BuildingClass::Factory_AI --
 *   Handle factory production and initiation. * BuildingClass::Find_Exit_Cell
 * -- Find a clear location to exit an object from this buildin*
 *   BuildingClass::Fire_Direction -- Fetches the direction of firing. *
 *   BuildingClass::Fire_Out -- Handles when attached animation expires. *
 *   BuildingClass::Flush_For_Placement -- Handles clearing a zone for object
 *   placement. * BuildingClass::Get_Image_Data -- Fetch the image pointer for
 *   the building. * BuildingClass::Grand_Opening -- Handles construction
 *   completed special operations. * BuildingClass::Greatest_Threat -- Searches
 *   for target that building can fire upon. * BuildingClass::How_Many_Survivors
 *   -- This determine the maximum number of survivors. * BuildingClass::Init --
 *   Initialize the building system to an empty null state. *
 *   BuildingClass::Limbo -- Handles power adjustment as building goes into
 *   limbo. * BuildingClass::Mark -- Building interface to map rendering system.
 *   * BuildingClass::Mission_Attack -- Handles attack mission for building. *
 *   BuildingClass::Mission_Construction -- Handles mission construction. *
 *   BuildingClass::Mission_Deconstruction -- Handles building deconstruction. *
 *   BuildingClass::Mission_Guard -- Handles guard mission for combat buildings.
 *   * BuildingClass::Mission_Harvest -- Handles refinery unloading harvesters.
 * * BuildingClass::Mission_Missile -- State machine for nuclear missile launch.
 *   * BuildingClass::Mission_Repair -- Handles the repair (active) state for
 *   building. * BuildingClass::Mission_Unload -- Handles the unload mission for
 *   a building. * BuildingClass::Pip_Count -- Determines "full" pips to display
 *   for building. * BuildingClass::Power_Output -- Fetches the current power
 *   output from this building. * BuildingClass::Read_INI -- Reads buildings
 * from INI file. * BuildingClass::Receive_Message -- Handle an incoming message
 * to the building. * BuildingClass::Remove_Gap_Effect -- Stop a gap generator
 *   from jamming cells * BuildingClass::Repair -- Initiates or terminates the
 *   repair process. * BuildingClass::Repair_AI -- Handle the repair (and sell)
 *   logic for the building. * BuildingClass::Revealed -- Reveals the building
 * to the specified house. * BuildingClass::Rotation_AI -- Process any turret
 *   rotation required of this building. * BuildingClass::Sell_Back -- Controls
 *   the sell back (demolish) operation. * BuildingClass::Shape_Number -- Fetch
 *   the shape number for this building. * BuildingClass::Sort_Y -- Returns the
 *   building coordinate used for sorting. * BuildingClass::Take_Damage --
 *   Inflicts damage points upon a building. * BuildingClass::Target_Coord --
 *   Return the coordinate to use when firing on this building. *
 *   BuildingClass::Toggle_Primary -- Toggles the primary factory state. *
 *   BuildingClass::Turret_Facing -- Fetches the turret facing for this
 * building.
 *   * BuildingClass::Unlimbo -- Removes a building from limbo state. *
 *   BuildingClass::Update_Buildables -- Informs sidebar of additional
 *   construction options. * BuildingClass::Value -- Determine the value of this
 *   building. * BuildingClass::What_Action -- Determines action to perform if
 *   click on specified object. * BuildingClass::What_Action -- Determines what
 *   action will occur. * BuildingClass::Write_INI -- Write out the building
 * data to the INI file specified. * BuildingClass::delete -- Deallocates
 * building object. * BuildingClass::new -- Allocates a building object from
 * building pool. * BuildingClass::~BuildingClass -- Destructor for building
 * type objects. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/objects/building.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>

#include "absl/log/check.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/fixed.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "game/core/cell_size.h"
#include "game/core/game_clock.h"
#include "game/core/object_heap.h"
#include "game/sim/direction.h"
#include "game/sim/enums.h"
#include "game/sim/heading.h"
#include "game/sim/mph.h"
#include "game/sim/utracker.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/prebuilt_base.h"
#include "ra/ai/trigger.h"
#include "ra/ai/trigger_type.h"
#include "ra/assets/audio.h"
#include "ra/assets/shape_draw.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/random_pick.h"
#include "ra/core/target.h"
#include "ra/core/type_heaps.h"
#include "ra/data/rules.h"
#include "ra/debug/debug_state.h"
#include "ra/editor/mapedit.h"
#include "ra/map/gscreen.h"
#include "ra/map/power.h"
#include "ra/map/radar.h"
#include "ra/map/world.h"
#include "ra/net/event.h"
#include "ra/net/network.h"
#include "ra/net/session.h"
#include "ra/objects/aircraft.h"
#include "ra/objects/anim.h"
#include "ra/objects/bullet.h"
#include "ra/objects/cell.h"
#include "ra/objects/drive.h"
#include "ra/objects/factory.h"
#include "ra/objects/foot.h"
#include "ra/objects/house.h"
#include "ra/objects/infantry.h"
#include "ra/objects/mission.h"
#include "ra/objects/object.h"
#include "ra/objects/overlay.h"
#include "ra/objects/radio.h"
#include "ra/objects/smudge.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/objects/vessel.h"
#include "ra/objects/weapon.h"
#include "ra/screens/goptions.h"
#include "ra/startup/game_state.h"

namespace ra {

using engine::gfx::ShapeFlags;

// SAM site launcher states kept in Status.
constexpr int kSamReady =
    0;  // Launcher can be facing any direction tracking targets.
constexpr int kSamFiring = 1;  // Stationary while missile is being fired.

/***************************************************************************
**	Center of building offset table.
*/
const base::EnumArray<game::BSizeType, Coordinate> BuildingClass::CenterOffset =
    {
        Coordinate::FromLeptons(0x0080, 0x0080),
        Coordinate::FromLeptons(0x00FF, 0x0080),
        Coordinate::FromLeptons(0x0080, 0x00FF),
        Coordinate::FromLeptons(0x00FF, 0x00FF),
        Coordinate::FromLeptons(0x00FF, 0x0180),
        Coordinate::FromLeptons(0x0180, 0x00FF),
        Coordinate::FromLeptons(0x0180, 0x0180),

        Coordinate::FromLeptons(0x0200, 0x00FF),

        Coordinate::FromLeptons(0x0280, 0x0280),
};

/***********************************************************************************************
 * BuildingClass::Receive_Message -- Handle an incoming message to the building.
 **
 *                                                                                             *
 *    This routine handles an incoming message to the building. Messages
 *regulate the          * various cooperative ventures between buildings and
 *units. This might include such        * actions as coordinating the
 *construction yard animation with the actual building's       * construction
 *animation.                                                                  *
 *                                                                                             *
 * INPUT:   from     -- The originator of the message received. *
 *                                                                                             *
 *          message  -- The radio message received. *
 *                                                                                             *
 *          param    -- Reference to an optional parameter that might be used to
 *return        * extra information to the message originator. *
 *                                                                                             *
 * OUTPUT:  Returns with the response to the message (typically, this is just
 *RADIO_OK).       *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/09/1994 JLB : Created. * 06/26/1995 JLB : Forces refinery load
 *anim to start immediately.                          * 08/13/1995 JLB : Uses
 *ScenarioInit for special loose "CAN_LOAD" check.                    *
 *=============================================================================================*/
RadioMessageType BuildingClass::Receive_Message(TechnoClass* from,
                                                RadioMessageType message,
                                                int32_t& param) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  switch (message) {
    /*
    **	This message is received as a request to attach/load/dock with this
    *building. *	Verify that this is allowed and return the appropriate
    *response.
    */
    case RADIO_CAN_LOAD:
      TechnoClass::Receive_Message(from, message, param);
      if (!House->Is_Ally(from)) {
        return RADIO_STATIC;
      }
      if (mission_state().current() == MISSION_CONSTRUCTION ||
          mission_state().current() == MISSION_DECONSTRUCTION ||
          BState == game::BSTATE_CONSTRUCTION ||
          (!TheWorld().scenario_init() && radio().InContact() &&
           Contact_With_Whom() != from)) {
        return RADIO_NEGATIVE;
      }
      switch (Class->Type) {
        case STRUCT_AIRSTRIP:
          if (from->What_Am_I() == RTTI_AIRCRAFT &&
              dynamic_cast<const AircraftClass*>(from)->Class->IsFixedWing) {
            return RADIO_ROGER;
          }
          break;

        case STRUCT_HELIPAD:
          if (from->What_Am_I() == RTTI_AIRCRAFT &&
              !dynamic_cast<const AircraftClass*>(from)->Class->IsFixedWing) {
            return RADIO_ROGER;
          }
          break;

        case STRUCT_REPAIR:
          if ((from->What_Am_I() == RTTI_UNIT ||
               from->What_Am_I() == RTTI_AIRCRAFT) &&
              (Transmit_Message(RADIO_ON_DEPOT, from) != RADIO_ROGER)) {
            return RADIO_ROGER;
          }

          return RADIO_NEGATIVE;

        case STRUCT_REFINERY:
          if (from->What_Am_I() == RTTI_UNIT &&
              *dynamic_cast<UnitClass*>(from) == UNIT_HARVESTER &&
              (TheWorld().scenario_init() ||
               !cargo().Is_Something_Attached())) {
            return RADIO_ROGER;
          }
          break;

        case StructType::STRUCT_NONE:
        case StructType::STRUCT_ADVANCED_TECH:
        case StructType::STRUCT_IRON_CURTAIN:
        case StructType::STRUCT_WEAP:
        case StructType::STRUCT_CHRONOSPHERE:
        case StructType::STRUCT_PILLBOX:
        case StructType::STRUCT_CAMOPILLBOX:
        case StructType::STRUCT_RADAR:
        case StructType::STRUCT_GAP:
        case StructType::STRUCT_TURRET:
        case StructType::STRUCT_AAGUN:
        case StructType::STRUCT_FLAME_TURRET:
        case StructType::STRUCT_CONST:
        case StructType::STRUCT_STORAGE:
        case StructType::STRUCT_SAM:
        case StructType::STRUCT_POWER:
        case StructType::STRUCT_ADVANCED_POWER:
        case StructType::STRUCT_SOVIET_TECH:
        case StructType::STRUCT_HOSPITAL:
        case StructType::STRUCT_BARRACKS:
        case StructType::STRUCT_TENT:
        case StructType::STRUCT_KENNEL:
        case StructType::STRUCT_BIO_LAB:
        case StructType::STRUCT_MISSION:
        case StructType::STRUCT_SHIP_YARD:
        case StructType::STRUCT_SUB_PEN:
        case StructType::STRUCT_MSLO:
        case StructType::STRUCT_FORWARD_COM:
        case StructType::STRUCT_TESLA:
        case StructType::STRUCT_FAKEWEAP:
        case StructType::STRUCT_FAKECONST:
        case StructType::STRUCT_FAKE_YARD:
        case StructType::STRUCT_FAKE_PEN:
        case StructType::STRUCT_FAKE_RADAR:
        case StructType::STRUCT_SANDBAG_WALL:
        case StructType::STRUCT_CYCLONE_WALL:
        case StructType::STRUCT_BRICK_WALL:
        case StructType::STRUCT_BARBWIRE_WALL:
        case StructType::STRUCT_WOOD_WALL:
        case StructType::STRUCT_FENCE:
        case StructType::STRUCT_AVMINE:
        case StructType::STRUCT_APMINE:
        case StructType::STRUCT_V01:
        case StructType::STRUCT_V02:
        case StructType::STRUCT_V03:
        case StructType::STRUCT_V04:
        case StructType::STRUCT_V05:
        case StructType::STRUCT_V06:
        case StructType::STRUCT_V07:
        case StructType::STRUCT_V08:
        case StructType::STRUCT_V09:
        case StructType::STRUCT_V10:
        case StructType::STRUCT_V11:
        case StructType::STRUCT_V12:
        case StructType::STRUCT_V13:
        case StructType::STRUCT_V14:
        case StructType::STRUCT_V15:
        case StructType::STRUCT_V16:
        case StructType::STRUCT_V17:
        case StructType::STRUCT_V18:
        case StructType::STRUCT_PUMP:
        case StructType::STRUCT_V20:
        case StructType::STRUCT_V21:
        case StructType::STRUCT_V22:
        case StructType::STRUCT_V23:
        case StructType::STRUCT_V24:
        case StructType::STRUCT_V25:
        case StructType::STRUCT_V26:
        case StructType::STRUCT_V27:
        case StructType::STRUCT_V28:
        case StructType::STRUCT_V29:
        case StructType::STRUCT_V30:
        case StructType::STRUCT_V31:
        case StructType::STRUCT_V32:
        case StructType::STRUCT_V33:
        case StructType::STRUCT_V34:
        case StructType::STRUCT_V35:
        case StructType::STRUCT_V36:
        case StructType::STRUCT_V37:
        case StructType::STRUCT_BARREL:
        case StructType::STRUCT_BARREL3:
        case StructType::STRUCT_QUEEN:
        case StructType::STRUCT_LARVA1:
        case StructType::STRUCT_LARVA2:
        default:
          break;
      }
      return RADIO_STATIC;

    /*
    **	This message is received when the object has attached itself to this
    **	building.
    */
    case RADIO_IM_IN:
      if (mission_state().current() == MISSION_DECONSTRUCTION) {
        return RADIO_NEGATIVE;
      }
      switch (Class->Type) {
        case STRUCT_REPAIR:
          IsReadyToCommence = true;
          Assign_Mission(MISSION_REPAIR);
          from->Assign_Mission(MISSION_SLEEP);
          return RADIO_ROGER;

        case STRUCT_AIRSTRIP:
        case STRUCT_HELIPAD:
          Assign_Mission(MISSION_REPAIR);
          from->Assign_Mission(MISSION_SLEEP);
          return RADIO_ROGER;

        case STRUCT_REFINERY:
          Mark(MARK_CHANGE);
          from->Assign_Mission(MISSION_UNLOAD);
          return RADIO_ROGER;

        case StructType::STRUCT_NONE:
        case StructType::STRUCT_ADVANCED_TECH:
        case StructType::STRUCT_IRON_CURTAIN:
        case StructType::STRUCT_WEAP:
        case StructType::STRUCT_CHRONOSPHERE:
        case StructType::STRUCT_PILLBOX:
        case StructType::STRUCT_CAMOPILLBOX:
        case StructType::STRUCT_RADAR:
        case StructType::STRUCT_GAP:
        case StructType::STRUCT_TURRET:
        case StructType::STRUCT_AAGUN:
        case StructType::STRUCT_FLAME_TURRET:
        case StructType::STRUCT_CONST:
        case StructType::STRUCT_STORAGE:
        case StructType::STRUCT_SAM:
        case StructType::STRUCT_POWER:
        case StructType::STRUCT_ADVANCED_POWER:
        case StructType::STRUCT_SOVIET_TECH:
        case StructType::STRUCT_HOSPITAL:
        case StructType::STRUCT_BARRACKS:
        case StructType::STRUCT_TENT:
        case StructType::STRUCT_KENNEL:
        case StructType::STRUCT_BIO_LAB:
        case StructType::STRUCT_MISSION:
        case StructType::STRUCT_SHIP_YARD:
        case StructType::STRUCT_SUB_PEN:
        case StructType::STRUCT_MSLO:
        case StructType::STRUCT_FORWARD_COM:
        case StructType::STRUCT_TESLA:
        case StructType::STRUCT_FAKEWEAP:
        case StructType::STRUCT_FAKECONST:
        case StructType::STRUCT_FAKE_YARD:
        case StructType::STRUCT_FAKE_PEN:
        case StructType::STRUCT_FAKE_RADAR:
        case StructType::STRUCT_SANDBAG_WALL:
        case StructType::STRUCT_CYCLONE_WALL:
        case StructType::STRUCT_BRICK_WALL:
        case StructType::STRUCT_BARBWIRE_WALL:
        case StructType::STRUCT_WOOD_WALL:
        case StructType::STRUCT_FENCE:
        case StructType::STRUCT_AVMINE:
        case StructType::STRUCT_APMINE:
        case StructType::STRUCT_V01:
        case StructType::STRUCT_V02:
        case StructType::STRUCT_V03:
        case StructType::STRUCT_V04:
        case StructType::STRUCT_V05:
        case StructType::STRUCT_V06:
        case StructType::STRUCT_V07:
        case StructType::STRUCT_V08:
        case StructType::STRUCT_V09:
        case StructType::STRUCT_V10:
        case StructType::STRUCT_V11:
        case StructType::STRUCT_V12:
        case StructType::STRUCT_V13:
        case StructType::STRUCT_V14:
        case StructType::STRUCT_V15:
        case StructType::STRUCT_V16:
        case StructType::STRUCT_V17:
        case StructType::STRUCT_V18:
        case StructType::STRUCT_PUMP:
        case StructType::STRUCT_V20:
        case StructType::STRUCT_V21:
        case StructType::STRUCT_V22:
        case StructType::STRUCT_V23:
        case StructType::STRUCT_V24:
        case StructType::STRUCT_V25:
        case StructType::STRUCT_V26:
        case StructType::STRUCT_V27:
        case StructType::STRUCT_V28:
        case StructType::STRUCT_V29:
        case StructType::STRUCT_V30:
        case StructType::STRUCT_V31:
        case StructType::STRUCT_V32:
        case StructType::STRUCT_V33:
        case StructType::STRUCT_V34:
        case StructType::STRUCT_V35:
        case StructType::STRUCT_V36:
        case StructType::STRUCT_V37:
        case StructType::STRUCT_BARREL:
        case StructType::STRUCT_BARREL3:
        case StructType::STRUCT_QUEEN:
        case StructType::STRUCT_LARVA1:
        case StructType::STRUCT_LARVA2:
        default:
          break;
      }
      break;

    /*
    **	Docking maneuver maintenance message. See if new order should be given
    *to the *	unit trying to dock.
    */
    case RADIO_DOCKING:
      TechnoClass::Receive_Message(from, message, param);

      /*
      **	When in radio contact for loading, the refinery starts
      **	flashing the lights.
      */
      if (*this == STRUCT_REFINERY && BState != game::BSTATE_FULL) {
        Begin_Mode(game::BSTATE_FULL);
      }

      /*
      **	If this building is already in radio contact, then it might
      **	be able to satisfy the request to load by bumping off any
      **	preoccupying task.
      */
      if (*this == STRUCT_REPAIR) {
        if (Contact_With_Whom() != from) {
          if ((Transmit_Message(RADIO_ON_DEPOT) == RADIO_ROGER) &&
              (Transmit_Message(RADIO_NEED_REPAIR) == RADIO_NEGATIVE)) {
            Transmit_Message(RADIO_RUN_AWAY);
            return RADIO_ROGER;
          }
            //					} else {
            //						if
            //(Transmit_Message(RADIO_NEED_TO_MOVE, from) == RADIO_ROGER) {
            //							param =
            //(long)As_Target();
            // Transmit_Message(RADIO_MOVE_HERE, param, from);
            //						}

        } else {
          if (Transmit_Message(RADIO_NEED_REPAIR) == RADIO_NEGATIVE) {
            return RADIO_NEGATIVE;
          }
        }
      }

      /*
      **	Establish contact with the object if this building isn't already
      *in contact *	with another.
      */
      if (!radio().InContact()) {
        Transmit_Message(RADIO_HELLO, from);
      }

      if (Transmit_Message(RADIO_NEED_TO_MOVE) == RADIO_ROGER) {
        switch (Class->Type) {
          case STRUCT_AIRSTRIP:
          case STRUCT_HELIPAD:
            param = As_Target().packed();
            break;

          case STRUCT_REPAIR:
            Transmit_Message(RADIO_TETHER);
            param = Target::FromCell(Cell::Containing(Center_Coord())).packed();
            break;

          case STRUCT_REFINERY:
            param =
                Target::FromCell(Cell::Containing(AdjacentCell(
                                     Center_Coord(), game::Direction::kSouth)))
                    .packed();
            break;
          case StructType::STRUCT_NONE:
          case StructType::STRUCT_ADVANCED_TECH:
          case StructType::STRUCT_IRON_CURTAIN:
          case StructType::STRUCT_WEAP:
          case StructType::STRUCT_CHRONOSPHERE:
          case StructType::STRUCT_PILLBOX:
          case StructType::STRUCT_CAMOPILLBOX:
          case StructType::STRUCT_RADAR:
          case StructType::STRUCT_GAP:
          case StructType::STRUCT_TURRET:
          case StructType::STRUCT_AAGUN:
          case StructType::STRUCT_FLAME_TURRET:
          case StructType::STRUCT_CONST:
          case StructType::STRUCT_STORAGE:
          case StructType::STRUCT_SAM:
          case StructType::STRUCT_POWER:
          case StructType::STRUCT_ADVANCED_POWER:
          case StructType::STRUCT_SOVIET_TECH:
          case StructType::STRUCT_HOSPITAL:
          case StructType::STRUCT_BARRACKS:
          case StructType::STRUCT_TENT:
          case StructType::STRUCT_KENNEL:
          case StructType::STRUCT_BIO_LAB:
          case StructType::STRUCT_MISSION:
          case StructType::STRUCT_SHIP_YARD:
          case StructType::STRUCT_SUB_PEN:
          case StructType::STRUCT_MSLO:
          case StructType::STRUCT_FORWARD_COM:
          case StructType::STRUCT_TESLA:
          case StructType::STRUCT_FAKEWEAP:
          case StructType::STRUCT_FAKECONST:
          case StructType::STRUCT_FAKE_YARD:
          case StructType::STRUCT_FAKE_PEN:
          case StructType::STRUCT_FAKE_RADAR:
          case StructType::STRUCT_SANDBAG_WALL:
          case StructType::STRUCT_CYCLONE_WALL:
          case StructType::STRUCT_BRICK_WALL:
          case StructType::STRUCT_BARBWIRE_WALL:
          case StructType::STRUCT_WOOD_WALL:
          case StructType::STRUCT_FENCE:
          case StructType::STRUCT_AVMINE:
          case StructType::STRUCT_APMINE:
          case StructType::STRUCT_V01:
          case StructType::STRUCT_V02:
          case StructType::STRUCT_V03:
          case StructType::STRUCT_V04:
          case StructType::STRUCT_V05:
          case StructType::STRUCT_V06:
          case StructType::STRUCT_V07:
          case StructType::STRUCT_V08:
          case StructType::STRUCT_V09:
          case StructType::STRUCT_V10:
          case StructType::STRUCT_V11:
          case StructType::STRUCT_V12:
          case StructType::STRUCT_V13:
          case StructType::STRUCT_V14:
          case StructType::STRUCT_V15:
          case StructType::STRUCT_V16:
          case StructType::STRUCT_V17:
          case StructType::STRUCT_V18:
          case StructType::STRUCT_PUMP:
          case StructType::STRUCT_V20:
          case StructType::STRUCT_V21:
          case StructType::STRUCT_V22:
          case StructType::STRUCT_V23:
          case StructType::STRUCT_V24:
          case StructType::STRUCT_V25:
          case StructType::STRUCT_V26:
          case StructType::STRUCT_V27:
          case StructType::STRUCT_V28:
          case StructType::STRUCT_V29:
          case StructType::STRUCT_V30:
          case StructType::STRUCT_V31:
          case StructType::STRUCT_V32:
          case StructType::STRUCT_V33:
          case StructType::STRUCT_V34:
          case StructType::STRUCT_V35:
          case StructType::STRUCT_V36:
          case StructType::STRUCT_V37:
          case StructType::STRUCT_BARREL:
          case StructType::STRUCT_BARREL3:
          case StructType::STRUCT_QUEEN:
          case StructType::STRUCT_LARVA1:
          case StructType::STRUCT_LARVA2:
          default:
            break;
        }

        /*
        **	Tell the harvester to move to the docking pad of the building.
        */
        if (Transmit_Message(RADIO_MOVE_HERE, param) == RADIO_YEA_NOW_WHAT) {
          /*
          **	Since the harvester is already there, tell it to begin the
          *backup *	procedure now. If it can't, then tell it to get outta
          *here.
          */
          Transmit_Message(RADIO_TETHER);
          if (*this == STRUCT_REFINERY &&
              Transmit_Message(RADIO_BACKUP_NOW, from) != RADIO_ROGER) {
            from->Scatter(Coordinate{}, true, true);
          }
        }
      }
      return RADIO_ROGER;

    /*
    **	If a transport or harvester is requesting permission to head toward,
    *dock *	and load/unload, check to make sure that this is allowed given
    *the current *	state of the building.
    */
    case RADIO_ARE_REFINERY:
      if (cargo().Is_Something_Attached() || radio().InContact() || IsInLimbo ||
          House->Class->House != from->Owner() ||
          *this != STRUCT_REFINERY /* && *this != STRUCT_REPAIR*/) {
        return RADIO_NEGATIVE;
      }
      return RADIO_ROGER;

    /*
    **	Someone is telling us that it is starting construction. This should only
    **	occur if this is a construction yard and a building was just placed on
    **	the map.
    */
    case RADIO_BUILDING:
      Assign_Mission(MISSION_REPAIR);
      TechnoClass::Receive_Message(from, message, param);
      return RADIO_ROGER;

    /*
    **	Someone is telling us that they have finished construction. This should
    **	only occur if this is a construction yard and the building that was
    *being *	constructed has finished. In this case, stop the construction
    *yard *	animation.
    */
    case RADIO_COMPLETE:
      if (mission_state().current() != MISSION_DECONSTRUCTION) {
        Assign_Mission(MISSION_GUARD);
      }
      TechnoClass::Receive_Message(from, message, param);
      return RADIO_ROGER;

    /*
    **	This message may occur unexpectedly if the unit in contact with this
    **	building is suddenly destroyed. Handle any cleanup necessary. For
    *example, *	a construction yard should stop its construction animation in
    *this case.
    */
    case RADIO_OVER_OUT:
      Begin_Mode(game::BSTATE_IDLE);
      TechnoClass::Receive_Message(from, message, param);
      return RADIO_ROGER;

    /*
    **	This message is received when an object has completely left
    ** building. Sometimes special cleanup action is required when
    **	this event occurs.
    */
    case RADIO_UNLOADED:
      if (*this == STRUCT_REPAIR) {
        if (Distance(from) < 0x0180) {
          return RADIO_ROGER;
        }
      }
      TechnoClass::Receive_Message(from, message, param);
      if (*this == STRUCT_WEAP || *this == STRUCT_AIRSTRIP ||
          *this == STRUCT_REPAIR) {
        return RADIO_RUN_AWAY;
      }
      return RADIO_ROGER;

    case RadioMessageType::RADIO_STATIC:
    case RadioMessageType::RADIO_ROGER:
    case RadioMessageType::RADIO_HELLO:
    case RadioMessageType::RADIO_PICK_UP:
    case RadioMessageType::RADIO_ATTACH:
    case RadioMessageType::RADIO_DELIVERY:
    case RadioMessageType::RADIO_HOLD_STILL:
    case RadioMessageType::RADIO_UNLOAD:
    case RadioMessageType::RADIO_NEGATIVE:
    case RadioMessageType::RADIO_REDRAW:
    case RadioMessageType::RADIO_TRYING_TO_LOAD:
    case RadioMessageType::RADIO_MOVE_HERE:
    case RadioMessageType::RADIO_NEED_TO_MOVE:
    case RadioMessageType::RADIO_YEA_NOW_WHAT:
    case RadioMessageType::RADIO_BACKUP_NOW:
    case RadioMessageType::RADIO_RUN_AWAY:
    case RadioMessageType::RADIO_TETHER:
    case RadioMessageType::RADIO_UNTETHER:
    case RadioMessageType::RADIO_REPAIR:
    case RadioMessageType::RADIO_PREPARED:
    case RadioMessageType::RADIO_ATTACK_THIS:
    case RadioMessageType::RADIO_RELOAD:
    case RadioMessageType::RADIO_CANT:
    case RadioMessageType::RADIO_ALL_DONE:
    case RadioMessageType::RADIO_NEED_REPAIR:
    case RadioMessageType::RADIO_ON_DEPOT:
    default:
      break;
  }

  /*
  **	Pass along the message to the default message handler in the radio
  *itself.
  */
  return TechnoClass::Receive_Message(from, message, param);
}

/***********************************************************************************************
 * BuildingClass::Draw_It -- Displays the building at the location specified. *
 *                                                                                             *
 *    This is the low level graphic routine that displays the building at the
 *location         * specified. *
 *                                                                                             *
 * INPUT:   x,y   -- The coordinate to draw the building at. *
 *                                                                                             *
 *          window   -- The clipping window to use. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/20/1994 JLB : Created. * 06/27/1994 JLB : Takes a clipping
 *window parameter.                                       * 07/06/1995 JLB :
 *Handles damaged silos correctly.                                         *
 *=============================================================================================*/
void BuildingClass::Draw_It(PixelView& view, int x, int y,
                            WindowNumberType window) const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  /*
  **	The shape file to use for rendering depends on whether the building
  **	is undergoing construction or not.
  */
  const auto shapefile = Get_Image_Data();
  if (shapefile.empty()) {
    return;
  }

  /*
  **	Actually draw the building shape.
  */
  Techno_Draw_Object(view, shapefile, Shape_Number(), x, y, window);

  /*
  ** Patch for adding overlay onto weapon factory.  Only add the overlay if
  ** the building has more than 1 hp.  Also, if the building's in radio
  ** contact, he must be unloading a constructed vehicle, so draw that
  ** vehicle before drawing the overlay.
  */
  if (BState != game::BSTATE_CONSTRUCTION) {
    /*
    **	A Tethered object is always rendered AFTER the building.
    */
    if (IsTethered && radio().InContact() && !Contact_With_Whom()->IsInLimbo &&
        Contact_With_Whom()->What_Am_I() != RTTI_BUILDING) {
      TechnoClass* contact = Contact_With_Whom();

      DCHECK(contact->IsActive);
      const int xxx = x + (Lepton_To_Pixel(contact->Render_Coord().x()) -
                           Lepton_To_Pixel(Render_Coord().x()));
      const int yyy = y + (Lepton_To_Pixel(contact->Render_Coord().y()) -
                           Lepton_To_Pixel(Render_Coord().y()));
      contact->Draw_It(view, xxx, yyy, window);
      contact->IsToDisplay = false;
    }

    /*
    **	Draw the weapon factory custom overlay graphic.
    */
    if (*this == STRUCT_WEAP || *this == STRUCT_FAKEWEAP) {
      int shapenum = door().Frame();
      if (Health_Ratio() <= TheRules().ConditionYellow) {
        shapenum += 4;
      }
      Techno_Draw_Object(view, BuildingTypeClass::WarFactoryOverlay, shapenum,
                         x, y, window);
    }

    /*
    **	Draw any repair feedback graphic required.
    */
    if (IsRepairing && IsWrenchVisible) {
      CC_Draw_Shape(view, ObjectTypeClass::SelectShapes, kSelectWrench, x, y,
                    window, ShapeFlags::kCentered);
    }
  }

  TechnoClass::Draw_It(view, x, y, window);

  /*
  ** If this is a factory that we're spying on, show what it's producing
  */
  if (SpiedBy & base::Bit<uint32_t>(ThePlayer()->Class->House) && IsSelected) {
    /*
    **	Fetch the factory that is associate with this building. For computer
    *controlled buildings, the *	factory pointer is integral to the
    *building itself. For human controlled buildings, the factory *	pointer
    *is part of the house structure and must be retrieved from there.
    */
    const FactoryClass* factory = nullptr;
    if (House->IsHuman) {
      factory = House->Fetch_Factory(Class->ToBuild);
    } else {
      factory = Factory;
    }

    /*
    **	If there is a factory associated with this building, then fetch any
    *attached *	object under production and display its cameo image over the top
    *of this building.
    */
    if (factory != nullptr) {
      const TechnoClass* obj = factory->Get_Object();
      if (obj != nullptr) {
        CC_Draw_Shape(view, obj->Techno_Type_Class()->Get_Cameo_Data(), 0, x, y,
                      window, ShapeFlags::kCentered, {});
      }
    }
  }
}

/***********************************************************************************************
 * BuildingClass::Shape_Number -- Fetch the shape number for this building. *
 *                                                                                             *
 *    This routine will examine the current state of the building and return
 *with the shape    * number to use. The shape number is subordinate to the
 *building graphic image data.       *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the shape number to use when rendering this building. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/29/1996 JLB : Created. *
 *=============================================================================================*/
int BuildingClass::Shape_Number() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  int shapenum = animation().Fetch_Stage();

  /*
  **	The shape file to use for rendering depends on whether the building
  **	is undergoing construction or not.
  */
  if (BState == game::BSTATE_CONSTRUCTION) {
    /*
    **	If the building is deconstructing, then the display frame progresses
    **	from the end to the beginning. Reverse the shape number accordingly.
    */
    if (mission_state().current() == MISSION_DECONSTRUCTION) {
      shapenum = Class->Anims.at(BState).Start + Class->Anims.at(BState).Count -
                 1 - shapenum;
    }

  } else {
    /*
    **	If this is a camouflaged pill box and it is not owned by the player,
    *then *	it is displayed with the MEGA-camouflaged imagery.
    */
    if (!IsOwnedByPlayer && *this == STRUCT_CAMOPILLBOX) {
      shapenum += 1;
    }

    /*
    **	The Tesla Coil has a stage value that can be overridden by
    **	its current state.
    */
    if (*this == STRUCT_TESLA) {
      if (IsCharged) {
        shapenum = 3;
      } else {
        if (IsCharging) {
          shapenum = animation().Fetch_Stage();
        } else {
          shapenum = 0;
        }
      }
    }

    /*
    **	Buildings that contain a turret handle their shape determination
    **	differently than normal buildings. They need to take into consideration
    **	the direction the turret is facing.
    */
    if (Class->IsTurretEquipped) {
      shapenum =
          base::At(BodyShape, game::BodyFrame32(PrimaryFacing.current()));

      if (*this == STRUCT_SAM) {
        /*
        **	SAM sites that are free to rotate fetch their animation frame
        **	from the building's turret facing. All other animation stages
        **	fetch their frame from the embedded animation sequencer.
        */
        if (Health_Ratio() <= TheRules().ConditionYellow) {
          shapenum += 35;
        }
      } else {
        if (IsInRecoilState) {
          shapenum += 32;
        }
        if (Health_Ratio() <= TheRules().ConditionYellow) {
          shapenum += 64;
        }
      }
    } else {
      /*
      **	If it is a significantly damaged weapons factory, it is shown in
      **	the worst state possible.
      */
      if (*this == STRUCT_WEAP || *this == STRUCT_FAKEWEAP) {
        shapenum = 0;
        if (Health_Ratio() <= TheRules().ConditionYellow) {
          shapenum = 1;
        }

      } else {
        /*
        **	Special render stage for silos. The stage is dependent on the
        *current *	Tiberium collected as it relates to Tiberium capacity.
        */
        if (*this == STRUCT_STORAGE) {
          const int level =
              House->Capacity
                  ? static_cast<int>(House->Tiberium * 5 / House->Capacity)
                  : 0;

          shapenum += std::clamp(level, 0, 4);
          if (Health_Ratio() <= TheRules().ConditionYellow) {
            shapenum += 5;
          }

        } else {
          /*
          **	If below half strenth, then show the damage frames of the
          **	building.
          */
          if (Health_Ratio() <= TheRules().ConditionYellow) {
            const int last1 = Class->Anims.at(game::BSTATE_IDLE).Start +
                              Class->Anims.at(game::BSTATE_IDLE).Count;
            int last2 = Class->Anims.at(game::BSTATE_ACTIVE).Start +
                        Class->Anims.at(game::BSTATE_ACTIVE).Count;
            int largest = std::max(last1, last2);
            last2 = Class->Anims.at(game::BSTATE_AUX1).Start +
                    Class->Anims.at(game::BSTATE_AUX1).Count;
            largest = std::max(largest, last2);
            last2 = Class->Anims.at(game::BSTATE_AUX2).Start +
                    Class->Anims.at(game::BSTATE_AUX2).Count;
            largest = std::max(largest, last2);
            shapenum += largest;
          }
        }
      }
    }
  }
  return shapenum;
}

/***********************************************************************************************
 * BuildingClass::Mark -- Building interface to map rendering system. *
 *                                                                                             *
 *    This routine is used to mark the map cells so that when it renders * the
 *underlying icons will also be updated as necessary. *
 *                                                                                             *
 * INPUT:   mark  -- Type of image change (MARK_UP, _DOWN, _CHANGE) * MARK_UP --
 *Building is removed.                                                *
 *             MARK_CHANGE -- Building changes shape. * MARK_DOWN -- Building is
 *added.                                                 *
 *                                                                                             *
 * OUTPUT:  bool; Did the mark operation succeed? Failure could be the result of
 *marking down  * when the building is already marked down, or visa versa. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/31/1994 JLB : Created. * 04/15/1994 JLB : Converted to member
 *function.                                            * 04/16/1994 JLB : Added
 *health bar tracking.                                               *
 *   12/23/1994 JLB : Calls low level check before proceeding. * 01/27/1995 JLB
 *: Special road spacer template added.                                      *
 *=============================================================================================*/
bool BuildingClass::Mark(MarkType mark) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (TechnoClass::Mark(mark)) {
    const std::span<const int16_t> occupy = Occupy_List();
    Cell cell = Cell::Containing(Coord);
    SmudgeType bib = SMUDGE_NONE;

    switch (mark) {
      case MARK_UP:
        TheMap().Pick_Up(cell, this);
        if (Class->Bib_And_Offset(bib, cell)) {
          auto* smudge = base::NewPooled<SmudgeClass>(bib);
          if (smudge != nullptr) {
            smudge->Disown(cell);
            base::DeletePooled(smudge);
          }
        }
        break;

      case MARK_DOWN:

        /*
        **	Special wall logic is handled here. A building that is really a
        *wall *	gets converted into an overlay wall type when it is placed down.
        *The *	actual building object itself is destroyed.
        */
        if (Class->IsWall) {
          switch (Class->Type) {
            case STRUCT_BRICK_WALL:
              base::NewPooled<OverlayClass>(OVERLAY_BRICK_WALL, cell,
                                            House->Class->House);
              break;

            case STRUCT_BARBWIRE_WALL:
              base::NewPooled<OverlayClass>(OVERLAY_BARBWIRE_WALL, cell,
                                            House->Class->House);
              break;

            case STRUCT_SANDBAG_WALL:
              base::NewPooled<OverlayClass>(OVERLAY_SANDBAG_WALL, cell,
                                            House->Class->House);
              break;

            case STRUCT_WOOD_WALL:
              base::NewPooled<OverlayClass>(OVERLAY_WOOD_WALL, cell,
                                            House->Class->House);
              break;

            case STRUCT_CYCLONE_WALL:
              base::NewPooled<OverlayClass>(OVERLAY_CYCLONE_WALL, cell,
                                            House->Class->House);
              break;

            case STRUCT_FENCE:
              base::NewPooled<OverlayClass>(OVERLAY_FENCE, cell,
                                            House->Class->House);
              break;

            case StructType::STRUCT_NONE:
            case StructType::STRUCT_ADVANCED_TECH:
            case StructType::STRUCT_IRON_CURTAIN:
            case StructType::STRUCT_WEAP:
            case StructType::STRUCT_CHRONOSPHERE:
            case StructType::STRUCT_PILLBOX:
            case StructType::STRUCT_CAMOPILLBOX:
            case StructType::STRUCT_RADAR:
            case StructType::STRUCT_GAP:
            case StructType::STRUCT_TURRET:
            case StructType::STRUCT_AAGUN:
            case StructType::STRUCT_FLAME_TURRET:
            case StructType::STRUCT_CONST:
            case StructType::STRUCT_REFINERY:
            case StructType::STRUCT_STORAGE:
            case StructType::STRUCT_HELIPAD:
            case StructType::STRUCT_SAM:
            case StructType::STRUCT_AIRSTRIP:
            case StructType::STRUCT_POWER:
            case StructType::STRUCT_ADVANCED_POWER:
            case StructType::STRUCT_SOVIET_TECH:
            case StructType::STRUCT_HOSPITAL:
            case StructType::STRUCT_BARRACKS:
            case StructType::STRUCT_TENT:
            case StructType::STRUCT_KENNEL:
            case StructType::STRUCT_REPAIR:
            case StructType::STRUCT_BIO_LAB:
            case StructType::STRUCT_MISSION:
            case StructType::STRUCT_SHIP_YARD:
            case StructType::STRUCT_SUB_PEN:
            case StructType::STRUCT_MSLO:
            case StructType::STRUCT_FORWARD_COM:
            case StructType::STRUCT_TESLA:
            case StructType::STRUCT_FAKEWEAP:
            case StructType::STRUCT_FAKECONST:
            case StructType::STRUCT_FAKE_YARD:
            case StructType::STRUCT_FAKE_PEN:
            case StructType::STRUCT_FAKE_RADAR:
            case StructType::STRUCT_AVMINE:
            case StructType::STRUCT_APMINE:
            case StructType::STRUCT_V01:
            case StructType::STRUCT_V02:
            case StructType::STRUCT_V03:
            case StructType::STRUCT_V04:
            case StructType::STRUCT_V05:
            case StructType::STRUCT_V06:
            case StructType::STRUCT_V07:
            case StructType::STRUCT_V08:
            case StructType::STRUCT_V09:
            case StructType::STRUCT_V10:
            case StructType::STRUCT_V11:
            case StructType::STRUCT_V12:
            case StructType::STRUCT_V13:
            case StructType::STRUCT_V14:
            case StructType::STRUCT_V15:
            case StructType::STRUCT_V16:
            case StructType::STRUCT_V17:
            case StructType::STRUCT_V18:
            case StructType::STRUCT_PUMP:
            case StructType::STRUCT_V20:
            case StructType::STRUCT_V21:
            case StructType::STRUCT_V22:
            case StructType::STRUCT_V23:
            case StructType::STRUCT_V24:
            case StructType::STRUCT_V25:
            case StructType::STRUCT_V26:
            case StructType::STRUCT_V27:
            case StructType::STRUCT_V28:
            case StructType::STRUCT_V29:
            case StructType::STRUCT_V30:
            case StructType::STRUCT_V31:
            case StructType::STRUCT_V32:
            case StructType::STRUCT_V33:
            case StructType::STRUCT_V34:
            case StructType::STRUCT_V35:
            case StructType::STRUCT_V36:
            case StructType::STRUCT_V37:
            case StructType::STRUCT_BARREL:
            case StructType::STRUCT_BARREL3:
            case StructType::STRUCT_QUEEN:
            case StructType::STRUCT_LARVA1:
            case StructType::STRUCT_LARVA2:
            default:
              break;
          }
          Transmit_Message(RADIO_OVER_OUT);
          delete this;

        } else {
          if (Can_Enter_Cell(cell) == game::MOVE_OK) {
            /*
            **	Determine if a bib is required for this building. If one is,
            *then *	create and place it.
            */
            Cell newcell = cell;
            if (Class->Bib_And_Offset(bib, newcell)) {
              base::NewPooled<SmudgeClass>(
                  bib, newcell.Center(),
                  Class->IsBase ? House->Class->House : HOUSE_NONE);
            }

            TheMap().Place_Down(cell, this);
          } else {
            return false;
          }
        }
        break;

      case MARK_CHANGE_REDRAW:
        TheMap().Refresh_Cells(cell, Overlap_List(true));
        break;

      case MarkType::MARK_CHANGE:
      case MarkType::MARK_OVERLAP_DOWN:
      case MarkType::MARK_OVERLAP_UP:
      default:
        TheMap().Refresh_Cells(cell, Overlap_List(false));
        TheMap().Refresh_Cells(cell, occupy);
        break;
    }
    return true;
  }
  return false;
}

/***********************************************************************************************
 * BuildingClass::AI -- Handles non-graphic AI processing for buildings. *
 *                                                                                             *
 *    This function is to handle the AI logic for the building. The graphic
 *logic (facing,     * firing, and animation) is handled elsewhere. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/31/1994 JLB : Created. * 12/26/1994 JLB : Handles production. *
 *   06/11/1995 JLB : Revamped. *
 *=============================================================================================*/
void BuildingClass::AI() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  /*
  **	Process building animation state changes. Transition to a following
  *state *	if there is one specified and the current animation sequence has
  *expired. *	This process must occur before mission AI since the mission AI
  *relies on *	the bstate change to occur immediately before the
  *MissionClass::AI.
  */
  Animation_AI();

  /*
  **	If now is a good time to act on a new mission, then do so. This process
  *occurs *	here because some outside event may have requested a mission
  *change for the building. *	Such outside requests (player input) must be
  *initiated BEFORE the normal AI process.
  */
  /*
  **	Clear the commencement flag ONLY if something actually occurred. By
  *acting *	this way, a building can set the IsReadyToCommence flag before
  *it goes *	to "sleep" knowing that it will wake up as soon as a new mission
  *comes *	along.
  */
  if ((IsReadyToCommence && BState != game::BSTATE_CONSTRUCTION) &&
      Commence()) {
    IsReadyToCommence = false;
  }

  /*
  **	Proceed with normal logic processing. This is where the mission
  *processing *	occurs. This call must be located after the animation sequence
  *makes the *	transition to the next frame (see above) in order for the
  *mission logic to *	act at the exact moment of graphic transition BEFORE it
  *has a chance to *	be displayed.
  */
  TechnoClass::AI();

  /*
  **	Bail if the object died in the AI routine.
  */
  if (!IsActive) {
    return;
  }

  /*
  **	Building ammo is instantly reloaded.
  */
  if (!Ammo) {
    Ammo = Class->MaxAmmo;
  }

  /*
  **	If now is a good time to act on a new mission, then do so. This occurs
  *here because *	some AI event may have requested a mission change
  *(usually from another mission *	state machine). This must occur here
  *before it has a chance to render.
  */
  /*
  **	Clear the commencement flag ONLY if something actually occurred. By
  *acting *	this way, a building can set the IsReadyToCommence flag before
  *it goes *	to "sleep" knowing that it will wake up as soon as a new mission
  *comes *	along.
  */
  if (IsReadyToCommence && Commence()) {
    IsReadyToCommence = false;
  }

  /*
  **	If a change of animation was requested, then make the change
  **	now. The building animation system acts independently but subordinate
  **	to the mission state machine system. By performing the animation
  *change-up *	here, the mission AI system is ensured of immediate visual
  *affect when it *	decides to change the animation state of the building.
  */
  if (QueueBState != game::BSTATE_NONE) {
    if (BState != QueueBState) {
      BState = QueueBState;
      const BuildingTypeClass::AnimControlType* ctrl = Fetch_Anim_Control();
      if (BState == game::BSTATE_CONSTRUCTION || BState == game::BSTATE_IDLE) {
        animation().Set_Rate(TheOptions().Normalize_Delay(ctrl->Rate));
      } else {
        animation().Set_Rate(ctrl->Rate);
      }
      animation().Set_Stage(ctrl->Start);
    }
    QueueBState = game::BSTATE_NONE;
  }

  /*
  **	If the building's strength has changed, then update the power
  **	accordingly.
  */
  if (Strength != LastStrength) {
    const int oldpower = Power_Output();
    LastStrength = Strength;
    const int newpower = Power_Output();
    House->Adjust_Power(newpower - oldpower);
  }

  /*
  **	Check to see if the destruction countdown timer is active. If so, then
  *decrement it. *	When this timer reaches zero, the building is removed
  *from the map. All the explosions *	are presumed to be in progress at this
  *time.
  */
  if (Strength == 0) {
    if (CountDown.IsFinished()) {
      Limbo();
      Drop_Debris(WhomToRepay);
      delete this;
    }
    return;
  }

  /*
  **	Charging logic.
  */
  Charging_AI();

  /*
  **	Handle any repair process that may be going on.
  */
  Repair_AI();

  /*
  **	For computer controlled buildings, determine what should be produced and
  *start *	production accordingly.
  */
  Factory_AI();

  /*
  **	Check for demolition timeout. When timeout has expired, the building
  *explodes.
  */
  if (IsGoingToBlow && CountDown.IsFinished()) {
    int damage = Strength;
    Take_Damage(damage, 0, WARHEAD_FIRE, WhomToRepay.AsTechno(), true);
    if (!IsActive) {
      return;
    }
    Mark(MARK_CHANGE);
  }

  /*
  **	Turret equiped buildings must handle turret rotation logic here. This
  *entails *	rotating the turret to the desired facing as well as figuring
  *out what that *	desired facing should be.
  */
  Rotation_AI();

  /*
  ** Gap Generators need to scan if they've just become activated, or if
  ** the power has just come on enough so they can scan.  Also, they need
  ** to un-jam if the power has just dropped off.
  */
  if (*this == STRUCT_GAP) {
    if (Arm.IsFinished()) {
      IsJamming = false;
      Arm.Set(kTicksPerMinute * TheRules().GapRegenInterval +
              RandomPick(1, kTicksPerSecond));
    }

    if (!IsJamming) {
      if (House->Power_Fraction() >= 1) {
        MapEditClass::Jam_From(Cell::Containing(Center_Coord()),
                               TheRules().GapShroudRadius, House);
        IsJamming = true;
      }
    } else {
      if (House->Power_Fraction() < 1) {
        IsJamming = false;
        MapEditClass::UnJam_From(Cell::Containing(Center_Coord()),
                                 TheRules().GapShroudRadius, House);
      }
    }
  }

  /*
  ** Radar facilities and SAMs need to check for the proximity of a mobile
  ** radar jammer.
  */
  if ((*this == STRUCT_RADAR || *this == STRUCT_SAM) &&
      game::CurrentFrame() % kTicksPerSecond == 0) {
    IsJammed = false;
    for (int index = 0; index < TheObjectHeaps().unit().Count(); index++) {
      const UnitClass* obj = TheObjectHeaps().unit().ActiveAt(index);
      if (obj != nullptr && !obj->IsInLimbo && !obj->House->Is_Ally(House) &&
          obj->Class->IsJammer && Distance(obj) <= TheRules().RadarJamRadius) {
        IsJammed = true;
        break;
      }
    }
  }
}

/***********************************************************************************************
 * BuildingClass::Unlimbo -- Removes a building from limbo state. *
 *                                                                                             *
 *    Use this routine to transform a building that has been held in limbo *
 *    state, into one that really exists on the map. Once a building as * been
 *unlimboed, then it becomes a normal object in the game world. *
 *                                                                                             *
 * INPUT:   pos   -- The position to place the building on the map. *
 *                                                                                             *
 *          dir (optional) -- not used for this class *
 *                                                                                             *
 * OUTPUT:  bool; Was the unlimbo successful? *
 *                                                                                             *
 * WARNINGS:   The unlimbo operation might not be successful if the * building
 *could not be placed at the location specified.                         *
 *                                                                                             *
 * HISTORY: * 04/16/1994 JLB : Created. * 06/07/1994 JLB : Matches virtual
 *function format for base class.                          * 05/09/1995 JLB :
 *Handles wall placement.                                                  *
 *   06/18/1995 JLB : Checks for wall legality before placing down. *
 *=============================================================================================*/
bool BuildingClass::Unlimbo(Coordinate coord, game::Direction dir) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  /*
  **	If this is a wall type building, then it never gets unlimboed. Instead,
  *it gets *	converted to an overlay type.
  */
  if (Class->IsWall) {
    if (Can_Enter_Cell(Cell::Containing(coord), game::Facing::kNone) ==
        game::MOVE_OK) {
      OverlayType otype = OVERLAY_NONE;
      switch (Class->Type) {
        case STRUCT_SANDBAG_WALL:
          otype = OVERLAY_SANDBAG_WALL;
          break;

        case STRUCT_CYCLONE_WALL:
          otype = OVERLAY_CYCLONE_WALL;
          break;

        case STRUCT_BRICK_WALL:
          otype = OVERLAY_BRICK_WALL;
          break;

        case STRUCT_BARBWIRE_WALL:
          otype = OVERLAY_BARBWIRE_WALL;
          break;

        case STRUCT_WOOD_WALL:
          otype = OVERLAY_WOOD_WALL;
          break;

        case STRUCT_FENCE:
          otype = OVERLAY_FENCE;
          break;

        case StructType::STRUCT_NONE:
        case StructType::STRUCT_ADVANCED_TECH:
        case StructType::STRUCT_IRON_CURTAIN:
        case StructType::STRUCT_WEAP:
        case StructType::STRUCT_CHRONOSPHERE:
        case StructType::STRUCT_PILLBOX:
        case StructType::STRUCT_CAMOPILLBOX:
        case StructType::STRUCT_RADAR:
        case StructType::STRUCT_GAP:
        case StructType::STRUCT_TURRET:
        case StructType::STRUCT_AAGUN:
        case StructType::STRUCT_FLAME_TURRET:
        case StructType::STRUCT_CONST:
        case StructType::STRUCT_REFINERY:
        case StructType::STRUCT_STORAGE:
        case StructType::STRUCT_HELIPAD:
        case StructType::STRUCT_SAM:
        case StructType::STRUCT_AIRSTRIP:
        case StructType::STRUCT_POWER:
        case StructType::STRUCT_ADVANCED_POWER:
        case StructType::STRUCT_SOVIET_TECH:
        case StructType::STRUCT_HOSPITAL:
        case StructType::STRUCT_BARRACKS:
        case StructType::STRUCT_TENT:
        case StructType::STRUCT_KENNEL:
        case StructType::STRUCT_REPAIR:
        case StructType::STRUCT_BIO_LAB:
        case StructType::STRUCT_MISSION:
        case StructType::STRUCT_SHIP_YARD:
        case StructType::STRUCT_SUB_PEN:
        case StructType::STRUCT_MSLO:
        case StructType::STRUCT_FORWARD_COM:
        case StructType::STRUCT_TESLA:
        case StructType::STRUCT_FAKEWEAP:
        case StructType::STRUCT_FAKECONST:
        case StructType::STRUCT_FAKE_YARD:
        case StructType::STRUCT_FAKE_PEN:
        case StructType::STRUCT_FAKE_RADAR:
        case StructType::STRUCT_AVMINE:
        case StructType::STRUCT_APMINE:
        case StructType::STRUCT_V01:
        case StructType::STRUCT_V02:
        case StructType::STRUCT_V03:
        case StructType::STRUCT_V04:
        case StructType::STRUCT_V05:
        case StructType::STRUCT_V06:
        case StructType::STRUCT_V07:
        case StructType::STRUCT_V08:
        case StructType::STRUCT_V09:
        case StructType::STRUCT_V10:
        case StructType::STRUCT_V11:
        case StructType::STRUCT_V12:
        case StructType::STRUCT_V13:
        case StructType::STRUCT_V14:
        case StructType::STRUCT_V15:
        case StructType::STRUCT_V16:
        case StructType::STRUCT_V17:
        case StructType::STRUCT_V18:
        case StructType::STRUCT_PUMP:
        case StructType::STRUCT_V20:
        case StructType::STRUCT_V21:
        case StructType::STRUCT_V22:
        case StructType::STRUCT_V23:
        case StructType::STRUCT_V24:
        case StructType::STRUCT_V25:
        case StructType::STRUCT_V26:
        case StructType::STRUCT_V27:
        case StructType::STRUCT_V28:
        case StructType::STRUCT_V29:
        case StructType::STRUCT_V30:
        case StructType::STRUCT_V31:
        case StructType::STRUCT_V32:
        case StructType::STRUCT_V33:
        case StructType::STRUCT_V34:
        case StructType::STRUCT_V35:
        case StructType::STRUCT_V36:
        case StructType::STRUCT_V37:
        case StructType::STRUCT_BARREL:
        case StructType::STRUCT_BARREL3:
        case StructType::STRUCT_QUEEN:
        case StructType::STRUCT_LARVA1:
        case StructType::STRUCT_LARVA2:
        default:
          otype = OVERLAY_NONE;
          break;
      }
      if (otype != OVERLAY_NONE) {
        ObjectClass* o =
            OverlayTypeClass::As_Reference(otype).Create_One_Of(House);
        if (o && o->Unlimbo(coord)) {
          TheMap().at(coord).Owner = House->Class->House;
          Transmit_Message(RADIO_OVER_OUT);
          TheMap().Sight_From(Cell::Containing(coord), Class->SightRange,
                              House);
          delete this;
          return true;
        }
      }
    }
    return false;
  }

  /*
  **	Normal building unlimbo process.
  */
  if (TechnoClass::Unlimbo(coord, dir)) {
    /*
    **	Ensure that the owning house knows about the
    **	new object.
    */
    House->BScan |= ScanBit(static_cast<int>(Class->Type));
    House->ActiveBScan |= ScanBit(static_cast<int>(Class->Type));

    /*
    **	Recalculate the center point of the house's base.
    */
    House->Recalc_Center();

    /*
    **	Update the total factory type, assuming this building has a factory.
    */
    House->Active_Add(this);

    /*
    **	Possibly the sidebar will be affected by this addition.
    */
    House->IsRecalcNeeded = true;
    LastStrength = 0;

    if ((!IsDiscoveredByPlayer && TheMap().at(coord).IsVisible) ||
        TheSession().Type != GAME_NORMAL) {
      Revealed(ThePlayer());
    }
    if (!House->IsHuman) {
      Revealed(House);
    }

    if (IsOwnedByPlayer) {
      TheMap().PowerClass::IsPowerToRedraw = true;
      TheMap().Flag_To_Redraw(false);
    }

    if ((Class->Ownable & (kHouseFlagGood | kHouseFlagBad)) !=
        (kHouseFlagGood | kHouseFlagBad)) {
      if (Class->Ownable & kHouseFlagGood) {
        ActLike = HOUSE_GREECE;
      } else {
        ActLike = HOUSE_USSR;
      }
    }

    return true;
  }
  return false;
}

/***********************************************************************************************
 * BuildingClass::Take_Damage -- Inflicts damage points upon a building. *
 *                                                                                             *
 *    This routine will inflict damage points upon the specified building. * It
 *will handle the damage animation and building destruction. Use * this routine
 *whenever a building is attacked.                                            *
 *                                                                                             *
 * INPUT:   damage   -- Amount of damage to inflict. *
 *                                                                                             *
 *          distance -- The distance from the damage center point to the
 *object's center point.*
 *                                                                                             *
 *          warhead  -- The kind of damage to inflict. *
 *                                                                                             *
 *          source   -- The source of the damage. This is used to change
 *targeting.            *
 *                                                                                             *
 *          forced   -- Is the damage forced upon the object regardless of
 *whether it          * is normally immune? *
 *                                                                                             *
 * OUTPUT:  true/false; Was the building destroyed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/21/1991     : Created. * 04/15/1994 JLB : Converted to member
 *function.                                            * 04/16/1994 JLB : Added
 *warhead modifier to damage.                                        *
 *   06/03/1994 JLB : Added source of damage as target value. * 06/20/1994 JLB :
 *Source is a base class pointer.                                          *
 *   11/22/1994 JLB : Shares base damage handler for techno objects. *
 *   07/15/1995 JLB : Power ratio gets adjusted. *
 *=============================================================================================*/
game::ResultType BuildingClass::Take_Damage(int& damage, int distance,
                                            WarheadType warhead,
                                            TechnoClass* source, bool forced) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  game::ResultType res = game::RESULT_NONE;
  int shakes = 0;

  if (this != source /*&& !Class->IsInsignificant*/) {
    if (source) {
      House->LATime = static_cast<int>(game::CurrentFrame());
      House->LAType = source->What_Am_I();
      House->LAZone = House->Which_Zone(this);
      House->LAEnemy = source->Owner();

      if (!House->Is_Ally(source)) {
        House->Enemy = source->Owner();
      }

      Base_Is_Attacked(source);
    }

    std::span<const int16_t> offset = Occupy_List();

    /*
    ** Memorize who they used to be in radio contact with.
    */
    TechnoClass* tech = Contact_With_Whom();
    /*
    **	Perform the low level damage assessment.
    */
    res = TechnoClass::Take_Damage(damage, distance, warhead, source, forced);
    switch (res) {
      case game::RESULT_DESTROYED:

        /*
        **	Add the building to the base prebuild list if allowed. This will
        *force *	the computer to rebuild this structure if it can.
        */
        if (IsToRebuild && Class->Level != -1 &&
            TheWorld().base().house() == House->Class->House &&
            !TheWorld().base().HasNodeFor(*this)) {
          //				if (IsToRebuild && Class->IsBuildable &&
          // Base.House == House->Class->House && Base.Get_Node(this) == 0) {
          TheWorld().base().AddNode(Class->Type, Cell::Containing(Coord));
        }

        /*
        **	Destroy all attached objects.
        */
        while (cargo().Attached_Object()) {
          const FootClass* obj = cargo().Detach_Object();

          Detach_All(true);
          base::DeletePooled(obj);
        }

        /*
        ** If we were in contact with a landed plane, blow the plane up too.
        */
        if (tech && tech->IsActive && tech->What_Am_I() == RTTI_AIRCRAFT &&
            dynamic_cast<AircraftClass*>(tech)->Class->IsFixedWing &&
            dynamic_cast<AircraftClass*>(tech)->In_Which_Layer() ==
                LAYER_GROUND) {
          int occupant_damage = 500;
          tech->Take_Damage(occupant_damage, 0, WARHEAD_AP, source, forced);
        }

        PlaySoundEffectAt(VOC_KABOOM22, Coord);
        while (offset.front() != kRefreshEol) {
          const Cell cell =
              Cell::Containing(Coord).Offset(base::ConsumeFront(offset));

          /*
          **	If the building is destroyed, then lots of
          **	explosions occur.
          */
          base::NewPooled<SmudgeClass>(
              RandomPick(SMUDGE_CRATER1, SMUDGE_CRATER6), cell.Center());
          if (PercentChance(50)) {
            base::NewPooled<AnimClass>(
                ANIM_FIRE_SMALL, Coord_Scatter(cell.Center(), 0x0080),
                static_cast<unsigned char>(RandomPick(0, 7)),
                static_cast<unsigned char>(RandomPick(1, 3)));
            if (PercentChance(50)) {
              base::NewPooled<AnimClass>(
                  ANIM_FIRE_MED, Coord_Scatter(cell.Center(), 0x0040),
                  static_cast<unsigned char>(RandomPick(0, 7)),
                  static_cast<unsigned char>(RandomPick(1, 3)));
            }
          }
          base::NewPooled<AnimClass>(
              ANIM_FBALL1, Coord_Scatter(cell.Center(), 0x0040),
              static_cast<unsigned char>(RandomPick(0, 3)));
        }

        shakes = Class->Cost_Of() / 400;
        if (shakes) {
          Shake_The_Screen(shakes);
        }
        PlaySoundEffectAt(VOC_CRUMBLE, Coord);
        if (mission_state().current() == MISSION_DECONSTRUCTION) {
          CountDown.Set(0);
          animation().Set_Rate(0);
        } else {
          CountDown.Set(8);
        }

        /*
        **	If it is in radio contact and the object seems to be attached,
        *then tell *	it to run away.
        */
        if (radio().InContact() &&
            Transmit_Message(RADIO_NEED_TO_MOVE) == RADIO_ROGER) {
          Transmit_Message(RADIO_RUN_AWAY);
        }

        /*
        **	A force destruction will not generate survivors.
        */
        if (forced || *this == STRUCT_KENNEL) {
          IsSurvivorless = true;
        }

        /*
        ** Destruction of a radar facility or advanced communications
        ** center will cause the spiedby field to change...
        */
        if (SpiedBy) {
          SpiedBy = 0;
          const StructType struc = *this;
          if (struc == STRUCT_RADAR /* || struc == STRUCT_EYE */) {
            Update_Radar_Spied();
          }
        }

        /*
        ** Destruction of a gap generator will cause the cells it affects
        ** to stop being jammed.
        */
        if (*this == STRUCT_GAP) {
          Remove_Gap_Effect();
        }

        /*
        ** Destruction of a shipyard or sub pen may cause attached ships
        ** who are repairing themselves to discontinue repairs.
        */
        if (*this == STRUCT_SHIP_YARD || *this == STRUCT_SUB_PEN) {
          for (int index = 0; index < TheObjectHeaps().vessel().Count();
               index++) {
            VesselClass* obj = TheObjectHeaps().vessel().ActiveAt(index);
            if ((obj && !obj->IsInLimbo && obj->House == House) &&
                obj->IsSelfRepairing &&
                (ra::Distance(Center_Coord(), obj->Center_Coord()) < 0x0200)) {
              obj->IsSelfRepairing = false;
              obj->IsToSelfRepair = false;
            }
          }
        }

        /*
        ** Destruction of a barrel will cause the surrounding squares to
        ** be hit with damage.
        */
        if (*this == STRUCT_BARREL || *this == STRUCT_BARREL3) {
          const Coordinate center = Center_Coord();
          const Cell cellcenter = Cell::Containing(center);

          auto* bullet = base::NewPooled<BulletClass>(
              BULLET_INVISIBLE,
              Target::FromCell(AdjacentCell(cellcenter, game::Facing::kNorth)),
              nullptr, 200, WARHEAD_FIRE, game::kMphMediumFast);
          if (bullet) {
            bullet->Unlimbo(center, game::Direction::kNorth);
          }

          bullet = base::NewPooled<BulletClass>(
              BULLET_INVISIBLE,
              Target::FromCell(AdjacentCell(cellcenter, game::Facing::kEast)),
              nullptr, 200, WARHEAD_FIRE, game::kMphMediumFast);
          if (bullet) {
            bullet->Unlimbo(center, game::Direction::kEast);
          }

          bullet = base::NewPooled<BulletClass>(
              BULLET_INVISIBLE,
              Target::FromCell(AdjacentCell(cellcenter, game::Facing::kSouth)),
              nullptr, 200, WARHEAD_FIRE, game::kMphMediumFast);
          if (bullet) {
            bullet->Unlimbo(center, game::Direction::kSouth);
          }

          bullet = base::NewPooled<BulletClass>(
              BULLET_INVISIBLE,
              Target::FromCell(AdjacentCell(cellcenter, game::Facing::kWest)),
              nullptr, 200, WARHEAD_FIRE, game::kMphMediumFast);
          if (bullet) {
            bullet->Unlimbo(center, game::Direction::kWest);
          }
        }
        break;

      case game::RESULT_HALF:
        if (*this == STRUCT_PUMP) {
          auto* anim = base::NewPooled<AnimClass>(
              ANIM_OILFIELD_BURN,
              Coord + Coordinate::FromLeptons(0x0130, 0x0040), 1);
          if (anim) {
            anim->Attach_To(this);
          }
        }
        [[fallthrough]];

      case game::RESULT_MAJOR:
        PlaySoundEffectAt(VOC_KABOOM1, Coord);
        while (offset.front() != kRefreshEol) {
          const Cell cell =
              Cell::Containing(Coord).Offset(base::ConsumeFront(offset));
          AnimClass* anim = nullptr;

          /*
          **	Show pieces of fire to indicate that a significant change in
          **	damage level has occurred.
          */
          if (warhead == WARHEAD_FIRE) {
            switch (RandomPick(0, 5 + Class->Width() + Class->Height())) {
              case 0:
                break;

              case 1:
              case 2:
              case 3:
              case 4:
              case 5:
                anim = base::NewPooled<AnimClass>(
                    ANIM_ON_FIRE_SMALL, Coord_Scatter(cell.Center(), 0x0060), 0,
                    static_cast<unsigned char>(RandomPick(1, 3)));
                break;

              case 6:
              case 7:
              case 8:
                anim = base::NewPooled<AnimClass>(
                    ANIM_ON_FIRE_MED, Coord_Scatter(cell.Center(), 0x0060), 0,
                    static_cast<unsigned char>(RandomPick(1, 3)));
                break;

              case 9:
                anim = base::NewPooled<AnimClass>(
                    ANIM_ON_FIRE_BIG, Coord_Scatter(cell.Center(), 0x0060), 0,
                    1);
                break;

              default:
                break;
            }
          } else {
            if (PercentChance(50)) {
              /*
              ** Building may catch on fire, but only if it wasn't a
              ** renovator that caused the damage.
              */
              if (source == nullptr || source->What_Am_I() != RTTI_INFANTRY ||
                  *dynamic_cast<InfantryClass*>(source) != INFANTRY_RENOVATOR) {
                anim = base::NewPooled<AnimClass>(
                    ANIM_FIRE_SMALL, Coord_Scatter(cell.Center(), 0x0060),
                    static_cast<unsigned char>(RandomPick(0, 7)),
                    static_cast<unsigned char>(RandomPick(1, 3)));
              }
            }
          }
          /*
          **	If the animation was created, then attach it to the building.
          */
          if (anim) {
            anim->Attach_To(this);
          }
        }
        break;

      case game::RESULT_NONE:
      case game::RESULT_LIGHT:
      default:
        break;
    }

    if (source && res != game::RESULT_NONE) {
      /*
      **	If any damage occurred, then inform the house of this fact. If
      *it is the player's *	house, it might announce this fact.
      */
      if (!Class->IsInsignificant) {
        House->Attacked();
      }

      /*
      ** Save the type of the house that's doing the damage, so if the building
      *burns
      ** to death credit can still be given for the kill
      */
      WhoLastHurtMe = source->Owner();

      /*
      **	When certain buildings are hit, they "snap out of it" and
      **	return fire if they are able and allowed.
      */
      if (*this != STRUCT_SAM && *this != STRUCT_AAGUN &&
          !House->Is_Ally(source) && Class->PrimaryWeapon != nullptr &&
          (!TarCom || !In_Range(TarCom))) {
        if (source->What_Am_I() != RTTI_AIRCRAFT &&
            (!House->IsHuman || TheRules().IsSmartDefense)) {
          Assign_Target(source->As_Target());
        } else {
          /*
          **	Generate a random rotation effect since there is nothing else
          *that this *	building can do.
          */
          if (!PrimaryFacing.IsRotating()) {
            PrimaryFacing.set_desired(
                RandomPick(game::Direction::kNorth, kMaxRandomDirection));
          }
        }
      }
    }
  }

  return res;
}

/***********************************************************************************************
 * BuildingClass::new -- Allocates a building object from building pool. *
 *                                                                                             *
 *    This routine will allocate a building slot from the building alloc *
 *    system. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the allocated building. If nullptr is *
 *          returned, then this indicates a failure to allocate. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/11/1994 JLB : Created. * 04/21/1994 JLB : Converted to operator
 *new.                                               * 05/17/1994 JLB : Revamped
 *allocation scheme                                               * 07/29/1994
 *JLB : Simplified. *
 *=============================================================================================*/
void* BuildingClass::operator new(std::size_t /*unused*/) noexcept {
  void* ptr = TheObjectHeaps().building().Allocate();
  if (ptr) {
    static_cast<BuildingClass*>(ptr)->IsActive = true;
  }
  return ptr;
}

/***********************************************************************************************
 * BuildingClass::delete -- Deallocates building object. *
 *                                                                                             *
 *    This is the memory deallocation operation for a building object. * Since
 *buildings are allocated out of a fixed memory block, all that * is needed is
 *to flag the unit as inactive.                                               *
 *                                                                                             *
 * INPUT:   ptr   -- Pointer to building to deallocate. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/21/1994 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::operator delete(void* ptr) {
  if (ptr) {
    static_cast<BuildingClass*>(ptr)->IsActive = false;
  }
  TheObjectHeaps().building().Free(static_cast<BuildingClass*>(ptr));
}

/***********************************************************************************************
 * BuildingClass::BuildingClass -- Constructor for buildings. *
 *                                                                                             *
 *    This routine inserts a building into the object tracking system. * It is
 *placed into a limbo state unless a location is provided for * it to unlimbo
 *at.                                                                        *
 *                                                                                             *
 * INPUT:   type  -- The structure type to make this object. *
 *                                                                                             *
 *          house -- The owner of this building. *
 *                                                                                             *
 *          pos   -- The position to unlimbo the building. If -1 is * specified,
 *then the building remains in a limbo                           * state. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/21/1994 JLB : Created. * 08/07/1995 JLB : Fixed act like value
 *to match expected value.                            *
 *=============================================================================================*/
BuildingClass::BuildingClass(StructType type, HousesType house)
    : TechnoClass(RTTI_BUILDING, TheObjectHeaps().building().SlotIndexOf(this),
                  house),
      Class(TheTypeHeaps().building().ActiveAt(static_cast<int>(type))),
      ActLike(House->ActLike),
      WhoLastHurtMe(house) {
  House->Tracking_Add(this);
  IsSecondShot = !Class->Is_Two_Shooter();
  Strength = Class->MaxStrength;
  Ammo = Class->MaxAmmo;

  /*
  **	If the building could never be built, then it can never be sold either.
  *This *	is due to the lack of buildup animation.
  */
  if (!Class->Get_Buildup_Data().empty()) {
    //	if (!Class->IsBuildable) {
    IsAllowedToSell = false;
  }

  //	if (Session.Type == GAME_INTERNET) {
  //		House->BuildingTotals->Increment_Unit_Total( (int) type);
  //	}
}

/***********************************************************************************************
 * BuildingClass::~BuildingClass -- Destructor for building type objects. *
 *                                                                                             *
 *    This destructor for building objects will put the building in limbo if
 *possible.         *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/18/1995 JLB : Created. *
 *=============================================================================================*/
BuildingClass::~BuildingClass() {
  if (TheGameState().active() && Class) {
    House->Tracking_Remove(this);
    BuildingClass::Limbo();
  }
  Class = nullptr;

  delete Factory;
  Factory = nullptr;
  ID = -1;
}

/***********************************************************************************************
 * BuildingClass::Drop_Debris -- Drops rubble when building is destroyed. *
 *                                                                                             *
 *    This routine is called when a building is destroyed. It handles * placing
 *the rubble down. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/14/1994 JLB : Created. * 06/13/1995 JLB : Added smoke and
 *normal infantry survivor possibility.                    * 07/16/1995 JLB :
 *Survival rate depends on if captured or sabotaged.                       *
 *=============================================================================================*/
void BuildingClass::Drop_Debris(Target source) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  /*
  **	Generate random survivors from the destroyed building.
  */
  const Cell cell = Cell::Containing(Coord);
  std::span<const int16_t> offset = Occupy_List();
  int odds = 2;
  if (WhomToRepay) {
    odds -= 1;
  }
  if (IsCaptured) {
    odds += 6;
  }
  int count = How_Many_Survivors();
  while (offset.front() != kRefreshEol) {
    const Cell newcell = cell.Offset(base::ConsumeFront(offset));
    const CellClass* cellptr = &TheMap().at(newcell);

    /*
    **	Infantry could run out of a destroyed building.
    */
    if (!House->IsToDie && count > 0) {
      InfantryClass* i = nullptr;

      if (RandomPick(0, odds) == 1) {
        i = nullptr;
        const InfantryType typ = Crew_Type();
        if (typ != INFANTRY_NONE) {
          i = base::NewPooled<InfantryClass>(typ, House->Class->House);
        }
        if (i != nullptr) {
          if (!Class->Get_Buildup_Data().empty() && i->Class->IsNominal) {
            i->IsTechnician = true;
          }
          TheWorld().scenario_init()++;
          if (i->Unlimbo(newcell.Center(), game::Direction::kNorth)) {
            count--;
            i->Strength = static_cast<int16_t>(
                RandomPick(5, static_cast<int>(i->Class->MaxStrength)));
            i->Scatter(Coordinate{}, true);
            if (source != Target{} && !House->Is_Ally(source.AsObject())) {
              i->Assign_Mission(MISSION_ATTACK);
              i->Assign_Target(source);
            } else {
              if (House->IsHuman) {
                i->Assign_Mission(MISSION_GUARD);
              } else {
                i->Assign_Mission(MISSION_HUNT);
              }
            }
          } else {
            base::DeletePooled(i);
          }
          TheWorld().scenario_init()--;
        }
      }
    }

    /*
    **	Smoke and fire only appear on terrestrail cells. They should not appear
    *on *	rivers, clifs, or water cells.
    */
    if (cellptr->Is_Clear_To_Move(SPEED_TRACK, true, true)) {
      /*
      **	Possibly add some smoke rising from the ashes of the building.
      */
      switch (RandomPick(0, 5)) {
        case 0:
        case 1:
        case 2:
          base::NewPooled<AnimClass>(
              ANIM_SMOKE_M, Coord_Scatter(newcell.Center(), 0x0050, false),
              static_cast<unsigned char>(RandomPick(0, 5)),
              static_cast<unsigned char>(RandomPick(1, 2)));
          break;

        default:
          break;
      }

      /*
      **	The building always scars the ground in some fashion.
      */
      if (PercentChance(25)) {
        base::NewPooled<SmudgeClass>(RandomPick(SMUDGE_SCORCH1, SMUDGE_SCORCH6),
                                     newcell.Center());
      } else {
        base::NewPooled<SmudgeClass>(
            RandomPick(SMUDGE_CRATER1, SMUDGE_CRATER6),
            Coord_Scatter(newcell.Center(), 0x0080, false));
      }
    }
  }
}

/***********************************************************************************************
 * BuildingClass::Active_Click_With -- Handles clicking on the map while the
 *building is selected.*
 *                                                                                             *
 *    This interface routine handles when the player clicks on the map while
 *this building     * is currently selected. This is used to assign an override
 *target to a turret or          * guard tower. *
 *                                                                                             *
 * INPUT:   target   -- The target that was clicked upon. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/28/1994 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Active_Click_With(ActionType action, ObjectClass* object) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (action == ACTION_ATTACK && object != nullptr) {
    Player_Assign_Mission(MISSION_ATTACK, object->As_Target());
  }

  if (action == ACTION_SELF && Class->Is_Factory()) {
    TheNetwork().out_list().Add(
        EventClass(EventClass::PRIMARY, Target::Of(this)));
  }
}

/***********************************************************************************************
 * BuildingClass::Active_Click_With -- Handles cell selection for buildings. *
 *                                                                                             *
 *    This routine really only serves one purpose -- to allow targeting of the
 *ground for      * buildings that are equipped with weapons. *
 *                                                                                             *
 * INPUT:   action   -- The requested action to perform. *
 *                                                                                             *
 *          cell     -- The cell location to perform the action upon. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/04/1995 JLB : Created. * 10/04/1995 JLB : Handles construction
 *yard undeploy to move logic.                        *
 *=============================================================================================*/
void BuildingClass::Active_Click_With(ActionType action, Cell cell) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (action == ACTION_ATTACK) {
    Player_Assign_Mission(MISSION_ATTACK, Target::FromCell(cell));
  }

  if (action == ACTION_MOVE && *this == STRUCT_CONST) {
    TheNetwork().out_list().Add(EventClass(
        EventClass::ARCHIVE, Target::Of(this), Target::FromCellIndex(cell)));
    TheNetwork().out_list().Add(EventClass(EventClass::SELL, Target::Of(this)));
  }
}

/***********************************************************************************************
 * BuildingClass::Assign_Target -- Assigns a target to the building. *
 *                                                                                             *
 *    Assigning of a target to a building makes sense if the building is one
 *that can attack.  * This routine would be used to assign the attack target to
 *a turret or guard tower.       *
 *                                                                                             *
 * INPUT:   target   -- The target that was clicked on while this building was
 *selected.       *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/28/1994 JLB : Created. * 11/02/1994 JLB : Checks for range
 *before assigning target.                                *
 *=============================================================================================*/
void BuildingClass::Assign_Target(Target target) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (*this != STRUCT_SAM && *this != STRUCT_AAGUN && !In_Range(target, 0)) {
    target = Target{};
  }

  TechnoClass::Assign_Target(target);
}

/***********************************************************************************************
 * BuildingClass::Init -- Initialize the building system to an empty null state.
 **
 *                                                                                             *
 *    This routine initializes the building system in preparation for a scenario
 *load.         *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Init() { TheObjectHeaps().building().FreeAll(); }

/***********************************************************************************************
 * BuildingClass::Exit_Object -- Initiates an object to leave the building. *
 *                                                                                             *
 *    This function is used to cause an object to exit the building. It is
 *called when a       * factory produces a vehicle or other mobile object and
 *that object needs to exit the      * building to join the ranks of a regular
 *unit. Typically, the object is placed down on    * the map such that it
 *overlaps the building and then it is given a movement order so that * it will
 *move to an adjacent free cell. *
 *                                                                                             *
 * INPUT:   base  -- Pointer to the object that is to exit the building. *
 *                                                                                             *
 * OUTPUT:  Returns the success rating for the exit attempt; * 0  = complete
 *failure (refund money please)                                     * 1  =
 *temporarily prevented (try again later please)                             *
 *             2  = successful *
 *                                                                                             *
 * WARNINGS:   The building is placed in radio contact with the object. The
 *object is in a     * tethered condition. This condition will be automatically
 *broken when the        * object reaches the adjacent square. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 JLB : Created. * 04/10/1995 JLB : Handles building
 *production by computer.                                 * 06/17/1995 JLB :
 *Handles refinery exit.                                                   *
 *=============================================================================================*/
int BuildingClass::Exit_Object(TechnoClass* base) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (!base) {
    return 0;
  }

  /*
  **	A unit exiting a building is always considered to be "locked". That
  *means, it *	will be considered as to have legally entered the visible map
  *domain.
  */
  base->IsLocked = true;

  /*
  **	Find a good cell to unload the object to. The object, probably a vehicle
  **	will drive/walk to the adjacent free cell.
  */
  Cell cell{};

  switch (base->What_Am_I()) {
    case RTTI_AIRCRAFT:
      if (!radio().InContact()) {
        auto* air = dynamic_cast<AircraftClass*>(base);

        air->Height = 0;
        TheWorld().scenario_init()++;
        if (air->Unlimbo(Docking_Coord(), air->Pose_Dir())) {
          Transmit_Message(RADIO_HELLO, air);
          Transmit_Message(RADIO_TETHER);
          TheWorld().scenario_init()--;
          return 2;
        }
        TheWorld().scenario_init()--;
      } else {
        auto* air = dynamic_cast<AircraftClass*>(base);

        if (Cell::Containing(Center_Coord()).x() - TheMap().MapCellX <
            TheMap().MapCellWidth / 2) {
          cell = Cell::FromXY(
              TheMap().MapCellX - 1,
              RandomPick(0, TheMap().MapCellHeight - 1) + TheMap().MapCellY);
        } else {
          cell = Cell::FromXY(
              TheMap().MapCellX + TheMap().MapCellWidth,
              RandomPick(0, TheMap().MapCellHeight - 1) + TheMap().MapCellY);
        }
        TheWorld().scenario_init()++;
        if (air->Unlimbo(cell.Center(), game::Direction::kNorth)) {
          // BG
          // air->Assign_Destination(ra::As_Target(Nearby_Location(air)));
          /*BG*/ air->Assign_Destination(
              Target::FromCell(air->Nearby_Location(this)));
          air->Assign_Mission(MISSION_MOVE);
          TheWorld().scenario_init()--;
          return 2;
        }
        TheWorld().scenario_init()--;
      }
      break;

    case RTTI_VESSEL:
      switch (Class->Type) {
        case STRUCT_SUB_PEN:
        case STRUCT_SHIP_YARD:
          TheWorld().scenario_init()++;
          cell = Find_Exit_Cell(base);
          if (cell.index() != 0 &&
              base->Unlimbo(cell.Center(), DirectionTo(cell.Center()))) {
            base->Assign_Mission(MISSION_GUARD);
            TheWorld().scenario_init()--;
            return 2;
          }
          TheWorld().scenario_init()--;
          break;

        case StructType::STRUCT_NONE:
        case StructType::STRUCT_ADVANCED_TECH:
        case StructType::STRUCT_IRON_CURTAIN:
        case StructType::STRUCT_WEAP:
        case StructType::STRUCT_CHRONOSPHERE:
        case StructType::STRUCT_PILLBOX:
        case StructType::STRUCT_CAMOPILLBOX:
        case StructType::STRUCT_RADAR:
        case StructType::STRUCT_GAP:
        case StructType::STRUCT_TURRET:
        case StructType::STRUCT_AAGUN:
        case StructType::STRUCT_FLAME_TURRET:
        case StructType::STRUCT_CONST:
        case StructType::STRUCT_REFINERY:
        case StructType::STRUCT_STORAGE:
        case StructType::STRUCT_HELIPAD:
        case StructType::STRUCT_SAM:
        case StructType::STRUCT_AIRSTRIP:
        case StructType::STRUCT_POWER:
        case StructType::STRUCT_ADVANCED_POWER:
        case StructType::STRUCT_SOVIET_TECH:
        case StructType::STRUCT_HOSPITAL:
        case StructType::STRUCT_BARRACKS:
        case StructType::STRUCT_TENT:
        case StructType::STRUCT_KENNEL:
        case StructType::STRUCT_REPAIR:
        case StructType::STRUCT_BIO_LAB:
        case StructType::STRUCT_MISSION:
        case StructType::STRUCT_MSLO:
        case StructType::STRUCT_FORWARD_COM:
        case StructType::STRUCT_TESLA:
        case StructType::STRUCT_FAKEWEAP:
        case StructType::STRUCT_FAKECONST:
        case StructType::STRUCT_FAKE_YARD:
        case StructType::STRUCT_FAKE_PEN:
        case StructType::STRUCT_FAKE_RADAR:
        case StructType::STRUCT_SANDBAG_WALL:
        case StructType::STRUCT_CYCLONE_WALL:
        case StructType::STRUCT_BRICK_WALL:
        case StructType::STRUCT_BARBWIRE_WALL:
        case StructType::STRUCT_WOOD_WALL:
        case StructType::STRUCT_FENCE:
        case StructType::STRUCT_AVMINE:
        case StructType::STRUCT_APMINE:
        case StructType::STRUCT_V01:
        case StructType::STRUCT_V02:
        case StructType::STRUCT_V03:
        case StructType::STRUCT_V04:
        case StructType::STRUCT_V05:
        case StructType::STRUCT_V06:
        case StructType::STRUCT_V07:
        case StructType::STRUCT_V08:
        case StructType::STRUCT_V09:
        case StructType::STRUCT_V10:
        case StructType::STRUCT_V11:
        case StructType::STRUCT_V12:
        case StructType::STRUCT_V13:
        case StructType::STRUCT_V14:
        case StructType::STRUCT_V15:
        case StructType::STRUCT_V16:
        case StructType::STRUCT_V17:
        case StructType::STRUCT_V18:
        case StructType::STRUCT_PUMP:
        case StructType::STRUCT_V20:
        case StructType::STRUCT_V21:
        case StructType::STRUCT_V22:
        case StructType::STRUCT_V23:
        case StructType::STRUCT_V24:
        case StructType::STRUCT_V25:
        case StructType::STRUCT_V26:
        case StructType::STRUCT_V27:
        case StructType::STRUCT_V28:
        case StructType::STRUCT_V29:
        case StructType::STRUCT_V30:
        case StructType::STRUCT_V31:
        case StructType::STRUCT_V32:
        case StructType::STRUCT_V33:
        case StructType::STRUCT_V34:
        case StructType::STRUCT_V35:
        case StructType::STRUCT_V36:
        case StructType::STRUCT_V37:
        case StructType::STRUCT_BARREL:
        case StructType::STRUCT_BARREL3:
        case StructType::STRUCT_QUEEN:
        case StructType::STRUCT_LARVA1:
        case StructType::STRUCT_LARVA2:
        default:
          break;
      }
      break;

    case RTTI_INFANTRY:
    case RTTI_UNIT:
      switch (Class->Type) {
        case STRUCT_REFINERY:
          if (base->What_Am_I() == RTTI_UNIT) {
            cell = Cell::Containing(Center_Coord());
            auto* unit = dynamic_cast<UnitClass*>(base);

            cell = AdjacentCell(cell, game::Facing::kSouthWest);
            TheWorld().scenario_init()++;
            if (unit->Unlimbo(
                    AdjacentCell(cell, game::Direction::kSouth).Center(),
                    DriveClass::kRefineryDock)) {
              unit->PrimaryFacing.SnapTo(game::Direction::kSouth);
              unit->Assign_Mission(MISSION_HARVEST);
            }
            TheWorld().scenario_init()--;
          } else {
            base->Scatter(Coordinate{}, true);
          }
          break;

        case STRUCT_WEAP:
          if (mission_state().current() == MISSION_UNLOAD) {
            for (int index = 0; index < TheObjectHeaps().building().Count();
                 index++) {
              BuildingClass* bldg = TheObjectHeaps().building().ActiveAt(index);
              if (bldg->Owner() == Owner() && *bldg == STRUCT_WEAP &&
                  bldg != this &&
                  bldg->mission_state().current() == MISSION_GUARD &&
                  !bldg->Factory) {
                FactoryClass* temp = Factory;
                bldg->Factory = Factory;
                Factory = nullptr;
                const int retval = bldg->Exit_Object(base);
                bldg->Factory = nullptr;
                Factory = temp;
                return retval;
              }
            }
            return 1;  // fail while we're still unloading previous
          }
          TheWorld().scenario_init()++;
          if (base->Unlimbo(Exit_Coord(), game::Direction::kSouth)) {
            base->Mark(MARK_UP);
            base->Coord = Exit_Coord();
            base->Mark(MARK_DOWN);
            Transmit_Message(RADIO_HELLO, base);
            Transmit_Message(RADIO_TETHER);
            Assign_Mission(MISSION_UNLOAD);
            TheWorld().scenario_init()--;
            return 2;
          }
          TheWorld().scenario_init()--;
          break;

        case STRUCT_BARRACKS:
        case STRUCT_TENT:
        case STRUCT_KENNEL:

          cell = Find_Exit_Cell(base);
          if (cell.index() != 0) {
            const game::Direction dir = DirectionTo(cell);
            const Coordinate start = Exit_Coord();

            TheWorld().scenario_init()++;
            if (base->Unlimbo(start, dir)) {
              base->Assign_Mission(MISSION_MOVE);

              /*
              **	When disembarking from a transport then guard an area
              *around the *	center of the base.
              */
              base->Assign_Destination(Target::FromCell(cell));
              if (House->IQ >= TheRules().IQGuardArea) {
                base->Assign_Mission(MISSION_GUARD_AREA);
                base->ArchiveTarget = Target::FromCell(
                    House->Where_To_Go(dynamic_cast<FootClass*>(base)));
              }

              /*
              **	Establish radio contact so unload coordination can
              *occur. This *	radio contact should always succeed.
              */
              if (Transmit_Message(RADIO_HELLO, base) == RADIO_ROGER) {
                Transmit_Message(RADIO_UNLOAD);
              }
              TheWorld().scenario_init()--;
              return 2;
            }
            TheWorld().scenario_init()--;
          }
          break;

        case StructType::STRUCT_NONE:
        case StructType::STRUCT_ADVANCED_TECH:
        case StructType::STRUCT_IRON_CURTAIN:
        case StructType::STRUCT_CHRONOSPHERE:
        case StructType::STRUCT_PILLBOX:
        case StructType::STRUCT_CAMOPILLBOX:
        case StructType::STRUCT_RADAR:
        case StructType::STRUCT_GAP:
        case StructType::STRUCT_TURRET:
        case StructType::STRUCT_AAGUN:
        case StructType::STRUCT_FLAME_TURRET:
        case StructType::STRUCT_CONST:
        case StructType::STRUCT_STORAGE:
        case StructType::STRUCT_HELIPAD:
        case StructType::STRUCT_SAM:
        case StructType::STRUCT_AIRSTRIP:
        case StructType::STRUCT_POWER:
        case StructType::STRUCT_ADVANCED_POWER:
        case StructType::STRUCT_SOVIET_TECH:
        case StructType::STRUCT_HOSPITAL:
        case StructType::STRUCT_REPAIR:
        case StructType::STRUCT_BIO_LAB:
        case StructType::STRUCT_MISSION:
        case StructType::STRUCT_SHIP_YARD:
        case StructType::STRUCT_SUB_PEN:
        case StructType::STRUCT_MSLO:
        case StructType::STRUCT_FORWARD_COM:
        case StructType::STRUCT_TESLA:
        case StructType::STRUCT_FAKEWEAP:
        case StructType::STRUCT_FAKECONST:
        case StructType::STRUCT_FAKE_YARD:
        case StructType::STRUCT_FAKE_PEN:
        case StructType::STRUCT_FAKE_RADAR:
        case StructType::STRUCT_SANDBAG_WALL:
        case StructType::STRUCT_CYCLONE_WALL:
        case StructType::STRUCT_BRICK_WALL:
        case StructType::STRUCT_BARBWIRE_WALL:
        case StructType::STRUCT_WOOD_WALL:
        case StructType::STRUCT_FENCE:
        case StructType::STRUCT_AVMINE:
        case StructType::STRUCT_APMINE:
        case StructType::STRUCT_V01:
        case StructType::STRUCT_V02:
        case StructType::STRUCT_V03:
        case StructType::STRUCT_V04:
        case StructType::STRUCT_V05:
        case StructType::STRUCT_V06:
        case StructType::STRUCT_V07:
        case StructType::STRUCT_V08:
        case StructType::STRUCT_V09:
        case StructType::STRUCT_V10:
        case StructType::STRUCT_V11:
        case StructType::STRUCT_V12:
        case StructType::STRUCT_V13:
        case StructType::STRUCT_V14:
        case StructType::STRUCT_V15:
        case StructType::STRUCT_V16:
        case StructType::STRUCT_V17:
        case StructType::STRUCT_V18:
        case StructType::STRUCT_PUMP:
        case StructType::STRUCT_V20:
        case StructType::STRUCT_V21:
        case StructType::STRUCT_V22:
        case StructType::STRUCT_V23:
        case StructType::STRUCT_V24:
        case StructType::STRUCT_V25:
        case StructType::STRUCT_V26:
        case StructType::STRUCT_V27:
        case StructType::STRUCT_V28:
        case StructType::STRUCT_V29:
        case StructType::STRUCT_V30:
        case StructType::STRUCT_V31:
        case StructType::STRUCT_V32:
        case StructType::STRUCT_V33:
        case StructType::STRUCT_V34:
        case StructType::STRUCT_V35:
        case StructType::STRUCT_V36:
        case StructType::STRUCT_V37:
        case StructType::STRUCT_BARREL:
        case StructType::STRUCT_BARREL3:
        case StructType::STRUCT_QUEEN:
        case StructType::STRUCT_LARVA1:
        case StructType::STRUCT_LARVA2:
        default:
          cell = Find_Exit_Cell(base);
          if (cell.index() != 0) {
            const game::Direction dir = DirectionTo(cell);
            const Coordinate start = Exit_Coord();

            TheWorld().scenario_init()++;
            if (base->Unlimbo(start, dir)) {
              base->Assign_Mission(MISSION_MOVE);

              /*
              **	When disembarking from a transport then guard an area
              *around the *	center of the base.
              */
              base->Assign_Destination(Target::FromCell(cell));
              if (House->IQ >= TheRules().IQGuardArea) {
                base->Assign_Mission(MISSION_GUARD_AREA);
                base->ArchiveTarget = Target::FromCell(
                    House->Where_To_Go(dynamic_cast<FootClass*>(base)));
              }
              TheWorld().scenario_init()--;
              return 2;
            }
            TheWorld().scenario_init()--;
          }
          break;
      }
      break;

    case RTTI_BUILDING:

      if (!House->IsHuman) {
        /*
        **	Find the next available spot to place this newly created
        *building. If the *	building could be placed at the desired
        *location, fine. If not, then this *	routine will return failure. The
        *calling routine will probably abandon this *	building in preference
        *to building another.
        */
        const BaseNode* node = TheWorld().base().FirstUnbuiltNode(
            dynamic_cast<BuildingClass*>(base)->Class->Type);
        Coordinate coord{};
        if (node) {
          coord = node->cell.Center();
        } else {
          /*
          **	Find a suitable new spot to place.
          */
          coord =
              House->Find_Build_Location(dynamic_cast<BuildingClass*>(base));
        }

        if (coord) {
          if (Flush_For_Placement(base, Cell::Containing(coord))) {
            return 1;
          }
          if (base->Unlimbo(coord)) {
            if (node && dynamic_cast<BuildingClass*>(base)->Class->Type ==
                            House->BuildStructure) {
              House->BuildStructure = STRUCT_NONE;
            }
            return 2;
          }
        }
      }
      break;

    case RTTIType::RTTI_NONE:
    case RTTIType::RTTI_AIRCRAFTTYPE:
    case RTTIType::RTTI_ANIM:
    case RTTIType::RTTI_ANIMTYPE:
    case RTTIType::RTTI_BUILDINGTYPE:
    case RTTIType::RTTI_BULLET:
    case RTTIType::RTTI_BULLETTYPE:
    case RTTIType::RTTI_CELL:
    case RTTIType::RTTI_FACTORY:
    case RTTIType::RTTI_HOUSE:
    case RTTIType::RTTI_HOUSETYPE:
    case RTTIType::RTTI_INFANTRYTYPE:
    case RTTIType::RTTI_OVERLAY:
    case RTTIType::RTTI_OVERLAYTYPE:
    case RTTIType::RTTI_SMUDGE:
    case RTTIType::RTTI_SMUDGETYPE:
    case RTTIType::RTTI_SPECIAL:
    case RTTIType::RTTI_TEAM:
    case RTTIType::RTTI_TEAMTYPE:
    case RTTIType::RTTI_TEMPLATE:
    case RTTIType::RTTI_TEMPLATETYPE:
    case RTTIType::RTTI_TERRAIN:
    case RTTIType::RTTI_TERRAINTYPE:
    case RTTIType::RTTI_TRIGGER:
    case RTTIType::RTTI_TRIGGERTYPE:
    case RTTIType::RTTI_UNITTYPE:
    case RTTIType::RTTI_VESSELTYPE:
    default:
      break;
  }

  /*
  **	Failure to exit the object results in a false return value.
  */
  return 0;
}

/***********************************************************************************************
 * BuildingClass::Update_Buildables -- Informs sidebar of additional
 *construction options.     *
 *                                                                                             *
 *    This routine will tell the sidebar of objects that can be built. The
 *function is called  * whenever a building matures. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/11/1994 JLB : Created. * 12/23/1994 JLB : Only updates for
 *PLAYER buildings.                                       *
 *=============================================================================================*/
void BuildingClass::Update_Buildables() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (House == ThePlayer() && !IsInLimbo && IsDiscoveredByPlayer) {
    switch (Class->ToBuild) {
      case RTTI_VESSELTYPE:
        for (const VesselType v : magic_enum::enum_values<VesselType>()) {
          if (ThePlayer()->Can_Build(&VesselTypeClass::As_Reference(v),
                                     ActLike)) {
            TheMap().Add(RTTI_VESSELTYPE, static_cast<int>(v));
          }
        }
        break;

      case RTTI_BUILDINGTYPE:
        for (const StructType i : magic_enum::enum_values<StructType>()) {
          if (ThePlayer()->Can_Build(&BuildingTypeClass::As_Reference(i),
                                     ActLike)) {
            TheMap().Add(RTTI_BUILDINGTYPE, static_cast<int>(i));
          }
        }
        break;

      case RTTI_UNITTYPE:
        for (const UnitType u : magic_enum::enum_values<UnitType>()) {
          if (ThePlayer()->Can_Build(&UnitTypeClass::As_Reference(u),
                                     ActLike)) {
            TheMap().Add(RTTI_UNITTYPE, static_cast<int>(u));
          }
        }
        break;

      case RTTI_INFANTRYTYPE:
        for (const InfantryType f : magic_enum::enum_values<InfantryType>()) {
          if (ThePlayer()->Can_Build(&InfantryTypeClass::As_Reference(f),
                                     ActLike)) {
            if (InfantryTypeClass::As_Reference(f).IsDog) {
              if (*this == STRUCT_KENNEL) {
                TheMap().Add(RTTI_INFANTRYTYPE, static_cast<int>(f));
              }
            } else {
              if (*this != STRUCT_KENNEL) {
                TheMap().Add(RTTI_INFANTRYTYPE, static_cast<int>(f));
              }
            }
          }
        }
        break;

      case RTTI_AIRCRAFTTYPE:
        for (const AircraftType a : magic_enum::enum_values<AircraftType>()) {
          if (ThePlayer()->Can_Build(&AircraftTypeClass::As_Reference(a),
                                     ActLike)) {
            TheMap().Add(RTTI_AIRCRAFTTYPE, static_cast<int>(a));
          }
        }
        break;

      case RTTIType::RTTI_NONE:
      case RTTIType::RTTI_AIRCRAFT:
      case RTTIType::RTTI_ANIM:
      case RTTIType::RTTI_ANIMTYPE:
      case RTTIType::RTTI_BUILDING:
      case RTTIType::RTTI_BULLET:
      case RTTIType::RTTI_BULLETTYPE:
      case RTTIType::RTTI_CELL:
      case RTTIType::RTTI_FACTORY:
      case RTTIType::RTTI_HOUSE:
      case RTTIType::RTTI_HOUSETYPE:
      case RTTIType::RTTI_INFANTRY:
      case RTTIType::RTTI_OVERLAY:
      case RTTIType::RTTI_OVERLAYTYPE:
      case RTTIType::RTTI_SMUDGE:
      case RTTIType::RTTI_SMUDGETYPE:
      case RTTIType::RTTI_SPECIAL:
      case RTTIType::RTTI_TEAM:
      case RTTIType::RTTI_TEAMTYPE:
      case RTTIType::RTTI_TEMPLATE:
      case RTTIType::RTTI_TEMPLATETYPE:
      case RTTIType::RTTI_TERRAIN:
      case RTTIType::RTTI_TERRAINTYPE:
      case RTTIType::RTTI_TRIGGER:
      case RTTIType::RTTI_TRIGGERTYPE:
      case RTTIType::RTTI_UNIT:
      case RTTIType::RTTI_VESSEL:
      default:
        break;
    }
  }
}

/***********************************************************************************************
 * BuildingClass::Fire_Out -- Handles when attached animation expires. *
 *                                                                                             *
 *    This routine is used to perform any fixups necessary when the attached
 *animation has     * terminated. This occurs when the fire & smoke animation
 *that a SAM site produces stops.  * At that point, normal reload procedures can
 *commence.                                    *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/30/1994 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Fire_Out() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);
}

/***********************************************************************************************
 * BuildingClass::Limbo -- Handles power adjustment as building goes into limbo.
 **
 *                                                                                             *
 *    This routine will handle the power adjustments for the associated house
 *when the         * building goes into limbo. This means that its power drain
 *or production is subtracted    * from the house accumulated totals. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Was the building limboed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/24/1994 JLB : Created. *
 *=============================================================================================*/
bool BuildingClass::Limbo() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (!IsInLimbo) {
    /*
    **	Update the total factory type, assuming this building has a factory.
    */
    House->Active_Remove(this);
    House->IsRecalcNeeded = true;
    House->Recalc_Center();

    /*
    **	Update the power status of the owner's house.
    */
    House->Adjust_Power(-Power_Output());
    House->Adjust_Drain(-Class->Drain);
    House->Adjust_Capacity(-Class->Capacity, true);
    if (House == ThePlayer()) {
      TheMap().PowerClass::IsPowerToRedraw = true;
      TheMap().Flag_To_Redraw(false);
    }

    /*
    **	This could be a building that builds. If so, then the sidebar may need
    *adjustment.
    ** Set IsInLimbo to true to "fool" the sidebar into knowing that this
    *building
    ** isn't available.  Set it back to false so the rest of the Limbo code
    *works.
    ** Otherwise, the sidebar won't properly remove non-available buildables.
    */
    //		if (IsOwnedByPlayer && !ScenarioInit) {
    //			IsInLimbo = true;
    //			Map.Recalc();
    //			IsInLimbo = false;
    //		}
  }
  return TechnoClass::Limbo();
}

/***********************************************************************************************
 * BuildingClass::Turret_Facing -- Fetches the turret facing for this building.
 **
 *                                                                                             *
 *    This will return the turret facing for this building. Some buildings don't
 *have a        * visual turret (e.g., pillbox) so they return a turret facing
 *that always faces their     * current target. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the current facing of the turret. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/29/1996 JLB : Created. *
 *=============================================================================================*/
game::Direction BuildingClass::Turret_Facing() const {
  if (!Class->IsTurretEquipped && TarCom) {
    return ra::DirectionBetween(Center_Coord(), TarCom.AsCoordinate());
  }
  return PrimaryFacing.current();
}

/***********************************************************************************************
 * BuildingClass::Greatest_Threat -- Searches for target that building can fire
 *upon.          *
 *                                                                                             *
 *    This routine intercepts the Greatest_Threat function so that it can add
 *the ability      * to search for ground targets, if this isn't a SAM site. *
 *                                                                                             *
 * INPUT:   threat   -- The base threat control value. Typically, it might be
 *THREAT_RANGE     * or THREAT_NORMAL. *
 *                                                                                             *
 * OUTPUT:  Returns with a suitable target. If none could be found, then
 *Target{} is        * returned instead. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/01/1995 JLB : Created. *
 *=============================================================================================*/
Target BuildingClass::Greatest_Threat(ThreatType threat)  // const
{
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (Class->PrimaryWeapon != nullptr) {
    threat = threat | Class->PrimaryWeapon->Allowed_Threats();
  }
  if (Class->SecondaryWeapon != nullptr) {
    threat = threat | Class->SecondaryWeapon->Allowed_Threats();
  }
  if (House->IsHuman) {
    threat = threat & ~THREAT_BUILDINGS;
  }
  threat = threat | THREAT_RANGE;

  //	if (Class->PrimaryWeapon != nullptr) {
  //		if (Class->PrimaryWeapon->Bullet->IsAntiAircraft) {
  //			threat = threat | THREAT_AIR;
  //		}
  //		if (Class->PrimaryWeapon->Bullet->IsAntiGround) {
  //			threat = threat |
  // THREAT_BUILDINGS|THREAT_INFANTRY|THREAT_BOATS|THREAT_VEHICLES;
  //		}
  //		threat = threat | THREAT_RANGE;
  //	}
  return TechnoClass::Greatest_Threat(threat);
}

/***********************************************************************************************
 * BuildingClass::Grand_Opening -- Handles construction completed special
 *operations.          *
 *                                                                                             *
 *    This routine is called when construction has finished. Typically, this
 *enables           * new production options for factories. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/08/1995 JLB : Created. * 06/13/1995 JLB : Added helipad. *
 *=============================================================================================*/
void BuildingClass::Grand_Opening(bool captured) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (!HasOpened || captured) {
    HasOpened = true;

    /*
    **	Adjust the owning house according to the power, drain, and Tiberium
    *capacity that *	this building has.
    */
    House->Adjust_Drain(Class->Drain);
    House->Adjust_Capacity(Class->Capacity);
    House->IsRecalcNeeded = true;

    /*	SPECIAL CASE:
    **	Tiberium Refineries get a free harvester. Add a harvester to the
    **	reinforcement list at this time.
    */
    if (*this == STRUCT_REFINERY && !TheWorld().scenario_init() && !captured &&
        !TheDebugState().map_editor_active() &&
        (!House->IsHuman || PurchasePrice == 0 ||
         PurchasePrice > Class->Raw_Cost())) {
      Cell cell = Cell::Containing(
          AdjacentCell(Center_Coord(), game::Direction::kSouth));

      auto* unit =
          base::NewPooled<UnitClass>(UNIT_HARVESTER, House->Class->House);
      if (unit != nullptr) {
        /*
        **	Try to place down the harvesters. If it could not be placed,
        *then try *	to place it in a nearby location.
        */
        if (!unit->Unlimbo(cell.Center(), game::Direction::kWest)) {
          cell = unit->Nearby_Location(this);

          /*
          **	If the harvester could still not be placed, then refund the
          *money *	to the owner and then bail.
          */
          if (!unit->Unlimbo(cell.Center(), game::Direction::kSouthWest)) {
            House->Refund_Money(unit->Class->Cost_Of());
            base::DeletePooled(unit);
          }
        }
      } else {
        /*
        **	If the harvester could not be created in the first place, then
        *give *	the full refund price to the owning player.
        */
        House->Refund_Money(
            UnitTypeClass::As_Reference(UNIT_HARVESTER).Cost_Of());
      }
    }

    /*
    **	Helicopter pads get a free attack helicopter.
    */
    if (!TheRules().IsSeparate && *this == STRUCT_HELIPAD && !captured) {
      TheWorld().scenario_init()++;
      AircraftClass* air = nullptr;
      if (IsSovietHouse(House->ActLike) || House->ActLike == HOUSE_BAD) {
        air =
            base::NewPooled<AircraftClass>(AIRCRAFT_HIND, House->Class->House);
      } else {
        air = base::NewPooled<AircraftClass>(AIRCRAFT_LONGBOW,
                                             House->Class->House);
      }
      if (air) {
        air->Height = 0;
        if (air->Unlimbo(Docking_Coord(), air->Pose_Dir())) {
          air->Assign_Mission(MISSION_GUARD);
          air->Transmit_Message(RADIO_HELLO, this);
          Transmit_Message(RADIO_TETHER);
        }
      }
      TheWorld().scenario_init()--;
    }
  }
}

/***********************************************************************************************
 * BuildingClass::Repair -- Initiates or terminates the repair process. *
 *                                                                                             *
 *    This routine will start, stop, or toggle the repair process. When a
 *building repairs, it * occurs incrementally over time. *
 *                                                                                             *
 * INPUT:   control  -- Determines how to control the repair process. * 0: Turns
 *repair process off (if it was on).                            * 1: Turns
 *repair process on (if it was off).                            * -1:Toggles
 *repair process to other state.                              *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/08/1995 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Repair(int control) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  switch (control) {
    case -1:
      IsRepairing = !static_cast<bool>(IsRepairing);
      break;

    case 1:
      if (IsRepairing) {
        return;
      }
      IsRepairing = true;
      break;

    case 0:
      if (!IsRepairing) {
        return;
      }
      IsRepairing = false;
      break;

    default:
      break;
  }

  /*
  **	At this point, we know that the repair state has changed. Perform
  **	appropriate action.
  */
  VocType soundid = VOC_NONE;
  if (IsRepairing) {
    if (std::cmp_equal(Strength, Class->MaxStrength)) {
      soundid = VOC_SCOLD;
    } else {
      soundid = VOC_CLICK;
      if (House->IsPlayerControl) {
        Clicked_As_Target();
      }
      IsWrenchVisible = true;
    }
  } else {
    soundid = VOC_CLICK;
  }
  if (House->IsPlayerControl) {
    PlaySoundEffectAt(soundid, Coord);
  }
}

/***********************************************************************************************
 * BuildingClass::Sell_Back -- Controls the sell back (demolish) operation. *
 *                                                                                             *
 *    This routine will initiate or stop the sell back process for a building.
 *It is called    * when the player clicks on a building when the sell mode is
 *active.                       *
 *                                                                                             *
 * INPUT:   control  -- The action to perform. 0 = turn deconstruction off, 1 =
 *deconstruct,   * -1 = toggle deconstruction state. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Sell_Back(int control) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (!Class->Get_Buildup_Data().empty()) {
    bool decon = false;
    switch (control) {
      case -1:
        decon = mission_state().current() != MISSION_DECONSTRUCTION;
        break;

      case 1:
        if (mission_state().current() == MISSION_DECONSTRUCTION) {
          return;
        }
        if (IsGoingToBlow) {
          return;
        }
        decon = true;
        break;

      case 0:
        if (mission_state().current() != MISSION_DECONSTRUCTION) {
          return;
        }
        decon = false;
        break;

      default:
        break;
    }

    /*
    **	At this point, we know that the repair state has changed. Perform
    **	appropriate action.
    */
    if (decon) {
      Assign_Mission(MISSION_DECONSTRUCTION);
      Commence();
      if (House->IsPlayerControl) {
        Clicked_As_Target();
      }
    }
    if (House->IsPlayerControl) {
      PlaySoundEffect(VOC_CLICK);
    }
  }
}

/***********************************************************************************************
 * BuildingClass::What_Action -- Determines action to perform if click on
 *specified object.    *
 *                                                                                             *
 *    This routine will determine what action to perform if the mouse was
 *clicked on the       * object specified. This determination is used to control
 *the mouse imagery and the        * function process when the mouse button is
 *pressed.                                       *
 *                                                                                             *
 * INPUT:   object   -- Pointer to the object that, if clicked on, will control
 *what action    * is to be performed. *
 *                                                                                             *
 * OUTPUT:  Returns with the ActionType that will occur if the mouse is clicked
 *over the       * object specified while the building is currently selected. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/18/1995 JLB : Created. *
 *=============================================================================================*/
ActionType BuildingClass::What_Action(ObjectClass* object) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  ActionType action = TechnoClass::What_Action(object);

  if (action == ACTION_SELF) {
    if (Class->Is_Factory() && ThePlayer() == House &&
        *House->Factory_Counter(Class->ToBuild) > 1) {
      switch (Class->ToBuild) {
        case RTTI_INFANTRYTYPE:
        case RTTI_INFANTRY:
          action = ACTION_NONE;
          if (*this == STRUCT_KENNEL) {
            for (int index = 0; index < TheObjectHeaps().building().Count();
                 index++) {
              const BuildingClass* bldg =
                  TheObjectHeaps().building().ActiveAt(index);
              if (bldg != this && bldg->Owner() == Owner() &&
                  *bldg == STRUCT_KENNEL) {
                action = ACTION_SELF;
                break;
              }
            }
          } else {
            for (int index = 0; index < TheObjectHeaps().building().Count();
                 index++) {
              const BuildingClass* bldg =
                  TheObjectHeaps().building().ActiveAt(index);
              if (bldg != this && bldg->Owner() == Owner() &&
                  bldg->Class->ToBuild == RTTI_INFANTRYTYPE &&
                  *bldg != STRUCT_KENNEL) {
                action = ACTION_SELF;
                break;
              }
            }
          }
          break;

        case RTTI_NONE:
          action = ACTION_NONE;
          break;

        case RTTIType::RTTI_AIRCRAFT:
        case RTTIType::RTTI_AIRCRAFTTYPE:
        case RTTIType::RTTI_ANIM:
        case RTTIType::RTTI_ANIMTYPE:
        case RTTIType::RTTI_BUILDING:
        case RTTIType::RTTI_BUILDINGTYPE:
        case RTTIType::RTTI_BULLET:
        case RTTIType::RTTI_BULLETTYPE:
        case RTTIType::RTTI_CELL:
        case RTTIType::RTTI_FACTORY:
        case RTTIType::RTTI_HOUSE:
        case RTTIType::RTTI_HOUSETYPE:
        case RTTIType::RTTI_OVERLAY:
        case RTTIType::RTTI_OVERLAYTYPE:
        case RTTIType::RTTI_SMUDGE:
        case RTTIType::RTTI_SMUDGETYPE:
        case RTTIType::RTTI_SPECIAL:
        case RTTIType::RTTI_TEAM:
        case RTTIType::RTTI_TEAMTYPE:
        case RTTIType::RTTI_TEMPLATE:
        case RTTIType::RTTI_TEMPLATETYPE:
        case RTTIType::RTTI_TERRAIN:
        case RTTIType::RTTI_TERRAINTYPE:
        case RTTIType::RTTI_TRIGGER:
        case RTTIType::RTTI_TRIGGERTYPE:
        case RTTIType::RTTI_UNIT:
        case RTTIType::RTTI_UNITTYPE:
        case RTTIType::RTTI_VESSEL:
        case RTTIType::RTTI_VESSELTYPE:
        default:
          break;
      }

    } else {
      action = ACTION_NONE;
    }
  }

  /*
  **	Don't allow targeting of SAM sites, even if the CTRL key
  **	is held down. Also don't allow targeting if the object is too
  **	far away.
  */
  if (action == ACTION_ATTACK &&
      (*this == STRUCT_SAM || *this == STRUCT_AAGUN || !In_Range(object, 0))) {
    action = ACTION_NONE;
  }

  if (action == ACTION_MOVE) {
    action = ACTION_NONE;
  }

  return action;
}

/***********************************************************************************************
 * BuildingClass::What_Action -- Determines what action will occur. *
 *                                                                                             *
 *    This routine examines the cell specified and returns with the action that
 *will be        * performed if that cell were clicked upon while the building
 *is selected.                 *
 *                                                                                             *
 * INPUT:   cell  -- The cell to examine. *
 *                                                                                             *
 * OUTPUT:  Returns the ActionType that indicates what should occur if the mouse
 *is clicked    * on this cell. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/18/1995 JLB : Created. *
 *=============================================================================================*/
ActionType BuildingClass::What_Action(Cell cell) const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  ActionType action = TechnoClass::What_Action(cell);

  if (action == ACTION_MOVE &&
      (*this != STRUCT_CONST || !TheRules().IsMCVDeploy)) {
    action = ACTION_NONE;
  }

  /*
  **	Don't allow targeting of SAM sites, even if the CTRL key
  **	is held down.
  */
  if (action == ACTION_ATTACK && Class->PrimaryWeapon != nullptr &&
      !Class->PrimaryWeapon->Bullet->IsAntiGround) {
    //	if (action == ACTION_ATTACK && (*this == STRUCT_SAM || *this ==
    // STRUCT_AAGUN)) {
    action = ACTION_NONE;
  }

  return action;
}

/***********************************************************************************************
 * BuildingClass::Begin_Mode -- Begins an animation mode for the building. *
 *                                                                                             *
 *    This routine will start the building animating. This animation will loop
 *indefinitely    * until explicitly stopped. *
 *                                                                                             *
 * INPUT:   bstate   -- The animation state to initiate. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   The building graphic state will reflect the first stage of this
 *animation the   * very next time it is rendered. *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. * 07/02/1995 JLB : Uses normalize
 *animation rate where applicable.                          *
 *=============================================================================================*/
void BuildingClass::Begin_Mode(game::BStateType bstate) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  QueueBState = bstate;
  if (BState == game::BSTATE_NONE || bstate == game::BSTATE_CONSTRUCTION ||
      TheWorld().scenario_init()) {
    BState = bstate;
    QueueBState = game::BSTATE_NONE;
    const BuildingTypeClass::AnimControlType* ctrl = Fetch_Anim_Control();

    int rate = ctrl->Rate;
    if (Class->IsRegulated && bstate != game::BSTATE_CONSTRUCTION) {
      rate = TheOptions().Normalize_Delay(rate);
    }
    animation().Set_Rate(rate);
    animation().Set_Stage(ctrl->Start);
  }
}

/***********************************************************************************************
 * BuildingClass::Center_Coord -- Fetches the center coordinate for the
 *building.              *
 *                                                                                             *
 *    This routine is used to set the center coordinate for this building. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the coordinate for the center location for the
 *building.              *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
Coordinate BuildingClass::Center_Coord() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  return Coord + CenterOffset.at(Class->Size);
}

/***********************************************************************************************
 * BuildingClass::Docking_Coord -- Fetches the coordinate to use for docking. *
 *                                                                                             *
 *    This routine will return the coordinate to use when an object wishes to
 *dock with this   * building. Normally the docking coordinate would be the
 *center of the building.           * Exceptions to this would be the airfield
 *and helipad. Their docking coordinates are      * offset to match the building
 *artwork.                                                    *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the coordinate to head to when trying to dock with this
 *building.     *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/21/1995 JLB : Created. *
 *=============================================================================================*/
Coordinate BuildingClass::Docking_Coord() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (*this == STRUCT_HELIPAD) {
    return Coord + CoordFromPixels(24, 18);
  }
  if (*this == STRUCT_AIRSTRIP) {
    return Coord +
           CoordFromPixels(
               game::kCellWidthPixels + (game::kCellWidthPixels / 2), 28);
  }
  return TechnoClass::Docking_Coord();
}

/***********************************************************************************************
 * BuildingClass::Can_Fire -- Determines if this building can fire. *
 *                                                                                             *
 *    Use this routine to see if the building can fire its weapon. *
 *                                                                                             *
 *                                                                                             *
 * INPUT:   target   -- The target that firing upon is desired. *
 *                                                                                             *
 *          which    -- Which weapon to use when firing. 0=primary, 1=secondary.
 **
 *                                                                                             *
 * OUTPUT:  Returns with the fire possibility code. If firing is allowed, then
 *FIRE_OK is      * returned. Other cases will result in appropriate fire code
 *value that indicates    * why firing is not allowed. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/03/1995 JLB : Created. *
 *=============================================================================================*/
game::FireErrorType BuildingClass::Can_Fire(Target target, int which) const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  const game::FireErrorType canfire = TechnoClass::Can_Fire(target, which);

  if (canfire == game::FIRE_OK) {
    /*
    **	Double check to make sure that the facing is roughly toward
    **	the target. If the difference is too great, then firing is
    **	temporarily postponed.
    */
    if (Class->IsTurretEquipped) {
      const int diff =
          game::ShortestTurn(PrimaryFacing.current(), DirectionTo(TarCom));
      if (std::abs(diff) > (*this == STRUCT_SAM ? 64 : 8)) {
        return game::FIRE_FACING;
      }

      /*
      **	If the turret is rotating then firing must be delayed.
      */
      //			if (PrimaryFacing.IsRotating()) {
      //				return(FIRE_ROTATING);
      //			}
    }

    /*
    **	Certain buildings cannot fire if there is insufficient power.
    */
    if (Class->IsPowered && House->Power_Fraction() < 1) {
      return game::FIRE_BUSY;
    }

    /*
    ** If an obelisk can fire, check the state of charge.
    */
    if (Class->PrimaryWeapon != nullptr && Class->PrimaryWeapon->IsElectric &&
        !IsCharged) {
      return game::FIRE_BUSY;
    }
  }
  return canfire;
}

/***********************************************************************************************
 * BuildingClass::Toggle_Primary -- Toggles the primary factory state. *
 *                                                                                             *
 *    This routine will change the primary factory state of this building. The
 *primary         * factory is the one that units will be produced from (by
 *default).                        *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Is this building NOW the primary factory? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/03/1995 JLB : Created. *
 *=============================================================================================*/
bool BuildingClass::Toggle_Primary() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (IsLeader) {
    IsLeader = false;
  } else {
    for (int index = 0; index < TheObjectHeaps().building().Count(); index++) {
      BuildingClass* building = TheObjectHeaps().building().ActiveAt(index);

      if (!building->IsInLimbo && building->Owner() == Owner() &&
          building->Class->ToBuild == Class->ToBuild) {
        if (Class->ToBuild == RTTI_INFANTRYTYPE) {
          if (*building == STRUCT_KENNEL && *this == STRUCT_KENNEL) {
            building->IsLeader = false;
          } else {
            if (*building != STRUCT_KENNEL && *this != STRUCT_KENNEL) {
              building->IsLeader = false;
            }
          }
        } else {
          building->IsLeader = false;
        }
      }
    }
    IsLeader = true;
    if (House == ThePlayer()) {
      Speak(VOX_PRIMARY_SELECTED);
    }
  }
  Mark(MARK_CHANGE);
  return IsLeader;
}

/***********************************************************************************************
 * BuildingClass::Captured -- Captures the building. *
 *                                                                                             *
 *    This routine will change the owner of the building. It handles updating
 *any related      * game systems as a result. Factories are the most prone to
 *have great game related        * consequences when captured. This could also
 *affect the sidebar and building ownership.   *
 *                                                                                             *
 * INPUT:   newowner -- Pointer to the house that is now the new owner. *
 *                                                                                             *
 * OUTPUT:  Was the capture attempt successful? *
 *                                                                                             *
 * WARNINGS:   Capturing could fail if the house is already owned by the one
 *specified or      * the building isn't allowed to be captured. *
 *                                                                                             *
 * HISTORY: * 05/03/1995 JLB : Created. * 07/05/1995 JLB : Fixed production
 *problem with capturing enemy buildings.                 *
 *=============================================================================================*/
bool BuildingClass::Captured(HouseClass* newowner) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (Class->IsCaptureable && newowner != House) {
    /*
    ** Make sure the capturer isn't spying on his own building, and if
    ** it was a radar facility, update the target house's RadarSpied field.
    */
    if (SpiedBy & base::Bit<uint32_t>(newowner->Class->House)) {
      SpiedBy &= ~base::Bit<uint32_t>(newowner->Class->House);
      if (*this == STRUCT_RADAR) {
        Update_Radar_Spied();
      }
    }

    if (House == ThePlayer()) {
      TheMap().PowerClass::IsPowerToRedraw = true;
      TheMap().Flag_To_Redraw(false);
    }

    if (*this == STRUCT_GAP) {
      Remove_Gap_Effect();
      IsJamming = false;
      Arm.Set(0);
    }

    /*
    ** Add this building to the list of buildings captured this game. For
    *internet stats purposes.
    */
    if (TheSession().Type == GAME_INTERNET) {
      newowner->CapturedBuildings->Increment_Unit_Total(
          static_cast<int>(Class->Type));
    }

    House->Adjust_Power(-Power_Output());
    LastStrength = 0;
    House->Adjust_Drain(-Class->Drain);
    const int booty = House->Adjust_Capacity(-Class->Capacity, true);

    /*
    **	If there is something loaded, then it gets captured as well.
    */
    TechnoClass* tech = cargo().Attached_Object();
    if (tech) {
      tech->Captured(newowner);
    }

    /*
    **	If something isn't technically attached, but is sitting on this
    **	building for another reason (e.g., helicopter on helipad), then it
    **	gets captured as well.
    */
    tech = Contact_With_Whom();
    if (tech) {
      if (Transmit_Message(RADIO_NEED_TO_MOVE) == RADIO_ROGER &&
          (ra::Distance(tech->Center_Coord(), Docking_Coord()) < 0x0040 ||
           (tech->What_Am_I() == RTTI_AIRCRAFT &&
            dynamic_cast<AircraftClass*>(tech)->Class->IsFixedWing &&
            dynamic_cast<AircraftClass*>(tech)->In_Which_Layer() ==
                LAYER_GROUND))) {
        tech->Captured(newowner);
      } else {
        Transmit_Message(RADIO_RUN_AWAY);
        Transmit_Message(RADIO_OVER_OUT);
      }
    }

    /*
    **	Abort any computer production in progress.
    */
    if (Factory) {
      delete Factory;
      Factory = nullptr;
    }

    /*
    **	Decrement the factory counter for the original owner.
    */
    House->Active_Remove(this);

    /*
    **	Flag that both owners now need to update their buildable lists.
    */
    House->IsRecalcNeeded = true;
    newowner->IsRecalcNeeded = true;
    HouseClass* oldowner = House;
    const Target tocap = As_Target();

    IsCaptured = true;
    TechnoClass::Captured(newowner);

    oldowner->ToCapture = tocap;
    oldowner->Recalc_Center();
    House->Recalc_Center();
    if (House->ToCapture == As_Target()) {
      House->ToCapture = Target{};
    }

    SmudgeType bib = SMUDGE_NONE;
    Cell cell = Cell::Containing(Coord);
    if (Class->Bib_And_Offset(bib, cell)) {
      auto* smudge = base::NewPooled<SmudgeClass>(bib);
      if (smudge) {
        smudge->Disown(cell);
        base::DeletePooled(smudge);
      }
      if (TheSession().Type == GAME_NORMAL) {
        base::NewPooled<SmudgeClass>(
            bib, cell.Center(),
            Class->IsBase ? House->Class->House : HOUSE_NONE);
      } else {
        base::NewPooled<SmudgeClass>(bib, cell.Center(), House->Class->House);
      }
    }

    House->Harvested(booty);
    House->Stole(Refund_Amount());

    /*
    **	Increment the factory count for the new owner.
    */
    House->Active_Add(this);

    IsRepairing = false;
    Grand_Opening(true);

    Mark(MARK_CHANGE);

    /*
    **	Perform a look operation when captured if it was the player
    **	that performed the capture.
    */
    if (House == ThePlayer()) {
      Look(false);
    }
    /*
    ** If it was spied upon by the player who just captured it, clear the
    ** spiedby flag for that house.
    */
    if (SpiedBy & base::Bit<uint32_t>(newowner->Class->House)) {
      SpiedBy &= ~base::Bit<uint32_t>(newowner->Class->House);
    }

    /*
    ** Update the new building's colors on the radar map.
    */
    std::span<const int16_t> offset = Occupy_List();
    while (offset.front() != kRefreshEol) {
      const Cell footprint_cell =
          Cell::Containing(Coord).Offset(base::ConsumeFront(offset));
      TheMap().Radar_Pixel(footprint_cell);
    }
    return true;
  }
  return false;
}

/***********************************************************************************************
 * BuildingClass::Sort_Y -- Returns the building coordinate used for sorting. *
 *                                                                                             *
 *    The coordinate value returned from this function should be used for
 *sorting purposes.    * It has special offset adjustment applied so that
 *vehicles don't overlap (as much).       *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a coordinate value suitable to be used for sorting. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/23/1995 JLB : Created. * 06/19/1995 JLB : Handles buildings
 *that come with bibs built-in.                          *
 *=============================================================================================*/
Coordinate BuildingClass::Sort_Y() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (*this == STRUCT_REPAIR) {
    return Coord;
  }
  if (*this == STRUCT_BARRACKS /*|| *this == STRUCT_POWER*/) {
    return Center_Coord();
  }
  if (*this == STRUCT_REFINERY) {
    return Center_Coord();
  }

  /*
  **	Mines need to bias their sort location such that they are typically
  *drawn *	before any objects that might overlap them.
  */
  if (*this == STRUCT_AVMINE || *this == STRUCT_APMINE) {
    return Coord_Move(Center_Coord(), game::Direction::kNorth,
                      game::kCellHeightLeptons);
  }

  return Center_Coord() + Coordinate::FromLeptons(0, Class->Height() * 256 / 3);
}

/***********************************************************************************************
 * BuildingClass::Can_Enter_Cell -- Determines if building can be placed down. *
 *                                                                                             *
 *    This routine will determine if the building can be placed down at the
 *location           * specified. *
 *                                                                                             *
 * INPUT:   cell  -- The cell to examine. This is usually the cell of the upper
 *left corner    * of the building if it were to be placed down. *
 *                                                                                             *
 * OUTPUT:  Returns with the move legality value for placement at the location
 *specified. This * will either be MOVE_OK or MOVE_NO. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. *
 *=============================================================================================*/
game::MoveType BuildingClass::Can_Enter_Cell(Cell cell,
                                             game::Facing /*unused*/) const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (*this == STRUCT_CONST && IsDown) {
    return TheMap().at(cell).Is_Clear_To_Build(Class->Speed) ? game::MOVE_OK
                                                             : game::MOVE_NO;
  }

  if (!TheDebugState().map_editor_active() && TheWorld().scenario_init() == 0 &&
      TheSession().Type == GAME_NORMAL && House->IsPlayerControl &&
      !TheMap().at(cell).IsMapped) {
    return game::MOVE_NO;
  }

  return Class->Legal_Placement(cell) ? game::MOVE_OK : game::MOVE_NO;
}

/***********************************************************************************************
 * BuildingClass::Can_Demolish -- Can the player demolish (sell back) the
 *building?            *
 *                                                                                             *
 *    Determines if the player can sell this building. Selling is possible if
 *the building     * is not currently in construction or deconstruction
 *animation.                            *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Can the building be demolished at this time? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. * 07/01/1995 JLB : If there is no
 *buildup data, then the building can't be sold.            * 07/17/1995 JLB :
 *Cannot sell a refinery that has a harvester attached.                    *
 *=============================================================================================*/
bool BuildingClass::Can_Demolish() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (Class->IsUnsellable) {
    return false;
  }

  if (!Class->Get_Buildup_Data().empty() &&
      BState != game::BSTATE_CONSTRUCTION &&
      mission_state().current() != MISSION_DECONSTRUCTION &&
      mission_state().current() != MISSION_CONSTRUCTION) {
    return *this != STRUCT_REFINERY || !cargo().Is_Something_Attached();
  }
  return false;
}

/***********************************************************************************************
 * BuildingClass::Mission_Guard -- Handles guard mission for combat buildings. *
 *                                                                                             *
 *    Buildings that can attack are given this mission. They will wait until a
 *suitable target * comes within range and then launch into the attack mission.
 *Buildings that have no       * weaponry will just sit in this routine forever.
 **
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of game frames to delay before this routine
 *will be called * again. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. *
 *=============================================================================================*/
int BuildingClass::Mission_Guard() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  /*
  **	If this building has a weapon, then search for a target to attack. When
  **	a target is found, switch into attack mode to deal with the threat.
  */
  if (Is_Weapon_Equipped()) {
    /*
    **	Weapon equipped buildings are ALWAYS ready to launch into another
    *mission if *	they are sitting around in guard mode.
    */
    IsReadyToCommence = true;

    /*
    **	If there is no target available, then search for one.
    */
    if (!TarCom) {
      const ThreatType threat = THREAT_NORMAL;
      Assign_Target(Greatest_Threat(threat));
    }

    /*
    **	There is a valid target. Switch into attack mode right away.
    */
    if (TarCom) {
      Assign_Mission(MISSION_ATTACK);
      Commence();
      return 1;
    }
  } else {
    /*
    **	This is the very simple state machine that basically does
    **	nothing. This is the mode that non weapon equipped buildings
    **	are normally in.
    */
    constexpr int kInitialEntry = 0;
    constexpr int kIdle = 1;
    switch (mission_state().step()) {
      case kInitialEntry:
        Begin_Mode(game::BSTATE_IDLE);
        mission_state().set_step(kIdle);
        break;

      case kIdle:
        /*
        **	Special case to break out of guard mode if this is a repair
        **	facility and there is a customer waiting at the grease pit.
        */
        if (*this == STRUCT_REPAIR && radio().InContact() &&
            Contact_With_Whom()->Is_Techno() &&
            Contact_With_Whom()->mission_state().current() == MISSION_ENTER &&
            Distance(Contact_With_Whom()) < 0x0040 &&
            Transmit_Message(RADIO_NEED_TO_MOVE) == RADIO_ROGER) {
          Assign_Mission(MISSION_REPAIR);
          return 1;
        }
        break;

      default:
        break;
    }

    if (*this == STRUCT_REPAIR) {
      return TheRules()
                 .mission_control()
                 .at(mission_state().current())
                 .Normal_Delay() +
             RandomPick(0, 2);
    }
    return (TheRules()
                .mission_control()
                .at(mission_state().current())
                .Normal_Delay() *
            3) +
           RandomPick(0, 2);
  }
  return TheRules().mission_control().at(mission_state().current()).AA_Delay() +
         RandomPick(0, 2);
}

/***********************************************************************************************
 * BuildingClass::Mission_Construction -- Handles mission construction. *
 *                                                                                             *
 *    This routine will handle mission construction. When this mission is
 *complete, the        * building will begin normal operation. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of game frames to delay before calling this
 *routine        * again. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. *
 *=============================================================================================*/
int BuildingClass::Mission_Construction() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  constexpr int kInitial = 0;
  constexpr int kDuring = 1;
  switch (mission_state().step()) {
    case kInitial:
      Begin_Mode(game::BSTATE_CONSTRUCTION);
      Transmit_Message(RADIO_BUILDING);
      if (House->IsPlayerControl) {
        PlaySoundEffectAt(VOC_CONSTRUCTION, Coord);
      }
      mission_state().set_step(kDuring);
      break;

    case kDuring:
      if (IsReadyToCommence) {
        /*
        **	When construction is complete, then transmit this
        **	to the construction yard so that it can stop its
        **	construction animation.
        */
        Transmit_Message(RADIO_COMPLETE);  // "I'm finished."
        Transmit_Message(RADIO_OVER_OUT);  // "You're free."
        Begin_Mode(game::BSTATE_IDLE);
        Grand_Opening();
        Assign_Mission(MISSION_GUARD);
        PrimaryFacing.SnapTo(Class->StartFace);
      }
      break;

    default:
      break;
  }
  return 1;
}

/***********************************************************************************************
 * BuildingClass::Mission_Deconstruction -- Handles building deconstruction. *
 *                                                                                             *
 *    This state machine is only used when the building is deconstructing as a
 *result of       * selling.  When this mission is finished, the building will
 *no longer exist.              *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of game frames to delay before calling this
 *routine again. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. * 08/13/1995 JLB : Enable selling of
 *units on a repair bay.                                 * 08/20/1995 JLB :
 *Scatters infantry from scattered starting points.                        *
 *=============================================================================================*/
int BuildingClass::Mission_Deconstruction() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  /*
  **	Always force repair off.
  */
  Repair(0);

  constexpr int kInitial = 0;
  constexpr int kHolding = 1;
  constexpr int kDuring = 2;
  switch (mission_state().step()) {
    case kInitial:

      /*
      **	Special check for the repair bay which has the ability to sell
      **	whatever is on it. If there is something on the repair bay, then
      **	it will be sold. If there is nothing on the repair bay, then
      **	the repair bay itself will be sold.
      */
      if ((*this == STRUCT_REPAIR || *this == STRUCT_AIRSTRIP) &&
          Transmit_Message(RADIO_NEED_TO_MOVE) == RADIO_ROGER &&
          Distance(Contact_With_Whom()) < 0x0080) {
        TechnoClass* tech = Contact_With_Whom();
        Transmit_Message(RADIO_OVER_OUT);
        if (IsOwnedByPlayer) {
          Speak(VOX_UNIT_SOLD);
        }
        tech->Sell_Back(1);
        Assign_Mission(MISSION_GUARD);
        return 1;
      }

      /*
      ** Selling off a shipyard or sub pen may cause attached ships
      ** who are repairing themselves to discontinue repairs.
      */
      if (*this == STRUCT_SHIP_YARD || *this == STRUCT_SUB_PEN) {
        for (int index = 0; index < TheObjectHeaps().vessel().Count();
             index++) {
          VesselClass* obj = TheObjectHeaps().vessel().ActiveAt(index);
          if ((obj && !obj->IsInLimbo && obj->House == House) &&
              obj->IsSelfRepairing &&
              (ra::Distance(Center_Coord(), obj->Center_Coord()) < 0x0200)) {
            obj->IsSelfRepairing = false;
            obj->IsToSelfRepair = false;
          }
        }
      }

      IsReadyToCommence = false;
      Transmit_Message(RADIO_RUN_AWAY);
      mission_state().set_step(kHolding);
      break;

    case kHolding:
      if (!IsTethered) {
        /*
        **	The crew will evacuate from the building. The number of crew
        **	members leaving is equal to the unrecovered cost of the building
        **	divided by 100 (the typical cost of a minigunner infantryman).
        */
        if (!ArchiveTarget || !TheRules().IsMCVDeploy ||
            *this != STRUCT_CONST) {
          int count = How_Many_Survivors();
          bool engine = false;

          while (count) {
            /*
            **	Ensure that the player only gets ONE engineer and not from a
            *captured *	construction yard.
            */
            InfantryType typ = Crew_Type();
            while (typ == INFANTRY_RENOVATOR && engine) {
              typ = Crew_Type();
            }
            if (typ == INFANTRY_RENOVATOR) {
              engine = true;
            }

            InfantryClass* infantry = nullptr;
            if (typ != INFANTRY_NONE) {
              infantry =
                  base::NewPooled<InfantryClass>(typ, House->Class->House);
            }
            if (infantry != nullptr) {
              TheWorld().scenario_init()++;
              Coordinate coord = Center_Coord() + CoordFromPixels(0, -12);
              coord = TheMap().at(coord).Closest_Free_Spot(coord, false);

              if (infantry->Unlimbo(coord, game::Direction::kNorth)) {
                if (infantry->Class->IsNominal) {
                  infantry->IsTechnician = true;
                }
                TheWorld().scenario_init()--;
                infantry->Scatter(Coordinate{}, true);
                TheWorld().scenario_init()++;
                infantry->Assign_Mission(MISSION_GUARD_AREA);
              } else {
                base::DeletePooled(infantry);
              }
              TheWorld().scenario_init()--;
            }
            count--;
          }
        }

        if (House->IsPlayerControl) {
          PlaySoundEffectAt(VOC_CASHTURN, Coord);
        }
        mission_state().set_step(kDuring);
        Begin_Mode(game::BSTATE_CONSTRUCTION);
        Detach_All(true);
        Transmit_Message(RADIO_OVER_OUT);
        IsReadyToCommence = false;
        break;
      }
      Transmit_Message(RADIO_RUN_AWAY);
      break;

    case kDuring:
      if (IsReadyToCommence) {
        House->IsRecalcNeeded = true;
        if (IsOwnedByPlayer) {
          Speak(VOX_STRUCTURE_SOLD);
        }

        /*
        **	Construction yards that deconstruct, really just revert back
        **	to an MCV.
        */
        if (ArchiveTarget && *this == STRUCT_CONST && House->IsHuman &&
            Strength > 0) {
          TheWorld().scenario_init()++;
          auto* unit =
              base::NewPooled<UnitClass>(UNIT_MCV, House->Class->House);
          TheWorld().scenario_init()--;
          if (unit != nullptr) {
            /*
            **	Unlimbo the MCV onto the map. The MCV should start in the same
            **	health condition that the construction yard was in.
            */
            const Fixed ratio = Health_Ratio();
            const int money = Refund_Amount();
            const Target arch = ArchiveTarget;
            const Coordinate place =
                AdjacentCell(Coord, game::Direction::kSouthEast).Snapped();

            delete this;

            if (unit->Unlimbo(place, game::Direction::kSouthWest)) {
              unit->Strength =
                  static_cast<int16_t>(unit->Class_Of().MaxStrength * ratio);

              /*
              **	Lift the move destination from the building and assign
              **	it to the unit.
              */
              if (arch) {
                unit->Assign_Destination(arch);
                unit->Assign_Mission(MISSION_MOVE);
              }

            } else {
              /*
              **	If, for some strange reason, the MCV could not be placed
              *on the *	map, then give the player some money to compensate.
              */
              House->Refund_Money(money);
            }
          } else {
            House->Refund_Money(Refund_Amount());
            delete this;
          }

        } else {
          /*
          ** Selling off a gap generator will cause the cells it affects
          ** to stop being jammed.
          */
          if (*this == STRUCT_GAP) {
            Remove_Gap_Effect();
          }

          /*
          **	A sold building still counts as a kill, but it just isn't
          *directly *	attributed to the enemy.
          */
          WhoLastHurtMe = HOUSE_NONE;
          Record_The_Kill(nullptr);

          /*
          **	The player gets part of the money back for the sell.
          */
          House->Refund_Money(Refund_Amount());
          House->Stole(-Refund_Amount());
          Limbo();

          /*
          **	Finally, delete the building from the game.
          */
          delete this;
        }
      }
      break;

    default:
      break;
  }
  return 1;
}

/***********************************************************************************************
 * BuildingClass::Mission_Attack -- Handles attack mission for building. *
 *                                                                                             *
 *    Buildings that can attack are processed by this attack mission state
 *machine.            *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of game frames to delay before calling this
 *routine        * again. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. * 02/22/1996 JLB : SAM doesn't lower
 *back into ground.                                      *
 *=============================================================================================*/
int BuildingClass::Mission_Attack() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (*this == STRUCT_SAM) {
    switch (mission_state().step()) {
      /*
      **	This is the target tracking state of the launcher. It will
      *rotate *	to face the current TarCom of the launcher.
      */
      case kSamReady:
        if ((Class->IsPowered && House->Power_Fraction() < 1) || IsJammed) {
          return 1;
        }
        if (!TarCom || TarCom.kind() != RTTI_AIRCRAFT ||
            TarCom.AsAircraft()->Height == 0) {
          Assign_Target(Target{});
          mission_state().set_step(kSamReady);
          Assign_Mission(MISSION_GUARD);
          Commence();
          return 1;
        }
        if (!PrimaryFacing.IsRotating()) {
          const game::Direction facing = DirectionTo(TarCom);
          if (PrimaryFacing.current() != facing) {
            PrimaryFacing.set_desired(facing);
          } else {
            mission_state().set_step(kSamFiring);
          }
        }
        return 1;

      /*
      **	The launcher is in the process of firing.
      */
      case kSamFiring:
        if (!TarCom || TarCom.kind() != RTTI_AIRCRAFT ||
            TarCom.AsAircraft()->Height == 0) {
          Assign_Target(Target{});
          mission_state().set_step(kSamReady);
        } else {
          const game::FireErrorType error = Can_Fire(TarCom, 0);
          if (error == game::FIRE_ILLEGAL || error == game::FIRE_CANT ||
              error == game::FIRE_RANGE) {
            Assign_Target(Target{});
            mission_state().set_step(kSamReady);
          } else {
            if (error == game::FIRE_FACING) {
              mission_state().set_step(kSamReady);
            } else {
              if (error == game::FIRE_OK) {
                Fire_At(TarCom, 0);
                Fire_At(TarCom, 1);
                mission_state().set_step(kSamReady);
              }
            }
          }
        }
        return 1;

      default:
        break;
    }
    return TheRules()
               .mission_control()
               .at(mission_state().current())
               .AA_Delay() +
           RandomPick(0, 2);
  }

  if (!TarCom) {
    Assign_Target(Target{});
    Assign_Mission(MISSION_GUARD);
    Commence();
    return 1;
  }

  const int primary = What_Weapon_Should_I_Use(TarCom);
  IsReadyToCommence = true;
  switch (Can_Fire(TarCom, primary)) {
    case game::FIRE_ILLEGAL:
    case game::FIRE_CANT:
    case game::FIRE_RANGE:
    case game::FIRE_AMMO:
      Assign_Target(Target{});
      Assign_Mission(MISSION_GUARD);
      Commence();
      break;

    case game::FIRE_FACING:
      PrimaryFacing.set_desired(DirectionTo(TarCom));
      return 2;

    case game::FIRE_REARM:
      PrimaryFacing.set_desired(DirectionTo(TarCom));
      return static_cast<int>(Arm.Remaining());

    case game::FIRE_BUSY:
      return 1;

    case game::FIRE_CLOAKED:
      Do_Uncloak();
      break;

    case game::FIRE_OK:
      Fire_At(TarCom, primary);
      return 1;

    case game::FireErrorType::FIRE_ROTATING:
    case game::FireErrorType::FIRE_MOVING:
    default:
      break;
  }
  PrimaryFacing.set_desired(DirectionTo(TarCom));
  return 1;
  //	return(MissionControl[Mission].Normal_Delay() + RandomPick(0, 2));
}

/***********************************************************************************************
 * BuildingClass::Mission_Harvest -- Handles refinery unloading harvesters. *
 *                                                                                             *
 *    This state machine handles the refinery when it unloads the harvester. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of game frames to delay before calling this
 *routine        * again. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. *
 *=============================================================================================*/
int BuildingClass::Mission_Harvest() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  constexpr int kInitial = 0;
  constexpr int kWaitForDock = 1;    // Waiting for docking to complete.
  constexpr int kMiddle = 2;         // Offload "bails" of tiberium.
  constexpr int kWaitForUndock = 3;  // Waiting for undocking to complete.
  switch (mission_state().step()) {
    case kInitial:
      mission_state().set_step(kWaitForDock);
      break;

    case kWaitForDock:
      if (IsReadyToCommence) {
        IsReadyToCommence = false;
        mission_state().set_step(kMiddle);
      }
      break;

    case kMiddle:
      if (IsReadyToCommence) {
        IsReadyToCommence = false;

        /*
        **	Force any bib squatters to scatter.
        */
        TheMap()
            .at(AdjacentCell(Cell::Containing(Center_Coord()),
                             game::Direction::kSouth))
            .Incoming(Coordinate{}, true, true);

        FootClass* techno = cargo().Attached_Object();
        if (techno) {
          const int bail = techno->Offload_Tiberium_Bail();

          if (bail) {
            House->Harvested(bail);
            if (techno->Tiberium_Load() > 0) {
              return 1;
            }
          }
        }
        mission_state().set_step(kWaitForUndock);
      }
      break;

    case kWaitForUndock:
      if (IsReadyToCommence) {
        /*
        **	Detach harvester and go back into idle state.
        */
        Assign_Mission(MISSION_GUARD);
      }
      break;

    default:
      break;
  }
  return 1;
}

/***********************************************************************************************
 * BuildingClass::Mission_Repair -- Handles the repair (active) state for
 *building.            *
 *                                                                                             *
 *    This state machine is used when the building is active in some sort of
 *repair or         * construction mode. The construction yard will animate. The
 *repair facility will repair   * anything that it docked on it. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of game frames to delay before calling this
 *routine again. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. * 06/25/1995 JLB : Handles repair
 *facility                                                  * 07/29/1995 JLB :
 *Repair rate is controlled by power rating.                               *
 *=============================================================================================*/
int BuildingClass::Mission_Repair() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (*this == STRUCT_CONST) {
    constexpr int kInitial = 0;
    constexpr int kDuring = 1;
    switch (mission_state().step()) {
      case kInitial:
        Begin_Mode(game::BSTATE_ACTIVE);
        mission_state().set_step(kDuring);
        break;

      case kDuring:
        if (!radio().InContact()) {
          Assign_Mission(MISSION_GUARD);
        }
        break;

      default:
        break;
    }
    return 1;
  }

  if (*this == STRUCT_REPAIR) {
    constexpr int kInitial = 0;
    constexpr int kIdle = 1;
    constexpr int kDuring = 2;
    switch (mission_state().step()) {
      case kInitial: {
        if (!radio().InContact()) {
          Begin_Mode(game::BSTATE_IDLE);
          Assign_Mission(MISSION_GUARD);
          return 1;
        }
        IsReadyToCommence = false;
        int distance = 0x10;
        TechnoClass* tech = Contact_With_Whom();

        /*
        ** BG: If the unit to repair is an aircraft, and the aircraft is
        ** fixed-wing, and it's landed, be much more liberal with the
        ** distance check.  Fixed-wing aircraft are very inaccurate with
        ** their landings.
        */
        if ((tech->What_Am_I() == RTTI_AIRCRAFT) &&
            (dynamic_cast<AircraftClass*>(tech)->Class->IsFixedWing &&
             dynamic_cast<AircraftClass*>(tech)->In_Which_Layer() ==
                 LAYER_GROUND)) {
          distance = 0x80;
        }

        if (Transmit_Message(RADIO_NEED_TO_MOVE) == RADIO_ROGER &&
            Distance(Contact_With_Whom()) < distance) {
          mission_state().set_step(kIdle);
          return kTicksPerSecond / 4;
        }
        break;
      }

      case kIdle:
        if (!radio().InContact()) {
          Assign_Mission(MISSION_GUARD);
          return 1;
        }

        if (Transmit_Message(RADIO_NEED_TO_MOVE) == RADIO_ROGER) {
          TechnoClass* radio = Contact_With_Whom();

          if ((radio->Health_Ratio() < TheRules().ConditionGreen ||
               (radio->What_Am_I() == RTTI_UNIT &&
                *dynamic_cast<UnitClass*>(radio) == UNIT_MINELAYER)) &&
              Transmit_Message(RADIO_REPAIR) == RADIO_ROGER) {
            /*
            **	If the object over the repair bay is marked as useless, then
            **	sell it back to get some money.
            */
            if (radio->IsUseless) {
              if (!radio->House->IsHuman) {
                radio->Sell_Back(1);
              }
              mission_state().set_step(kInitial);
              IsReadyToCommence = true;
            } else {
              if (IsOwnedByPlayer) {
                Speak(VOX_REPAIRING);
              }
              mission_state().set_step(kDuring);
              Begin_Mode(game::BSTATE_ACTIVE);
              IsReadyToCommence = false;
            }
          } else {
            //						Transmit_Message(RADIO_RUN_AWAY);
            ///*BG*/					if(radio->Health_Ratio()
            ///>= Rule.ConditionGreen) {
            //								Transmit_Message(RADIO_RUN_AWAY);
            //							}
          }
        }
        break;

      case kDuring:
        if (!radio().InContact()) {
          Begin_Mode(game::BSTATE_IDLE);
          mission_state().set_step(kIdle);
          return 1;
        }

        /*
        **	Check to see if the repair light blink has completed and the
        *attached *	unit is not doing something else. If these conditions
        *are favorable, *	the repair can proceed another step.
        */
        if (IsReadyToCommence &&
            Transmit_Message(RADIO_NEED_TO_MOVE) == RADIO_ROGER) {
          IsReadyToCommence = false;

          /*
          **	Tell the attached unit to repair one step. It will respond with
          *how *	it fared.
          */
          switch (Transmit_Message(RADIO_REPAIR)) {
            /*
            **	The repair step proceeded smoothly. Proceed normally with the
            **	repair process.
            */
            case RADIO_ROGER:
              break;

            /*
            **	The repair operation was aborted because of some reason. Presume
            **	that the reason is because of low cash.
            */
            case RADIO_CANT:
              if (IsOwnedByPlayer) {
                Speak(VOX_NO_CASH);
              }
              Begin_Mode(game::BSTATE_IDLE);
              mission_state().set_step(kIdle);
              break;

            /*
            **	The repair step resulted in a completely repaired unit.
            */
            case RADIO_ALL_DONE:
              if (IsOwnedByPlayer) {
                Speak(VOX_UNIT_REPAIRED);
              }
              //							Transmit_Message(RADIO_RUN_AWAY);
              Begin_Mode(game::BSTATE_IDLE);
              mission_state().set_step(kIdle);
              break;

            /*
            **	The repair step could not be completed because this unit is
            *already *	at full strength.
            */
            case RADIO_NEGATIVE:
            case RadioMessageType::RADIO_STATIC:
            case RadioMessageType::RADIO_HELLO:
            case RadioMessageType::RADIO_OVER_OUT:
            case RadioMessageType::RADIO_PICK_UP:
            case RadioMessageType::RADIO_ATTACH:
            case RadioMessageType::RADIO_DELIVERY:
            case RadioMessageType::RADIO_HOLD_STILL:
            case RadioMessageType::RADIO_UNLOADED:
            case RadioMessageType::RADIO_UNLOAD:
            case RadioMessageType::RADIO_BUILDING:
            case RadioMessageType::RADIO_COMPLETE:
            case RadioMessageType::RADIO_REDRAW:
            case RadioMessageType::RADIO_DOCKING:
            case RadioMessageType::RADIO_CAN_LOAD:
            case RadioMessageType::RADIO_ARE_REFINERY:
            case RadioMessageType::RADIO_TRYING_TO_LOAD:
            case RadioMessageType::RADIO_MOVE_HERE:
            case RadioMessageType::RADIO_NEED_TO_MOVE:
            case RadioMessageType::RADIO_YEA_NOW_WHAT:
            case RadioMessageType::RADIO_IM_IN:
            case RadioMessageType::RADIO_BACKUP_NOW:
            case RadioMessageType::RADIO_RUN_AWAY:
            case RadioMessageType::RADIO_TETHER:
            case RadioMessageType::RADIO_UNTETHER:
            case RadioMessageType::RADIO_REPAIR:
            case RadioMessageType::RADIO_PREPARED:
            case RadioMessageType::RADIO_ATTACK_THIS:
            case RadioMessageType::RADIO_RELOAD:
            case RadioMessageType::RADIO_NEED_REPAIR:
            case RadioMessageType::RADIO_ON_DEPOT:
            default:
              //							Transmit_Message(RADIO_RUN_AWAY);
              Begin_Mode(game::BSTATE_IDLE);
              mission_state().set_step(kIdle);
              break;
          }
        }
        return 1;

      default:
        break;
    }
    return TheRules()
        .mission_control()
        .at(mission_state().current())
        .Normal_Delay();
  }

  if (*this == STRUCT_HELIPAD || *this == STRUCT_AIRSTRIP) {
    constexpr int kInitial = 0;
    constexpr int kDuring = 1;
    switch (mission_state().step()) {
      case kInitial:
        if (Transmit_Message(RADIO_NEED_TO_MOVE) == RADIO_ROGER &&
            Transmit_Message(RADIO_PREPARED) == RADIO_NEGATIVE) {
          Begin_Mode(game::BSTATE_ACTIVE);
          Contact_With_Whom()->Assign_Mission(MISSION_SLEEP);
          mission_state().set_step(kDuring);
          return 1;
        }
        Assign_Mission(MISSION_GUARD);
        break;

      case kDuring:
        if (IsReadyToCommence) {
          if (!radio().InContact() ||
              Transmit_Message(RADIO_NEED_TO_MOVE) == RADIO_NEGATIVE) {
            Assign_Mission(MISSION_GUARD);
            return 1;
          }

          if (Transmit_Message(RADIO_PREPARED) == RADIO_ROGER) {
            Contact_With_Whom()->Assign_Mission(MISSION_GUARD);
            Assign_Mission(MISSION_GUARD);
            return 1;
          }

          if (Transmit_Message(RADIO_RELOAD) != RADIO_ROGER) {
            Assign_Mission(MISSION_GUARD);
            Contact_With_Whom()->Assign_Mission(MISSION_GUARD);
            return 1;
          }
          Fixed pfrac = House->Power_Fraction();
          pfrac.Saturate(1);
          if (pfrac < Fixed::kHalf) {
            pfrac = Fixed::kHalf;
          }
          const int time =
              pfrac.Invert() * TheRules().ReloadRate * kTicksPerMinute;
          IsReadyToCommence = false;
          return time;
        }
        break;

      default:
        break;
    }
    return 3;
  }
  return kTicksPerSecond;
}

/***********************************************************************************************
 * BuildingClass::Mission_Missile -- State machine for nuclear missile launch. *
 *                                                                                             *
 *    This handles the Temple of Nod launching its nuclear missile. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of frames to delay before calling this
 *routine again.      *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/04/1995 JLB : Commented. *
 *=============================================================================================*/
int BuildingClass::Mission_Missile() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (*this == STRUCT_ADVANCED_TECH) {
    // Stages 2 (satellite deploy) and 3 (done) have no handling.
    constexpr int kDoorOpening = 0;
    constexpr int kLaunchUp = 1;

    switch (mission_state().step()) {
      /*
      ** The initial case is responsible for starting the door
      ** opening on the building, the missile rising, and smoke broiling.
      */
      case kDoorOpening: {
        const Coordinate door =
            Coord_Move(Center_Coord(), game::Direction(0xC0), 0x30);
        const auto* sput = base::NewPooled<AnimClass>(ANIM_SPUTDOOR, door);
        if (sput) {
          IsReadyToCommence = false;
          mission_state().set_step(kLaunchUp);
          AnimToTrack = sput->As_Target();
        }
      }
        [[fallthrough]];

      /*
      ** Once the smoke has been going for a little while this
      ** actually handles launching the missile into the air.
      */
      case kLaunchUp: {
        const AnimClass* sput = AnimToTrack.AsAnimation();
        if (sput && (sput->animation().Fetch_Stage() >= 19)) {
          const Cell center = Cell::Containing(Center_Coord());
          const Cell cell = Cell::FromXY(center.x(), 1);
          const Target targ = Target::FromCell(cell);

          auto* bullet =
              base::NewPooled<BulletClass>(BULLET_GPS_SATELLITE, targ, this,
                                           200, WARHEAD_FIRE, game::kMphRocket);
          if (bullet) {
            const Coordinate launch =
                Coord_Move(Center_Coord(), game::Direction(0xC0), 0x30);
            if (!bullet->Unlimbo(launch, game::Direction::kNorth)) {
              base::DeletePooled(bullet);
              bullet = nullptr;
            }
          }

          if (bullet) {
            Assign_Mission(MISSION_GUARD);
          }
        }
      }
        return 1;
      default:
        break;
    }
  }

  if (*this == STRUCT_MSLO) {
    constexpr int kInitial = 0;
    constexpr int kDoorOpening = 1;
    constexpr int kLaunchUp = 2;
    constexpr int kLaunchDown = 3;
    constexpr int kDoneLaunch = 4;

    switch (mission_state().step()) {
      /*
      ** The initial case is responsible for starting the door
      ** opening on the building.
      */
      case kInitial:
        IsReadyToCommence = false;
        Begin_Mode(game::BSTATE_ACTIVE);  // open the door
        mission_state().set_step(kDoorOpening);
        return 1;

      /*
      ** This polls for the case when the door is actually open and
      ** then kicks off the missile smoke.
      */
      case kDoorOpening:
        if (IsReadyToCommence) {
          Begin_Mode(game::BSTATE_AUX1);  // hold the door open
          mission_state().set_step(kLaunchUp);
          return 14;
        }
        return 1;

      /*
      ** Once the smoke has been going for a little while this
      ** actually handles launching the missile into the air.
      */
      case kLaunchUp: {
        const Cell center = Cell::Containing(Center_Coord());
        const Cell cell = Cell::FromXY(center.x(), 1);
        const Target targ = Target::FromCell(cell);
        auto* bullet = base::NewPooled<BulletClass>(
            BULLET_NUKE_UP, targ, this, 200, WARHEAD_HE, game::kMphVeryFast);
        if (bullet) {
          const Coordinate launch =
              Coord_Move(Center_Coord(), game::Direction(28), 0xA0);
          if (!bullet->Unlimbo(launch, game::Direction::kNorth)) {
            base::DeletePooled(bullet);
            bullet = nullptr;
          }
        }

        if (bullet) {
          Speak(VOX_ABOMB_LAUNCH);
          mission_state().set_step(kLaunchDown);
          /*
          ** Hack: If it's the artificial nukes, don't let the bullets come down
          *(as
          ** they're the only ones that blow up).  We know it's artificial if
          *you're
          ** at tech level 10 or below, because you can't build the nuclear silo
          *until
          ** tech level 15 or so.
          */
          if (House->Control.TechLevel <= 10) {
            return 6;
          }
          bullet = base::NewPooled<BulletClass>(
              BULLET_NUKE_DOWN, Target::FromCell(House->NukeDest), this, 200,
              WARHEAD_NUKE, game::kMphVeryFast);
          if (bullet) {
            const int celly = std::max(House->NukeDest.y() - 64, 1);
            const Coordinate start =
                Cell::FromXY(House->NukeDest.x(), celly).Center();
            if (!bullet->Unlimbo(start, game::Direction::kSouth)) {
              base::DeletePooled(bullet);
            }
          }
          return 8 * kTicksPerSecond;
        }
      }
        return 1;

      /*
      ** Once the missile is in the air, this handles waiting for
      ** the missile to be off the screen and then launching one down
      ** over the target.
      */
      case kLaunchDown: {
        Begin_Mode(game::BSTATE_AUX2);  // start the door closing
        mission_state().set_step(kDoneLaunch);
        return 6;
      }

      /*
      ** Once the missile is done launching this handles allowing
      ** the building to sit there with its door closed.
      */
      case kDoneLaunch:
        Begin_Mode(game::BSTATE_IDLE);  // keep the door closed.
        Assign_Mission(MISSION_GUARD);
        return 60;
      default:
        break;
    }
  }
  return TheRules()
      .mission_control()
      .at(mission_state().current())
      .Normal_Delay();
}

/***********************************************************************************************
 * BuildingClass::Revealed -- Reveals the building to the specified house. *
 *                                                                                             *
 *    This routine will reveal the building to the specified house. It will
 *handle updating    * the sidebar for player owned buildings. A player owned
 *building that hasn't been         * revealed, is in a state of pseudo-limbo.
 *It cannot be used for any of its special        * abilities even though it
 *exists on the map for all other purposes.                       *
 *                                                                                             *
 * INPUT:   house -- The house that this building is being revealed to. *
 *                                                                                             *
 * OUTPUT:  Was this building revealed by this procedure? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. *
 *=============================================================================================*/
bool BuildingClass::Revealed(HouseClass* house) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (TechnoClass::Revealed(house)) {
    if (!TheWorld().scenario_init()) {
      House->JustBuiltStructure = Class->Type;
      House->IsBuiltSomething = true;
    }
    House->IsRecalcNeeded = true;

    /*
    **	Perform any grand opening here so that in the scenarios where a player
    **	owned house is not yet revealed, it won't be reflected in the sidebar
    **	selection icons.
    */
    if (!radio().InContact() && House->IsHuman &&
        mission_state().current() != MISSION_CONSTRUCTION) {
      Grand_Opening();
    } else {
      if (!radio().InContact() && !House->IsHuman && house == House &&
          mission_state().current() != MISSION_CONSTRUCTION) {
        Grand_Opening();
      }
    }

    return true;
  }
  return false;
}

/***********************************************************************************************
 * BuildingClass::Enter_Idle_Mode -- The building will enter its idle mode. *
 *                                                                                             *
 *    This routine is called when the exact mode of the building isn't known. By
 *examining     * the building's condition, this routine will assign an
 *appropriate mission.               *
 *                                                                                             *
 * INPUT:   initial  -- This this being called during scenario init? *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Enter_Idle_Mode(bool initial) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  /*
  **	Assign an appropriate mission for the building. If the ScenarioInit flag
  *is true, then *	this must be an initial building. Start such buildings
  *in idle state. For other buildings *	it indicates that it is being placed
  *during game play and thus it must start in *	the "construction" mission.
  */
  MissionType mission = MISSION_GUARD;

  if (!initial || TheWorld().scenario_init() ||
      TheDebugState().map_editor_active()) {
    Begin_Mode(game::BSTATE_IDLE);
    mission = MISSION_GUARD;
  } else {
    Begin_Mode(game::BSTATE_CONSTRUCTION);
    mission = MISSION_CONSTRUCTION;
  }
  Assign_Mission(mission);
}

/***********************************************************************************************
 * BuildingClass::Pip_Count -- Determines "full" pips to display for building. *
 *                                                                                             *
 *    This routine will determine the number of pips that should be filled in
 *when rendering   * the building. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns the number of pips to display as filled in. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/28/1995 JLB : Created. *
 *=============================================================================================*/
int BuildingClass::Pip_Count() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  return Class->Max_Pips() * House->Tiberium_Fraction();
}

/***********************************************************************************************
 * BuildingClass::Death_Announcement -- Announce the death of this building. *
 *                                                                                             *
 *    This routine is called when the building is destroyed by "unnatural"
 *means. Typically    * as a result of combat. If the building is known to the
 *player, then it should be         * announced. *
 *                                                                                             *
 * INPUT:   source   -- The object most directly responsible for the building's
 *death.         *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/04/1995 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Death_Announcement(const TechnoClass* source) const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (source != nullptr && House->IsPlayerControl) {
    Speak(VOX_STRUCTURE_DESTROYED);
  }
}

/***********************************************************************************************
 * BuildingClass::Fire_Direction -- Fetches the direction of firing. *
 *                                                                                             *
 *    This routine will return with the default direction to use when firing
 *from this         * building. This is the facing of the turret except for the
 *case of non-turret equipped    * buildings that have a weapon (e.g., guard
 *tower).                                        *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the default firing direction for this building. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/04/1995 JLB : Created. *
 *=============================================================================================*/
game::Direction BuildingClass::Fire_Direction() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (Class->IsTurretEquipped) {
    return PrimaryFacing.current();
  }
  return game::Direction(TarCom.packed());
}

/***********************************************************************************************
 * BuildingClass::Mission_Unload -- Handles the unload mission for a building. *
 *                                                                                             *
 *    This is the unload mission for a building. This really only applies to the
 *weapon's      * factory, since it needs the sophistication of an unload
 *mission due to the door          * animation. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of game frames to delay before calling this
 *routine        * again. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/29/1995 JLB : Created. *
 *=============================================================================================*/
int BuildingClass::Mission_Unload() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (*this == STRUCT_WEAP) {
    const Cell cell =
        Cell::Containing(Coord).Offset(base::At(Class->ExitList, 0));
    const Coordinate coord = cell.Center();
    CellClass* cellptr = &TheMap().at(cell);
    constexpr int kInitial = 0;
    constexpr int kClearBib = 1;
    constexpr int kOpen = 2;
    constexpr int kLeave = 3;
    constexpr int kClose = 4;
    constexpr int kDoorFrames = 4;
    constexpr int kDoorRate = 8;
    UnitClass* unit = nullptr;
    switch (mission_state().step()) {
      /*
      **	Start the door opening.
      */
      case kInitial:
        //				if (cellptr->Cell_Techno()) {
        //					cellptr->Incoming(0, true);
        //				}
        unit = dynamic_cast<UnitClass*>(Contact_With_Whom());
        if (unit) {
          unit->Assign_Mission(MISSION_GUARD);
          unit->Commence();
        }
        door().Open(kDoorRate, kDoorFrames);
        mission_state().set_step(kClearBib);
        break;

      /*
      **	Now that the occupants can peek out the door, they will tell
      **	everyone that could be blocking the way, that they should
      **	scatter away.
      */
      case kClearBib:
        if (cellptr->Cell_Techno()) {
          cellptr->Incoming(Coordinate{}, true, true);

          /*
          **	Scatter everything around the weapon's factory door.
          */
          for (const game::Facing f : magic_enum::enum_values<game::Facing>()) {
            CellClass* cptr = &cellptr->Adjacent_Cell(f);
            if (cptr->Cell_Building() == nullptr) {
              cptr->Incoming(coord, true, true);
            }
          }
        } else {
          mission_state().set_step(kOpen);
        }
        break;

      /*
      **	When the door is finally open and the way is clear, tell the
      **	unit to drive out.
      */
      case kOpen:
        if (door().IsOpen()) {
          unit = dynamic_cast<UnitClass*>(Contact_With_Whom());
          if (unit) {
            unit->Assign_Mission(MISSION_MOVE);

            if (House->IQ >= TheRules().IQGuardArea) {
              unit->Assign_Mission(MISSION_GUARD_AREA);
              unit->ArchiveTarget = Target::FromCell(House->Where_To_Go(unit));
            }
            unit->Force_Track(DriveClass::kOutOfWeaponFactory, coord);
            //						unit->Force_Track(DriveClass::kOutOfWeaponFactory,
            // Adjacent_Cell(Adjacent_Cell(Center_Coord(), Facing::kSouth),
            // Facing::kSouth));
            unit->Set_Speed(128);
            mission_state().set_step(kLeave);
          } else {
            door().Close(kDoorRate, kDoorFrames);
            mission_state().set_step(kClose);
          }
        }
        break;

      /*
      **	Wait until the unit has completely left the building.
      */
      case kLeave:
        if (!IsTethered) {
          door().Close(kDoorRate, kDoorFrames);
          mission_state().set_step(kClose);
        } else {
          //					if (radio().InContact() &&
          //!((FootClass *)Contact_With_Whom())->IsDriving) {
          //						Transmit_Message(RADIO_OVER_OUT);
          //					}
        }
        break;

      /*
      **	Wait while the door closes.
      */
      case kClose:
        if (door().IsClosed()) {
          Enter_Idle_Mode();
        }
        break;

      default:
        break;
    }
    return TheRules()
               .mission_control()
               .at(mission_state().current())
               .Normal_Delay() +
           RandomPick(0, 2);
  }

  Assign_Mission(MISSION_GUARD);
  return 1;
}

/***********************************************************************************************
 * BuildingClass::Power_Output -- Fetches the current power output from this
 *building.         *
 *                                                                                             *
 *    This routine will return the current power output for this building. The
 *power output    * is adjusted according to the damage level of the building. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns the current power output for this building. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/29/1995 JLB : Created. *
 *=============================================================================================*/
int BuildingClass::Power_Output() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (Class->Power) {
    return Class->Power * Fixed(LastStrength, Class->MaxStrength);
  }
  return 0;
}

/***********************************************************************************************
 * BuildingClass::Detach -- Handles target removal from the game system. *
 *                                                                                             *
 *    This routine is called when the specified target is about to be removed
 *from the game    * system. *
 *                                                                                             *
 * INPUT:   target   -- The target to be removed from this building's targeting
 *computer.      *
 *                                                                                             *
 *          all      -- Is the target about to be completely eliminated? *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/29/1995 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Detach(Target target, bool all) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  TechnoClass::Detach(target, all);
  if (target == WhomToRepay) {
    WhomToRepay = Target{};
  }
  if (target == AnimToTrack) {
    AnimToTrack = Target{};
  }
}

/***********************************************************************************************
 * BuildingClass::Crew_Type -- This determines the crew that this object
 *generates.            *
 *                                                                                             *
 *    When selling very cheap buildings (such as the silo), a technician will
 *pop out since    * generating minigunners would be overkill -- the player
 *could use this loophole to        * gain an advantage. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns the infantry type that this building will generate as a
 *survivor.          *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/05/1995 JLB : Created. *
 *=============================================================================================*/
InfantryType BuildingClass::Crew_Type() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  switch (Class->Type) {
    case STRUCT_STORAGE:
      if (PercentChance(50)) {
        return INFANTRY_C1;
      }
      return INFANTRY_C7;

    case STRUCT_CONST:
      if (!IsCaptured && House->IsHuman && PercentChance(25)) {
        return INFANTRY_RENOVATOR;
      }
      break;

    case STRUCT_KENNEL:
      if (PercentChance(50)) {
        return INFANTRY_DOG;
      }
      return INFANTRY_NONE;

    case STRUCT_TENT:
    case STRUCT_BARRACKS:
      return INFANTRY_E1;

    case StructType::STRUCT_NONE:
    case StructType::STRUCT_ADVANCED_TECH:
    case StructType::STRUCT_IRON_CURTAIN:
    case StructType::STRUCT_WEAP:
    case StructType::STRUCT_CHRONOSPHERE:
    case StructType::STRUCT_PILLBOX:
    case StructType::STRUCT_CAMOPILLBOX:
    case StructType::STRUCT_RADAR:
    case StructType::STRUCT_GAP:
    case StructType::STRUCT_TURRET:
    case StructType::STRUCT_AAGUN:
    case StructType::STRUCT_FLAME_TURRET:
    case StructType::STRUCT_REFINERY:
    case StructType::STRUCT_HELIPAD:
    case StructType::STRUCT_SAM:
    case StructType::STRUCT_AIRSTRIP:
    case StructType::STRUCT_POWER:
    case StructType::STRUCT_ADVANCED_POWER:
    case StructType::STRUCT_SOVIET_TECH:
    case StructType::STRUCT_HOSPITAL:
    case StructType::STRUCT_REPAIR:
    case StructType::STRUCT_BIO_LAB:
    case StructType::STRUCT_MISSION:
    case StructType::STRUCT_SHIP_YARD:
    case StructType::STRUCT_SUB_PEN:
    case StructType::STRUCT_MSLO:
    case StructType::STRUCT_FORWARD_COM:
    case StructType::STRUCT_TESLA:
    case StructType::STRUCT_FAKEWEAP:
    case StructType::STRUCT_FAKECONST:
    case StructType::STRUCT_FAKE_YARD:
    case StructType::STRUCT_FAKE_PEN:
    case StructType::STRUCT_FAKE_RADAR:
    case StructType::STRUCT_SANDBAG_WALL:
    case StructType::STRUCT_CYCLONE_WALL:
    case StructType::STRUCT_BRICK_WALL:
    case StructType::STRUCT_BARBWIRE_WALL:
    case StructType::STRUCT_WOOD_WALL:
    case StructType::STRUCT_FENCE:
    case StructType::STRUCT_AVMINE:
    case StructType::STRUCT_APMINE:
    case StructType::STRUCT_V01:
    case StructType::STRUCT_V02:
    case StructType::STRUCT_V03:
    case StructType::STRUCT_V04:
    case StructType::STRUCT_V05:
    case StructType::STRUCT_V06:
    case StructType::STRUCT_V07:
    case StructType::STRUCT_V08:
    case StructType::STRUCT_V09:
    case StructType::STRUCT_V10:
    case StructType::STRUCT_V11:
    case StructType::STRUCT_V12:
    case StructType::STRUCT_V13:
    case StructType::STRUCT_V14:
    case StructType::STRUCT_V15:
    case StructType::STRUCT_V16:
    case StructType::STRUCT_V17:
    case StructType::STRUCT_V18:
    case StructType::STRUCT_PUMP:
    case StructType::STRUCT_V20:
    case StructType::STRUCT_V21:
    case StructType::STRUCT_V22:
    case StructType::STRUCT_V23:
    case StructType::STRUCT_V24:
    case StructType::STRUCT_V25:
    case StructType::STRUCT_V26:
    case StructType::STRUCT_V27:
    case StructType::STRUCT_V28:
    case StructType::STRUCT_V29:
    case StructType::STRUCT_V30:
    case StructType::STRUCT_V31:
    case StructType::STRUCT_V32:
    case StructType::STRUCT_V33:
    case StructType::STRUCT_V34:
    case StructType::STRUCT_V35:
    case StructType::STRUCT_V36:
    case StructType::STRUCT_V37:
    case StructType::STRUCT_BARREL:
    case StructType::STRUCT_BARREL3:
    case StructType::STRUCT_QUEEN:
    case StructType::STRUCT_LARVA1:
    case StructType::STRUCT_LARVA2:
    default:
      break;
  }
  return TechnoClass::Crew_Type();
}

/***********************************************************************************************
 * BuildingClass::Detach_All -- Possibly abandons production according to
 *factory type.        *
 *                                                                                             *
 *    When this routine is called, it indicates that the building is about to be
 *destroyed     * or captured. In such a case any production it may be doing,
 *must be abandoned.           *
 *                                                                                             *
 * INPUT:   all   -- Is the object about the be completely destroyed? *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/05/1995 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Detach_All(bool all) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  /*
  **	If it is producing something, then it must be abandoned.
  */
  if (Factory) {
    Factory->Abandon();
    delete Factory;
    Factory = nullptr;
  }

  /*
  ** If the owner HouseClass is building something, and this building can
  ** build that thing, we may be the last building for that house that can
  ** build that thing; if so, abandon production of it.
  */
  if (House) {
    const FactoryClass* factory = House->Fetch_Factory(Class->ToBuild);

    /*
    **	If a factory was found, then temporarily disable this building and then
    **	determine if any object that is being produced can still be produced. If
    **	not, then the object being produced must be abandoned.
    */
    if (factory) {
      const TechnoClass* object = factory->Get_Object();
      IsInLimbo = true;
      if (object && !object->Techno_Type_Class()->Who_Can_Build_Me(
                        true, false, House->Class->House)) {
        House->Abandon_Production(Class->ToBuild);
      }
      IsInLimbo = false;
    }
  }

  TechnoClass::Detach_All(all);
}

/***********************************************************************************************
 * BuildingClass::Flush_For_Placement -- Handles clearing a zone for object
 *placement.         *
 *                                                                                             *
 *    This routine is used to clear the way for placement of the specified
 *object (usually     * a building). If there are friendly units blocking the
 *placement area, they are told      * to scatter. Enemy blocking units are
 *attacked.                                           *
 *                                                                                             *
 * INPUT:   techno   -- Pointer to the object that is desired to be placed. *
 *                                                                                             *
 *          cell     -- The cell that placement wants to occur at. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/06/1995 JLB : Created. * 09/27/1995 JLB : Revised to use type
 *class function.                                      *
 *=============================================================================================*/
bool BuildingClass::Flush_For_Placement(TechnoClass* techno, Cell cell) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (techno) {
    return dynamic_cast<const BuildingTypeClass&>(techno->Class_Of())
        .Flush_For_Placement(cell, House);
  }
  return false;
}

/***********************************************************************************************
 * BuildingClass::Find_Exit_Cell -- Find a clear location to exit an object from
 *this building *
 *                                                                                             *
 *    This routine is called when the building needs to discharge a unit. It
 *will find a       * nearby (adjacent) cell that is clear enough for the
 *specified object to enter. Typical   * use of this routine is when the
 *airfield disgorges its cargo.                            *
 *                                                                                             *
 * INPUT:   techno   -- Pointer to the object that wishes to exit this building.
 **
 *                                                                                             *
 * OUTPUT:  Returns with the cell number to use for object placement. If no free
 *location      * could be found, then zero (0) is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/21/1995 JLB : Created. * 02/20/1996 JLB : Added default case
 *for exit cell calculation.                            *
 *=============================================================================================*/
Cell BuildingClass::Find_Exit_Cell(const TechnoClass* techno) const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  const Cell origin = Cell::Containing(Coord);

  std::span<const int16_t> ptr = Class->ExitList;
  if (!ptr.empty()) {
    while (ptr.front() != kRefreshEol) {
      const Cell cell = origin.Offset(base::ConsumeFront(ptr));
      if (TheMap().In_Radar(cell) &&
          techno->Can_Enter_Cell(cell) == game::MOVE_OK) {
        return cell;
      }
    }
  } else {
    int x1 = 0;
    Cell cell{};

    int y1 = -1;
    const int y2 = Class->Height();
    for (x1 = -1; x1 <= Class->Width(); x1++) {
      cell = origin.Offset(x1 + (y1 * MAP_CELL_W));
      if (TheMap().In_Radar(cell) &&
          techno->Can_Enter_Cell(cell) == game::MOVE_OK) {
        return cell;
      }
      cell = origin.Offset(x1 + (y2 * MAP_CELL_W));
      if (TheMap().In_Radar(cell) &&
          techno->Can_Enter_Cell(cell) == game::MOVE_OK) {
        return cell;
      }
    }

    x1 = -1;
    const int x2 = Class->Width();
    for (y1 = -1; y1 <= Class->Height(); y1++) {
      cell = origin.Offset((y1 * MAP_CELL_W) + x1);
      if (TheMap().In_Radar(cell) &&
          techno->Can_Enter_Cell(cell) == game::MOVE_OK) {
        return cell;
      }
      cell = origin.Offset((y1 * MAP_CELL_W) + x2);
      if (TheMap().In_Radar(cell) &&
          techno->Can_Enter_Cell(cell) == game::MOVE_OK) {
        return cell;
      }
    }
  }
  return Cell{};
}

/***********************************************************************************************
 * BuildingClass::Can_Player_Move -- Can this building be moved? *
 *                                                                                             *
 *    This routine answers the question 'can this building be moved?' Typically,
 *only the      * construction yard can be moved and it does this by undeploying
 *back into a MCV.          *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Can the building move to a new location under player control? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/04/1995 JLB : Created. *
 *=============================================================================================*/
bool BuildingClass::Can_Player_Move() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  return *this == STRUCT_CONST;
}

/***********************************************************************************************
 * BuildingClass::Exit_Coord -- Determines location where object will leave it.
 **
 *                                                                                             *
 *    This routine will return the coordinate where an object that wishes to
 *leave the         * building will exit at. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the coordinate that the object should be created at. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/20/1996 JLB : Created. *
 *=============================================================================================*/
Coordinate BuildingClass::Exit_Coord() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().building(), this);
  DCHECK(IsActive);

  if (Class->ExitCoordinate) {
    return Coord + Class->ExitCoordinate;
  }
  return TechnoClass::Exit_Coord();
}

/***********************************************************************************************
 * BuildingClass::Check_Point -- Fetches the landing checkpoint for the given
 *flight pattern.  *
 *                                                                                             *
 *    Use this routine to coordinate a landing operation. The specified
 *checkpoint is          * converted into a cell number. The landing aircraft
 *should fly over that cell and then    * request the next check point. *
 *                                                                                             *
 * INPUT:   cp    -- The check point to convert to a cell number. *
 *                                                                                             *
 * OUTPUT:  Returns with the cell that the aircraft should fly over in order to
 *complete       * that portion of the landing pattern. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/06/1996 JLB : Created. *
 *=============================================================================================*/
Cell BuildingClass::Check_Point(CheckPointType cp) const {
  int xoffset = 6;  // Downwind offset, in cells.
  int yoffset = 5;  // Crosswind offset, in cells.
  const Cell cell = Cell::Containing(Center_Coord());

  switch (cp) {
    case CHECK_STACK:
      xoffset = 0;
      break;

    case CHECK_CROSSWIND:
      yoffset = 0;
      break;

    case CHECK_DOWNWIND:
    default:
      break;
  }

  if (cell.x() - TheMap().MapCellX > TheMap().MapCellWidth / 2) {
    xoffset = -xoffset;
  }

  if (cell.y() - TheMap().MapCellY > TheMap().MapCellHeight / 2) {
    yoffset = -yoffset;
  }

  return Cell::FromXY(cell.x() + xoffset, cell.y() + yoffset);
}

/***********************************************************************************************
 * BuildingClass::Update_Radar_Spied - set house's RadarSpied field
 *appropriately.				  *
 *                                                                                             *
 *    This routine is called when a radar facility is captured or destroyed.  It
 *fills in the  * RadarSpied field of the house based on whether there's a
 *spied-upon radar facility or not*
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  House->RadarSpied field gets set appropriately.
 **
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/22/1996 BWG : Created. *
 *=============================================================================================*/
void BuildingClass::Update_Radar_Spied() {
  House->RadarSpied = 0;
  for (int index = 0; index < TheObjectHeaps().building().Count(); index++) {
    const BuildingClass* obj = TheObjectHeaps().building().ActiveAt(index);
    if (obj && !obj->IsInLimbo && obj->House == House) {
      if (*obj == STRUCT_RADAR /* || *obj == STRUCT_EYE */) {
        House->RadarSpied |= obj->SpiedBy;
      }
    }
  }
  TheMap().RadarClass::Flag_To_Redraw(true);
}

/***********************************************************************************************
 * BuildingClass::Read_INI -- Reads buildings from INI file. *
 *                                                                                             *
 *    This is the basic scenario initialization of building function. It * is
 *called when reading the scenario startup INI file and it handles * creation of
 *all specified buildings.                                                     *
 *                                                                                             *
 *    INI entry format: * Housename, Typename, Strength, Cell, Facing,
 *Triggername                               *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the loaded INI file data. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/24/1994 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Read_INI(engine::file::IniFile& ini) {
  for (const auto& [entry, value] : ini.Entries(INI_Name())) {
    /*
    **	1st token: house name.
    */
    base::Tokenizer tokens(value);
    const HousesType bhouse =
        HouseTypeClass::From_Name(tokens.Next());  // Building house.

    /*
    **	2nd token: building name.
    */
    const StructType classid =
        BuildingTypeClass::From_Name(tokens.Next());  // Building type.

    if (bhouse != HOUSE_NONE && classid != STRUCT_NONE) {

      /*
      **	3rd token: strength.
      */
      int strength = base::ParseIntegerOr<int>(tokens.Next(), 0);

      /*
      **	4th token: cell #.
      */
      const Cell cell = Cell::FromIndex(base::ParseIntegerOr<int16_t>(
          tokens.Next(), 0));  // Cell of building.

      /*
      **	5th token: facing.
      */
      const auto facing =
          game::Direction(base::ParseIntegerOr<int>(tokens.Next(), 0));

      /*
      **	6th token: triggername (can be empty).
      */
      const std::string_view trigname = tokens.Next();

      bool sellable = false;
      std::string_view token_pointer = tokens.Next();
      if (!token_pointer.empty()) {
        sellable = base::ParseIntegerOr<int>(token_pointer, 0) != 0;
      }

      bool rebuild = false;
      token_pointer = tokens.Next();
      if (!token_pointer.empty()) {
        rebuild = base::ParseIntegerOr<int>(token_pointer, 0) != 0;
      }

      auto* b = base::NewPooled<BuildingClass>(
          classid, bhouse);  // Working unit pointer.
      if (b) {
        TriggerType* tp = TriggerType::FromName(trigname);
        if (tp) {
          TriggerClass* tt = Find_Or_Make(tp);
          if (tt) {
            tt->AttachCount++;
            b->Trigger = tt;
          }
        }
        b->IsAllowedToSell = sellable;
        b->IsToRebuild = rebuild;
        b->IsToRepair = rebuild || *b == STRUCT_CONST;

        if (b->Unlimbo(cell.Center(), facing)) {
          strength = std::min(strength, 0x100);
          strength = b->Class->MaxStrength * Fixed(strength, 256);
          b->Strength = static_cast<int16_t>(strength);
          if (b->Strength > b->Class->MaxStrength - 3) {
            b->Strength = b->Class->MaxStrength;
          }
          b->IsALemon = false;
        } else {
          /*
          **	If the building could not be unlimboed on the map, then this
          *indicates *	a serious error. Delete the building.
          */
          base::DeletePooled(b);
        }
      }
    }
  }
}

/***********************************************************************************************
 * BuildingClass::Write_INI -- Write out the building data to the INI file
 *specified.          *
 *                                                                                             *
 *    This will store the building data (as it relates to scenario
 *initialization) to the      * INI database specified. *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database that the building data will
 *be stored to.   *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/06/1996 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Write_INI(engine::file::IniFile& ini) {
  /*
  **	First, clear out all existing building data from the ini file.
  */
  ini.Clear(INI_Name());

  /*
  **	Write the data out.
  */
  for (int index = 0; index < TheObjectHeaps().building().Count(); index++) {
    const BuildingClass* building = TheObjectHeaps().building().ActiveAt(index);
    if (!building->IsInLimbo) {
      char uname[10];
      char buf[127];

      absl::SNPrintF(uname, sizeof(uname), "%d", index);
      absl::SNPrintF(buf, sizeof(buf), "%s,%s,%d,%u,%d,%s,%d,%d",
                     building->House->Class->IniName, building->Class->IniName,
                     building->Health_Ratio() * 256,
                     Cell::Containing(building->Coord).index(),
                     building->PrimaryFacing.current().value(),
                     building->Trigger != nullptr
                         ? building->Trigger->Class->IniName
                         : "None",
                     building->IsAllowedToSell, building->IsToRebuild);
      ini.PutString(INI_Name(), uname, buf);
    }
  }
}

/***********************************************************************************************
 * BuildingClass::Target_Coord -- Return the coordinate to use when firing on
 *this building.   *
 *                                                                                             *
 *    This routine will determine the "center" location of this building for
 *purposes of       * targeting. Usually, this location is somewhere near the
 *foundation of the building.      *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the coordinate to use when firing upon this building
 *(or trying to    * walk onto it). *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/19/1996 JLB : Created. *
 *=============================================================================================*/
Coordinate BuildingClass::Target_Coord() const {
  if (Class->FoundationFace != game::Facing::kNone) {
    return AdjacentCell(Center_Coord(), Class->FoundationFace);
  }
  return Center_Coord();
}

/***********************************************************************************************
 * BuildingClass::Factory_AI -- Handle factory production and initiation. *
 *                                                                                             *
 *    Some building (notably the computer controlled ones) can have a factory
 *object attached. * This routine handles processing of that factory and also
 *detecting when production       * should begin in order to initiate
 *production.                                            *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Only call this routine once per building per game logic loop. *
 *                                                                                             *
 * HISTORY: * 07/29/1996 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Factory_AI() {
  /*
  **	Handle any production tied to this building. Only computer controlled
  *buildings have *	production attached to the building itself. The player
  *uses the sidebar interface for *	all production control.
  */
  if (Factory != nullptr && Factory->Has_Completed() &&
      PlacementDelay.IsFinished()) {
    TechnoClass* product = Factory->Get_Object();
    //		FactoryClass * fact = Factory;

    switch (Exit_Object(product)) {
      /*
      **	If the object could not leave the factory, then either request
      **	a transport, place the (what must be a) building using another
      *method, or *	abort the production and refund money.
      */
      case 0:
        Factory->Abandon();
        delete Factory;
        Factory = nullptr;
        break;

      /*
      **	Exiting this building is prevented by some temporary blockage.
      *Wait *	a bit before trying again.
      */
      case 1:
        PlacementDelay.Set(int64_t{kTicksPerSecond} * 3);
        break;

      /*
      **	The object was successfully sent from this factory. Inform the
      *house *	tracking logic that the requested object has been produced.
      */
      case 2:
        switch (product->What_Am_I()) {
          case RTTI_VESSEL:
            House->JustBuiltVessel =
                dynamic_cast<VesselClass*>(product)->Class->Type;
            House->IsBuiltSomething = true;
            break;

          case RTTI_UNIT:
            House->JustBuiltUnit =
                dynamic_cast<UnitClass*>(product)->Class->Type;
            House->IsBuiltSomething = true;
            break;

          case RTTI_INFANTRY:
            House->JustBuiltInfantry =
                dynamic_cast<InfantryClass*>(product)->Class->Type;
            House->IsBuiltSomething = true;
            break;

          case RTTI_BUILDING:
            House->JustBuiltStructure =
                dynamic_cast<BuildingClass*>(product)->Class->Type;
            House->IsBuiltSomething = true;
            break;

          case RTTI_AIRCRAFT:
            House->JustBuiltAircraft =
                dynamic_cast<AircraftClass*>(product)->Class->Type;
            House->IsBuiltSomething = true;
            break;

          case RTTIType::RTTI_NONE:
          case RTTIType::RTTI_AIRCRAFTTYPE:
          case RTTIType::RTTI_ANIM:
          case RTTIType::RTTI_ANIMTYPE:
          case RTTIType::RTTI_BUILDINGTYPE:
          case RTTIType::RTTI_BULLET:
          case RTTIType::RTTI_BULLETTYPE:
          case RTTIType::RTTI_CELL:
          case RTTIType::RTTI_FACTORY:
          case RTTIType::RTTI_HOUSE:
          case RTTIType::RTTI_HOUSETYPE:
          case RTTIType::RTTI_INFANTRYTYPE:
          case RTTIType::RTTI_OVERLAY:
          case RTTIType::RTTI_OVERLAYTYPE:
          case RTTIType::RTTI_SMUDGE:
          case RTTIType::RTTI_SMUDGETYPE:
          case RTTIType::RTTI_SPECIAL:
          case RTTIType::RTTI_TEAM:
          case RTTIType::RTTI_TEAMTYPE:
          case RTTIType::RTTI_TEMPLATE:
          case RTTIType::RTTI_TEMPLATETYPE:
          case RTTIType::RTTI_TERRAIN:
          case RTTIType::RTTI_TERRAINTYPE:
          case RTTIType::RTTI_TRIGGER:
          case RTTIType::RTTI_TRIGGERTYPE:
          case RTTIType::RTTI_UNITTYPE:
          case RTTIType::RTTI_VESSELTYPE:
          default:
            break;
        }
        //				fact->Completed();
        Factory->Completed();
        //				delete fact;
        delete Factory;
        Factory = nullptr;
        break;

      default:
        break;
    }
  }

  /*
  **	Pick something to create for this factory.
  */
  if ((House->IsStarted && mission_state().current() != MISSION_CONSTRUCTION &&
       mission_state().current() != MISSION_DECONSTRUCTION) &&
      (Class->ToBuild != RTTI_NONE)) {
    /*
    **	Buildings that produce other objects have special factory logic handled
    *here.
    */
    if (Factory != nullptr) {
      /*
      **	If production has halted, then just abort production and make
      *the *	funds available for something else.
      */
      if (PlacementDelay.IsFinished() && !Factory->Is_Building()) {
        Factory->Abandon();
        delete Factory;
        Factory = nullptr;
      }

    } else {
      /*
      **	Only look to start production if there is at least a small
      *amount of *	money available. In cases where there is no practical
      *money left, then *	production can never complete -- don't bother
      *starting it.
      */
      if (House->IsStarted && House->Available_Money() > 10) {
        const TechnoTypeClass* techno =
            House->Suggest_New_Object(Class->ToBuild, *this == STRUCT_KENNEL);

        /*
        **	If a suitable object type was selected for production, then
        *start *	producing it now.
        */
        if (techno != nullptr) {
          Factory = base::NewPooled<FactoryClass>();
          if (Factory != nullptr) {
            if (!Factory->Set(*techno, *House)) {
              delete Factory;
              Factory = nullptr;
            } else {
              House->Production_Begun(Factory->Get_Object());
              Factory->Start();
            }
          }
        }
      }
    }
  }
}

/***********************************************************************************************
 * BuildingClass::Rotation_AI -- Process any turret rotation required of this
 *building.        *
 *                                                                                             *
 *    Some buildings have a turret and this routine handles processing the
 *turret rotation.    *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Only call this routine once per building per game logic loop. *
 *                                                                                             *
 * HISTORY: * 07/29/1996 JLB : Created. * 10/27/1996 JLB : Rotation does not
 *occur if power and no power avail.                     *
 *=============================================================================================*/
void BuildingClass::Rotation_AI() {
  if ((Class->IsTurretEquipped &&
       mission_state().current() != MISSION_CONSTRUCTION &&
       mission_state().current() != MISSION_DECONSTRUCTION &&
       (!Class->IsPowered || House->Power_Fraction() >= 1)) &&
      PrimaryFacing.IsRotating() && PrimaryFacing.Turn(Class->ROT))
  /*
  **	Rotate turret to match desired facing.
  */

  {
    Mark(MARK_CHANGE);
  }
}

/***********************************************************************************************
 * BuildingClass::Charging_AI -- Handles the special charging logic for Tesla
 *coils.           *
 *                                                                                             *
 *    This handles the special logic required of the charging tesla coil. It
 *requires special  * processing since its charge up is dependant upon the
 *target and power surplus of the     * owning house. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/29/1996 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Charging_AI() {
  if (Class->PrimaryWeapon != nullptr && Class->PrimaryWeapon->IsElectric &&
      BState != game::BSTATE_CONSTRUCTION) {
    if (TarCom && House->Power_Fraction() >= 1) {
      if (!IsCharged) {
        if (IsCharging) {
          //					if (stagechange) {
          Mark(MARK_CHANGE);
          if (animation().Fetch_Stage() >= 9) {
            IsCharged = true;
            IsCharging = false;
            animation().Set_Rate(0);
          }
          //					}
        } else if (Arm.IsFinished()) {
          IsCharged = false;
          IsCharging = true;
          animation().Set_Stage(0);
          animation().Set_Rate(3);
          PlaySoundEffectAt(VOC_TESLA_POWER_UP, Coord);
        }
      }
    } else {
      if (IsCharging || IsCharged) {
        Mark(MARK_CHANGE);
        IsCharging = false;
        IsCharged = false;
        animation().Set_Stage(0);
        animation().Set_Rate(0);
      }
    }
  }
}

/***********************************************************************************************
 * BuildingClass::Repair_AI -- Handle the repair (and sell) logic for the
 *building.            *
 *                                                                                             *
 *    This routine handle the repair animation and healing logic. It also
 *detects when the     * (computer controlled) building should begin repair or
 *sell itself.                       *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Only call this routine once per building per game logic loop. *
 *                                                                                             *
 * HISTORY: * 07/29/1996 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Repair_AI() {
  if ((House->IQ >= TheRules().IQRepairSell &&
       mission_state().current() != MISSION_CONSTRUCTION &&
       mission_state().current() != MISSION_DECONSTRUCTION) &&
      Can_Repair())
  /*
  **	Possibly start repair process if the building is below half strength.
  */
  //		unsigned ratio = std::min(House->Smartness, 0x00F0);
  {
    if (House->Available_Money() >= TheRules().RepairThreshhold) {
      if ((!House->DidRepair) &&
          (!IsRepairing && (IsCaptured || IsToRepair || House->IsHuman ||
                            TheSession().Type != GAME_NORMAL))) {
        House->DidRepair = true;  // flag that this house did its repair
                                  // allocation for this frame
        Repair(1);

        if (!House->IsHuman) {
          House->RepairTimer.Set(
              RandomPick(House->RepairDelay * (kTicksPerMinute / 4),
                         House->RepairDelay * kTicksPerMinute * 2));
        }
      }

    } else {
      if ((TheSession().Type != GAME_NORMAL || IsAllowedToSell) &&
          IsTickedOff && House->Control.TechLevel >= TheRules().IQSellBack &&
          RandomPick(0, 50) < House->Control.TechLevel && Trigger == nullptr &&
          *this != STRUCT_CONST && Health_Ratio() < TheRules().ConditionRed) {
        Sell_Back(1);
      }
    }
  }

  /*
  **	If it is repairing, then apply any repair effects as necessary.
  */
  if (IsRepairing &&
      game::CurrentFrame() % (TheRules().RepairRate * kTicksPerMinute) == 0) {
    IsWrenchVisible = !static_cast<bool>(IsWrenchVisible);
    Mark(MARK_CHANGE);
    const int cost = Class->Repair_Cost();
    const int step = Class->Repair_Step();

    /*
    **	Check for and expend any necessary monies to continue the repair.
    */
    if (House->Available_Money() >= cost) {
      House->Spend_Money(cost);
      Strength = static_cast<int16_t>(Strength + step);

      if (std::cmp_greater_equal(Strength, Class->MaxStrength)) {
        Strength = Class->MaxStrength;
        IsRepairing = false;
      }
    } else {
      IsRepairing = false;
    }
  }
}

/***********************************************************************************************
 * BuildingClass::Animation_AI -- Handles normal building animation processing.
 **
 *                                                                                             *
 *    This will process the general building animation mechanism. It detects
 *when the          * building animation sequence has completed and flags the
 *building to perform mission      * changes as a result. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Call this routine only once per building per game logic loop. *
 *                                                                                             *
 * HISTORY: * 07/29/1996 JLB : Created. *
 *=============================================================================================*/
void BuildingClass::Animation_AI() {
  const bool stagechange = animation().Graphic_Logic();
  bool toloop = false;

  /*
  **	Always refresh the SAM site if it has an animation change.
  */
  if (*this == STRUCT_SAM && stagechange) {
    Mark(MARK_CHANGE);
  }

  if ((!Class->IsTurretEquipped && *this != STRUCT_TESLA) ||
      mission_state().current() == MISSION_CONSTRUCTION ||
      mission_state().current() == MISSION_DECONSTRUCTION) {
    if (stagechange) {
      /*
      **	Check for animation end or if special case of MCV deconstructing
      *when it is allowed *	to convert back into an MCV.
      */
      const BuildingTypeClass::AnimControlType* ctrl = Fetch_Anim_Control();

      /*
      **	When the last frame of the current animation sequence is
      *reached, flag that *	a new mission may be started. This must occur
      *before the animation actually *	loops so that if a mission change does
      *occur, it will have a chance to change *	the building graphic before the
      *last frame is replaced by the first frame of *	the loop.
      */
      if (animation().Fetch_Stage() == ctrl->Start + ctrl->Count - 1 ||
          (!ArchiveTarget /*Special.IsMCVDeploy*/ && *this == STRUCT_CONST &&
           mission_state().current() == MISSION_DECONSTRUCTION &&
           animation().Fetch_Stage() == 42 - 19)) {
        IsReadyToCommence = true;
      }

      /*
      **	If the animation advances beyond the last frame, then start the
      *animation *	sequence over from the beginning.
      */
      if (animation().Fetch_Stage() >= ctrl->Start + ctrl->Count) {
        toloop = true;
      }
      Mark(MARK_CHANGE);
    } else {
      if (BState == game::BSTATE_NONE || animation().Fetch_Rate() == 0) {
        IsReadyToCommence = true;
      }
    }
  }

  /*
  **	The animation sequence has looped. Restart it and flag this loop
  *condition. *	This is used to tell the mission system that the animation has
  *completed. It *	also signals that now is a good time to act on any
  *pending mission.
  */
  if (toloop) {
    const BuildingTypeClass::AnimControlType* ctrl = Fetch_Anim_Control();
    if (BState == game::BSTATE_CONSTRUCTION || BState == game::BSTATE_IDLE) {
      animation().Set_Rate(TheOptions().Normalize_Delay(ctrl->Rate));
    } else {
      animation().Set_Rate(ctrl->Rate);
    }
    animation().Set_Stage(ctrl->Start);
    Mark(MARK_CHANGE);
  }
}

// Survivor count is building cost scaled by SurvivorFraction, measured in
// minigunner cost units. Captured buildings double the cost divisor to halve
// the survivor count as a penalty.
int BuildingClass::How_Many_Survivors() const {
  if (IsSurvivorless || !Class->IsCrew) {
    return 0;
  }

  int soldier_cost = InfantryTypeClass::As_Reference(INFANTRY_E1).Raw_Cost();
  if (soldier_cost == 0) {
    return 0;
  }
  if (IsCaptured) {
    soldier_cost *= 2;
  }
  const int survivors =
      Class->Raw_Cost() * TheRules().SurvivorFraction / soldier_cost;
  return std::clamp(survivors, 1, 5);
}

/***********************************************************************************************
 * BuildingClass::Get_Image_Data -- Fetch the image pointer for the building. *
 *                                                                                             *
 *    This routine will return with a pointer to the shape data for the
 *building. The shape    * data is different than normal when the building is
 *undergoing construction and           * disassembly. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the shape data for this building. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/06/1996 JLB : Created. *
 *=============================================================================================*/
std::span<const std::byte> BuildingClass::Get_Image_Data() const {
  if (BState == game::BSTATE_CONSTRUCTION) {
    return Class->Get_Buildup_Data();
  }
  return TechnoClass::Get_Image_Data();
}

/***********************************************************************************************
 * BuildingClass::Value -- Determine the value of this building. *
 *                                                                                             *
 *    The value of the building is normally just its ordinary assigned value.
 *However, in the  * case of fakes, the value is artificially enhanced to match
 *the structure that is         * being faked. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the point value of the building type. *
 *                                                                                             *
 * WARNINGS:   The point value returned should not be used for scoring, only for
 *target        * scanning. *
 *                                                                                             *
 * HISTORY: * 09/16/1996 JLB : Created. *
 *=============================================================================================*/
int BuildingClass::Value() const {
  if (Class->IsFake) {
    switch (Class->Type) {
      case STRUCT_FAKEWEAP:
        return BuildingTypeClass::As_Reference(STRUCT_WEAP).Reward +
               BuildingTypeClass::As_Reference(STRUCT_WEAP).Risk;

      case STRUCT_FAKECONST:
        return BuildingTypeClass::As_Reference(STRUCT_CONST).Reward +
               BuildingTypeClass::As_Reference(STRUCT_CONST).Risk;

      case STRUCT_FAKE_YARD:
        return BuildingTypeClass::As_Reference(STRUCT_SHIP_YARD).Reward +
               BuildingTypeClass::As_Reference(STRUCT_SHIP_YARD).Risk;

      case STRUCT_FAKE_PEN:
        return BuildingTypeClass::As_Reference(STRUCT_SUB_PEN).Reward +
               BuildingTypeClass::As_Reference(STRUCT_SUB_PEN).Risk;

      case STRUCT_FAKE_RADAR:
        return BuildingTypeClass::As_Reference(STRUCT_RADAR).Reward +
               BuildingTypeClass::As_Reference(STRUCT_RADAR).Risk;

      case StructType::STRUCT_NONE:
      case StructType::STRUCT_ADVANCED_TECH:
      case StructType::STRUCT_IRON_CURTAIN:
      case StructType::STRUCT_WEAP:
      case StructType::STRUCT_CHRONOSPHERE:
      case StructType::STRUCT_PILLBOX:
      case StructType::STRUCT_CAMOPILLBOX:
      case StructType::STRUCT_RADAR:
      case StructType::STRUCT_GAP:
      case StructType::STRUCT_TURRET:
      case StructType::STRUCT_AAGUN:
      case StructType::STRUCT_FLAME_TURRET:
      case StructType::STRUCT_CONST:
      case StructType::STRUCT_REFINERY:
      case StructType::STRUCT_STORAGE:
      case StructType::STRUCT_HELIPAD:
      case StructType::STRUCT_SAM:
      case StructType::STRUCT_AIRSTRIP:
      case StructType::STRUCT_POWER:
      case StructType::STRUCT_ADVANCED_POWER:
      case StructType::STRUCT_SOVIET_TECH:
      case StructType::STRUCT_HOSPITAL:
      case StructType::STRUCT_BARRACKS:
      case StructType::STRUCT_TENT:
      case StructType::STRUCT_KENNEL:
      case StructType::STRUCT_REPAIR:
      case StructType::STRUCT_BIO_LAB:
      case StructType::STRUCT_MISSION:
      case StructType::STRUCT_SHIP_YARD:
      case StructType::STRUCT_SUB_PEN:
      case StructType::STRUCT_MSLO:
      case StructType::STRUCT_FORWARD_COM:
      case StructType::STRUCT_TESLA:
      case StructType::STRUCT_SANDBAG_WALL:
      case StructType::STRUCT_CYCLONE_WALL:
      case StructType::STRUCT_BRICK_WALL:
      case StructType::STRUCT_BARBWIRE_WALL:
      case StructType::STRUCT_WOOD_WALL:
      case StructType::STRUCT_FENCE:
      case StructType::STRUCT_AVMINE:
      case StructType::STRUCT_APMINE:
      case StructType::STRUCT_V01:
      case StructType::STRUCT_V02:
      case StructType::STRUCT_V03:
      case StructType::STRUCT_V04:
      case StructType::STRUCT_V05:
      case StructType::STRUCT_V06:
      case StructType::STRUCT_V07:
      case StructType::STRUCT_V08:
      case StructType::STRUCT_V09:
      case StructType::STRUCT_V10:
      case StructType::STRUCT_V11:
      case StructType::STRUCT_V12:
      case StructType::STRUCT_V13:
      case StructType::STRUCT_V14:
      case StructType::STRUCT_V15:
      case StructType::STRUCT_V16:
      case StructType::STRUCT_V17:
      case StructType::STRUCT_V18:
      case StructType::STRUCT_PUMP:
      case StructType::STRUCT_V20:
      case StructType::STRUCT_V21:
      case StructType::STRUCT_V22:
      case StructType::STRUCT_V23:
      case StructType::STRUCT_V24:
      case StructType::STRUCT_V25:
      case StructType::STRUCT_V26:
      case StructType::STRUCT_V27:
      case StructType::STRUCT_V28:
      case StructType::STRUCT_V29:
      case StructType::STRUCT_V30:
      case StructType::STRUCT_V31:
      case StructType::STRUCT_V32:
      case StructType::STRUCT_V33:
      case StructType::STRUCT_V34:
      case StructType::STRUCT_V35:
      case StructType::STRUCT_V36:
      case StructType::STRUCT_V37:
      case StructType::STRUCT_BARREL:
      case StructType::STRUCT_BARREL3:
      case StructType::STRUCT_QUEEN:
      case StructType::STRUCT_LARVA1:
      case StructType::STRUCT_LARVA2:
      default:
        break;
    }
  }
  return TechnoClass::Value();
}

/***********************************************************************************************
 * BuildingClass::Remove_Gap_Effect -- Stop a gap generator from jamming cells.
 **
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none
 **
 *                                                                                             *
 * WARNINGS:
 **
 *                                                                                             *
 * HISTORY: * 09/20/1996 BWG : Created. *
 *=============================================================================================*/
void BuildingClass::Remove_Gap_Effect() {
  // unjam this one's field...
  MapEditClass::UnJam_From(Cell::Containing(Center_Coord()),
                           TheRules().GapShroudRadius, House);
  if (!House->IsPlayerControl && ThePlayer()->IsGPSActive) {
    TheMap().Sight_From(Cell::Containing(Center_Coord()),
                        TheRules().GapShroudRadius, ThePlayer());
  }
  // and rejam any overlapping buildings' fields
  for (int index = 0; index < TheObjectHeaps().building().Count(); index++) {
    BuildingClass* obj = TheObjectHeaps().building().ActiveAt(index);
    if (obj && !obj->IsInLimbo && obj->House == House && *obj == STRUCT_GAP &&
        obj != this) {
      obj->IsJamming = false;
      obj->Arm.Set(0);
      //			Map.Jam_From(Cell::Containing(obj->Center_Coord()),
      // Rule.GapShroudRadius, PlayerPtr);
    }
  }
}

std::span<const int16_t> BuildingClass::Overlap_List(bool redraw) const {
  if ((SpiedBy & base::Bit<uint32_t>(ThePlayer()->Class->House)) != 0 &&
      IsSelected && (*this == STRUCT_BARRACKS || *this == STRUCT_TENT)) {
    static const int16_t _list[] = {-1, 2, (MAP_CELL_W * 1) - 1,
                                    (MAP_CELL_W * 1) + 2, kRefreshEol};
    return _list;
  }
  return TechnoClass::Overlap_List(redraw);
}

}  // namespace ra
