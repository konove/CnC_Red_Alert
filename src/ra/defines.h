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

/* $Header: /CounterStrike/DEFINES.H 4     3/07/97 9:55a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DEFINES.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : September 10, 1993   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */
#ifndef CNC_RED_ALERT_RA_DEFINES_H_
#define CNC_RED_ALERT_RA_DEFINES_H_

#include <cstdint>
#include <span>
#include <utility>

#include "base/attributes.h"
#include "base/enum_array.h"
#include "base/numeric.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/jshell.h"
#include "tech/fixed.h"

/**********************************************************************
**	Optional parameter control for special options.
*/

// Obfuscated command-line keywords, compared against Obfuscate() output.
// Cheat behaviour itself is controlled by config::kVirginCheatKeysEnabled.
inline constexpr uint32_t kParmPlaytest = 0xF7DDC227;  // "PLAYTEST"
inline constexpr uint32_t kParmInstall = 0xD95C68A2;   // "FROMINSTALL"

/**********************************************************************
**	Defines for verifying free disk space
*/
inline constexpr int kInitFreeDiskSpace = 8388608;
inline constexpr int kSaveGameDiskSpace = kInitFreeDiskSpace - (1024 * 4096);

/**********************************************************************
**	This is the complete list of VQs allowed to be played in the game.
*/
enum class VQType {
  VQ_NONE = -1,
  VQ_AAGUN,
  VQ_MIG,
  VQ_SFROZEN,
  VQ_AIRFIELD,
  VQ_BATTLE,
  VQ_BMAP,
  VQ_BOMBRUN,
  VQ_DPTHCHRG,
  VQ_GRVESTNE,
  VQ_MONTPASS,
  VQ_MTNKFACT,
  VQ_CRONTEST,
  VQ_OILDRUM,
  VQ_ALLYEND,
  VQ_RADRRAID,
  VQ_SHIPYARD,
  VQ_SHORBOMB,
  VQ_SITDUCK,
  VQ_SLNTSRVC,
  VQ_SNOWBASE,
  VQ_EXECUTE,
  VQ_TITLE,  // Low res.
  VQ_NUKESTOK,
  VQ_V2ROCKET,
  VQ_SEARCH,
  VQ_BINOC,
  VQ_ELEVATOR,
  VQ_FROZEN,
  VQ_MCV,
  VQ_SHIPSINK,
  VQ_SOVMCV,
  VQ_TRINITY,
  VQ_ALLYMORF,
  VQ_APCESCPE,
  VQ_BRDGTILT,
  VQ_CRONFAIL,
  VQ_STRAFE,
  VQ_DESTROYR,
  VQ_DOUBLE,
  VQ_FLARE,
  VQ_SNSTRAFE,
  VQ_LANDING,
  VQ_ONTHPRWL,
  VQ_OVERRUN,
  VQ_SNOWBOMB,
  VQ_SOVCEMET,
  VQ_TAKE_OFF,
  VQ_TESLA,
  VQ_SOVIET8,
  VQ_SPOTTER,
  VQ_SCENE1,
  VQ_SCENE2,
  VQ_SCENE4,
  VQ_SOVFINAL,
  VQ_ASSESS,
  VQ_SOVIET10,
  VQ_DUD,
  VQ_MCV_LAND,
  VQ_MCVBRDGE,
  VQ_PERISCOP,
  VQ_SHORBOM1,
  VQ_SHORBOM2,
  VQ_SOVBATL,
  VQ_SOVTSTAR,
  VQ_AFTRMATH,
  VQ_SOVIET11,
  VQ_MASASSLT,
  VQ_REDINTRO,  // High res
  VQ_SOVIET1,
  VQ_SOVIET2,
  VQ_SOVIET3,
  VQ_SOVIET4,
  VQ_SOVIET5,
  VQ_SOVIET6,
  VQ_SOVIET7,
  VQ_INTRO_MOVIE,
  VQ_AVERTED,
  VQ_COUNTDWN,
  VQ_MOVINGIN,
  VQ_ALLIED10,
  VQ_ALLIED12,
  VQ_ALLIED5,
  VQ_ALLIED6,
  VQ_ALLIED8,
  VQ_TANYA1,
  VQ_TANYA2,
  VQ_ALLY10B,
  VQ_ALLY11,
  VQ_ALLY14,
  VQ_ALLY9,
  VQ_SPY,
  VQ_TOOFAR,
  VQ_SOVIET12,
  VQ_SOVIET13,
  VQ_SOVIET9,
  VQ_BEACHEAD,
  VQ_SOVIET14,
  VQ_SIZZLE,
  VQ_SIZZLE2,
  VQ_ANTEND,
  VQ_ANTINTRO
};
using enum VQType;

/**********************************************************************
**	These enumerations are used to implement RTTI. The target system
**	uses these and thus there can be no more RTTI types than can fit
**	in the exponent of a target value.
*/
enum class RTTIType {
  RTTI_NONE = 0,
  RTTI_AIRCRAFT,
  RTTI_AIRCRAFTTYPE,
  RTTI_ANIM,
  RTTI_ANIMTYPE,
  RTTI_BUILDING,
  RTTI_BUILDINGTYPE,
  RTTI_BULLET,
  RTTI_BULLETTYPE,
  RTTI_CELL,
  RTTI_FACTORY,
  RTTI_HOUSE,
  RTTI_HOUSETYPE,
  RTTI_INFANTRY,
  RTTI_INFANTRYTYPE,
  RTTI_OVERLAY,
  RTTI_OVERLAYTYPE,
  RTTI_SMUDGE,
  RTTI_SMUDGETYPE,
  RTTI_SPECIAL,
  RTTI_TEAM,
  RTTI_TEAMTYPE,
  RTTI_TEMPLATE,
  RTTI_TEMPLATETYPE,
  RTTI_TERRAIN,
  RTTI_TERRAINTYPE,
  RTTI_TRIGGER,
  RTTI_TRIGGERTYPE,
  RTTI_UNIT,
  RTTI_UNITTYPE,
  RTTI_VESSEL,
  RTTI_VESSELTYPE
};
using enum RTTIType;

// These are the difficulty settings of the game.
enum class DiffType { DIFF_EASY, DIFF_NORMAL, DIFF_HARD };
using enum DiffType;
inline constexpr int kDiffCount = static_cast<int>(DIFF_HARD) + 1;

/**********************************************************************
**	This is the size of the speech buffer. This value should be as large
**	as the largest speech sample, plus a few bytes for overhead
**	(16 bytes is sufficient).
*/
inline constexpr int kSpeechBufferSize = 50000;

/**********************************************************************
**	The theater mixfiles are cached into a buffer of this size. Ensure
**	that the size specified is at least as large as the largest
**	theater mixfile data block.
*/
inline constexpr int kTheaterBufferSize = 1100000;

/**********************************************************************
**	This is the size of the shape buffer. This buffer is used as a staging
**	buffer for the shape drawing technology. It MUST be as big as the
**	largest shape (uncompressed) that will be drawn. If this value is
**	changed, be sure to update the makefile and rebuild all of the shape
**	data files.
*/
inline constexpr int kShapeBufferSize = 65000;

/**********************************************************************
**	Filenames of the data files it can create at run time.
*/
inline constexpr char kFameFileName[] = "HALLFAME.DAT";
inline constexpr char kNetSaveFileName[] = "SAVEGAME.NET";
inline constexpr char kConfigFileName[] = "REDALERT.INI";

/**********************************************************************
**	Map controls. The map is composed of square elements called 'cells'.
**	All larger elements are build upon these.
*/

// Set in either half of a COORDINATE when it points off the map.
inline constexpr uint32_t kHighCoordMask = 0x80008000;

// A map coordinate with cell resolution. Declared here rather than alongside
// COORDINATE below so that the map dimensions can carry it: a loop walking the
// map then has a counter no narrower than its own bound.
using CELL = int16_t;

// Size of the map in cells. The brace initialization fails to compile if the
// map ever outgrows a CELL.
inline constexpr CELL MAP_CELL_W{128};
inline constexpr CELL MAP_CELL_H{128};
inline constexpr CELL MAP_CELL_TOTAL{MAP_CELL_W * MAP_CELL_H};

// This number ends a refresh/occupy offset list.
inline constexpr int16_t kRefreshEol = 32767;

// This number flags that sidebar needs refreshing.
inline constexpr int16_t kRefreshSidebar = 32766;

/****************************************************************************
**	These are custom C&C specific types. The CELL (declared above, with the
**	map dimensions) is used for map coordinates with cell resolution. The
**	COORDINATE type is used for map coordinates that have a lepton
**	resolution. CELL is more efficient when indexing into the map and when
**	size is critical. COORDINATE is more efficient when dealing with
**	accuracy and object movement.
*/
using LEPTON = uint16_t;
union LEPTON_COMPOSITE {
  LEPTON Raw;
  struct {
    unsigned char Lepton;
    unsigned char Cell;
  } Sub;
};

using COORDINATE = uint32_t;
union COORD_COMPOSITE {
  COORDINATE Coord;
  struct {
    LEPTON_COMPOSITE X;
    LEPTON_COMPOSITE Y;
  } Sub;
};

union CELL_COMPOSITE {
  CELL Cell;
  // uint16_t bit-fields keep the union the size of a CELL, which is what
  // lets std::bit_cast convert between the two.
  struct {
    uint16_t X : 7;
    uint16_t Y : 7;
  } Sub;
};

// The composites are std::bit_cast views of their packed words.
static_assert(sizeof(LEPTON_COMPOSITE) == sizeof(LEPTON));
static_assert(sizeof(COORD_COMPOSITE) == sizeof(COORDINATE));
static_assert(sizeof(CELL_COMPOSITE) == sizeof(CELL));

using WAYPOINT = int;

/**********************************************************************
**	This is the target composit information. Notice that with an RTTI_NONE
**	and an index value of 0, the target value returned is identical with
**	kTargetNone. This is by design and is necessary.
*/
using TARGET = int32_t;

inline constexpr int kTargetMantissaBits = 24;  // Bits of value precision.
inline constexpr int kTargetExponentBits = 8;
union TARGET_COMPOSITE {
  TARGET Target;
  struct {
    unsigned Mantissa : kTargetMantissaBits;
    unsigned Exponent : kTargetExponentBits;
  } Sub;
};
static_assert(sizeof(TARGET_COMPOSITE) == sizeof(TARGET));

inline TARGET Build_Target(const RTTIType kind, const int value) {
  TARGET_COMPOSITE target{};

  target.Target = 0;
  target.Sub.Exponent = static_cast<unsigned>(kind);
  target.Sub.Mantissa = static_cast<unsigned>(value);
  return target.Target;
}

inline constexpr TARGET kTargetNone{};

/*
**	The map is broken down into regions of this specified dimensions.
*/
inline constexpr int kRegionWidth = 4;
inline constexpr int kRegionHeight = 4;
// Region grid size: the map rounded up to whole regions, plus a one-region
// border on every side.
inline constexpr int kMapRegionWidth =
    ((MAP_CELL_W + kRegionWidth - 1) / kRegionWidth) + 2;
inline constexpr int kMapRegionHeight =
    ((MAP_CELL_H + kRegionHeight - 1) / kRegionHeight) + 2;
inline constexpr int kMapTotalRegions = kMapRegionWidth * kMapRegionHeight;

/**********************************************************************
**	This enumerates the various known fear states for infantry units.
**	At these stages, certain events or recovery actions are performed.
*/
enum class FearType {
  FEAR_NONE = 0,      // No fear at all (default state).
  FEAR_ANXIOUS = 10,  // Something makes them scared.
  FEAR_SCARED = 100,  // Scared enough to take cover.
  FEAR_PANIC = 200,   // Run away! Run away!
  FEAR_MAXIMUM = 255  // Scared to death.
};
using enum FearType;

/**********************************************************************
**	When a moving object moves, the Per_Cell_Process function is called
**	at various times during the move. Certain operations must be
**	performed at different stages of the move. This enum specifies the
**	different conditions under which the Per_Cell_Process function is
**	called.
*/
enum class PCPType {
  PCP_ROTATION,  // When sitting in place and performing rotations.
  PCP_DURING,    // While moving between two cells.
  PCP_END,       // When the 'center' of a cell is reached during movement.
};
using enum PCPType;

/**********************************************************************
**	A base is broken up into several zones. This type enumerates the
**	various zones.
*/
enum class ZoneType {
  ZONE_CORE = 0,   // Center of base.
  ZONE_NORTH = 1,  // North section.
  ZONE_EAST = 2,   // East section.
  ZONE_SOUTH = 3,  // South section.
  ZONE_WEST = 4,   // West section.

  ZONE_NONE = -1
};
using enum ZoneType;

/**********************************************************************
**	The map is prescanned to mark of movement zones according to certain
**	movement characteristics. This enum specifies those characteristics
**	and movement zones kept track of.
*/
enum class MZoneType {
  MZONE_NORMAL,     // Normal terrestrial objects (can't crush walls).
  MZONE_CRUSHER,    // Can crush crushable wall types.
  MZONE_DESTROYER,  // Can destroy walls.
  MZONE_WATER       //	Water based objects.
};
using enum MZoneType;

// Bit masks over MZoneType for Map.Zone_Reset and zone checks.
inline constexpr uint32_t kZoneFlagNormal = base::Bit<uint32_t>(MZONE_NORMAL);
inline constexpr uint32_t kZoneFlagCrusher = base::Bit<uint32_t>(MZONE_CRUSHER);
inline constexpr uint32_t kZoneFlagDestroyer =
    base::Bit<uint32_t>(MZONE_DESTROYER);
inline constexpr uint32_t kZoneFlagWater = base::Bit<uint32_t>(MZONE_WATER);
inline constexpr uint32_t kZoneFlagAll =
    kZoneFlagNormal | kZoneFlagCrusher | kZoneFlagDestroyer | kZoneFlagWater;

/**********************************************************************
**	This records the current state of the computer controlled base. The
**	AI will respond according to this state in order to control
**	production and unit orders.
*/
enum class StateType {
  STATE_BUILDUP,     // Base is building up (defensive buildup stage).
  STATE_BROKE,       // Low on money, need cash or income source.
  STATE_THREATENED,  // Enemy units are designated to move close by.
  STATE_ATTACKED,    // Base is under direct attack.
  STATE_ENDGAME      //	Resistance is futile.
};
using enum StateType;

/**********************************************************************
**	Urgency rating used to determine what action to perform. The greater
**	the urgency the more likely the corresponding action will be chosen.
**	These values are assigned to each potential desired action the house
**	is to perform.
*/
enum class UrgencyType {
  URGENCY_NONE,     // No action on this matter is needed or desired.
  URGENCY_LOW,      // Minimal attention requested.
  URGENCY_MEDIUM,   //	Normal attention requested.
  URGENCY_HIGH,     // High attention requested.
  URGENCY_CRITICAL  // This matter must be addressed immediately.
};
using enum UrgencyType;

/**********************************************************************
**	These are the various actions a house may perform. These actions refer
**	to global events that encompass selling and production. Low level house
**	specific actions of choosing targets is handled elsewhere.
*/
enum class StrategyType {
  STRATEGY_BUILD_POWER,     // Power is low, build more.
  STRATEGY_BUILD_DEFENSE,   // Defense needs boosting.
  STRATEGY_BUILD_INCOME,    // Income is low or in jeopardy, raise it.
  STRATEGY_FIRE_SALE,       // Situation hopeless, sell and attack.
  STRATEGY_BUILD_ENGINEER,  // An engineer is needed.
  STRATEGY_BUILD_OFFENSE,   // Offensive weapons are needed.
  STRATEGY_RAISE_MONEY,     // Money is low, emergency raise cash.
  STRATEGY_RAISE_POWER,     // Money is low, raise power by selling.
  STRATEGY_LOWER_POWER,     // Too much power, sell power plants.
  STRATEGY_ATTACK           // General charge the enemy attack logic.
};
using enum StrategyType;

/**********************************************************************
**	These are the various return conditions that production may
**	produce.
*/
enum class ProdFailType {
  PROD_OK,       // Production request successful.
  PROD_LIMIT,    // Failed with production capacity limit reached.
  PROD_ILLEGAL,  // Failed because of illegal request.
  PROD_CANT      // Failed because unable to comply (busy or occupied).
};
using enum ProdFailType;

/**********************************************************************
**	When performing a landing operation, the aircraft must pass through
**	navigation checkpoints. These enumerations specify the check points.
*/
enum class CheckPointType {
  CHECK_STACK,     // Holding area far away from airfield.
  CHECK_DOWNWIND,  // Downwind leg of approach.
  CHECK_CROSSWIND  // Crosswind leg of approach.
};
using enum CheckPointType;

/**********************************************************************
**	These enumerate the various crate powerups that are available.
*/
enum class CrateType {
  CRATE_MONEY,
  CRATE_UNIT,
  CRATE_PARA_BOMB,
  CRATE_HEAL_BASE,
  CRATE_CLOAK,
  CRATE_EXPLOSION,
  CRATE_NAPALM,
  CRATE_SQUAD,
  CRATE_DARKNESS,
  CRATE_REVEAL,
  CRATE_SONAR,
  CRATE_ARMOR,
  CRATE_SPEED,
  CRATE_FIREPOWER,
  CRATE_ICBM,
  CRATE_TIMEQUAKE,
  CRATE_INVULN,
  CRATE_VORTEX
};
using enum CrateType;

/**********************************************************************
**	These are the special weapons that can be used in the game. The common
*thread *	with these weapons is that they are controlled through the
*sidebar *	mechanism.
*/
// Fixed underlying type so that kSpcChrono2, one past the last weapon, is a
// representable value.
enum class SpecialWeaponType : int {
  SPC_NONE = -1,
  SPC_SONAR_PULSE,    // Momentarily reveals submarines.
  SPC_NUCLEAR_BOMB,   //	Tactical nuclear weapon.
  SPC_CHRONOSPHERE,   // Paradox device, for teleportation
  SPC_PARA_BOMB,      // Parachute bomb delivery.
  SPC_PARA_INFANTRY,  // Parachute reinforcement delivery.
  SPC_SPY_MISSION,    // Spy plane to take photo recon mission.
  SPC_IRON_CURTAIN,   // Bestow invulnerability on a unit/building
  SPC_GPS             // give allies free unjammable radar.
};
using enum SpecialWeaponType;
// Second stage of chronosphere targeting: picking the destination.
inline constexpr auto kSpcChrono2 =
    static_cast<SpecialWeaponType>(magic_enum::enum_count<SpecialWeaponType>());

/**********************************************************************
**	The computer AI is categorized by the following enumerations. If
**	the player is controlling a house, then the IQ rating is zero. When
**	the IQ rating is at maximum, then the computer has complete control
**	of the house.
*/
// enum IQType {
//   IQ_DEAD=0x0000,// Player controlled (computer does nothing).
//   IQ_IDIOT=0x0040,
//   IQ_IMBECILE=0x0080,
//   IQ_MORON=0x00C0,
//   IQ_MENSA=0x0100 // Complete computer control -- look out!
// };

/**********************************************************************
**	These are the response values when checking to see if an object
**	can enter or exist at a specified location. By examining this
**	return value, appropriate action may be chosen.
**	NOTE: If this changes, update the static array in Find_Path module.
*/
enum class MoveType {
  MOVE_OK,            // No blockage.
  MOVE_CLOAK,         // A cloaked blocking enemy object.
  MOVE_MOVING_BLOCK,  // Blocked, but only temporarily.
  MOVE_DESTROYABLE,   // Enemy unit or building is blocking.
  MOVE_TEMP,          // Blocked by friendly unit.
  MOVE_NO             // Strictly prohibited terrain.
};
using enum MoveType;

/**********************************************************************
**	These are the themes that the game can play. They must be in exact
**	same order as specified in the CONQUER.TXT file as well as the filename
**	list located in the ThemeClass.
*/
enum class ThemeType {
  THEME_QUIET = -3,
  THEME_PICK_ANOTHER = -2,
  THEME_NONE = -1,
  THEME_BIGF = 0,
  THEME_CRUS = 1,
  THEME_FAC1 = 2,
  THEME_FAC2 = 3,
  THEME_HELL = 4,
  THEME_RUN1 = 5,
  THEME_SMSH = 6,
  THEME_TREN = 7,
  THEME_WORK = 8,
  THEME_AWAIT = 9,
  THEME_DENSE_R = 10,
  THEME_FOGGER1A = 11,
  THEME_MUD1A = 12,
  THEME_RADIO2 = 13,
  THEME_ROLLOUT = 14,
  THEME_SNAKE = 15,
  THEME_TERMINAT = 16,
  THEME_TWIN = 17,
  THEME_VECTOR1A = 18,

  THEME_MAP = 19,
  THEME_SCORE = 20,
  THEME_INTRO = 21,
  THEME_CREDITS = 22,

  THEME_2ND_HAND = 23,
  THEME_ARAZOID = 24,
  THEME_BACKSTAB = 25,
  THEME_CHAOS2 = 26,
  THEME_SHUT_IT = 27,
  THEME_TWINMIX1 = 28,
  THEME_UNDER3 = 29,
  THEME_VR2 = 30,

  THEME_BOG = 31,
  THEME_FLOAT_V2 = 32,
  THEME_GLOOM = 33,
  THEME_GRNDWIRE = 34,
  THEME_RPT = 35,
  THEME_SEARCH = 36,
  THEME_TRACTION = 37,
  THEME_WASTELND = 38
};
using enum ThemeType;

/**********************************************************************
**	This is the various threat scan methods that can be used when looking
**	for targets.
*/
enum class CNC_FLAG_ENUM ThreatType {
  THREAT_NORMAL = 0x0000,    // Any distance threat scan?
  THREAT_RANGE = 0x0001,     // Limit scan to weapon range?
  THREAT_AREA = 0x0002,      // Limit scan to general area (twice weapon range)?
  THREAT_AIR = 0x0004,       // Scan for air units?
  THREAT_INFANTRY = 0x0008,  // Scan for infantry units?
  THREAT_VEHICLES = 0x0010,  // Scan for vehicles?
  THREAT_BUILDINGS = 0x0020,  // Scan for buildings?
  THREAT_TIBERIUM = 0x0040,   // Limit scan to Tiberium processing objects?
  THREAT_BOATS = 0x0080,      // Scan for gunboats?
  THREAT_CIVILIANS = 0x0100,  // Consider civilians to be primary target?
  THREAT_CAPTURE = 0x0200,    // Consider capturable buildings only?
  THREAT_FAKES = 0x0400,      // Consider fake buildings a greater target?
  THREAT_POWER =
      0x0800,  // Consider power generating facilities a greater target?
  THREAT_FACTORIES = 0x1000,  // Consider factories a greater target?
  THREAT_BASE_DEFENSE =
      0x2000  // Consider base defense buildings a greater target?
};
using enum ThreatType;

inline constexpr ThreatType kThreatGround =
    THREAT_VEHICLES | THREAT_BUILDINGS | THREAT_INFANTRY;

/**********************************************************************
**	These return values are used when determine if firing is legal.
**	By examining this value it can be determined what should be done
**	to fix the reason why firing wasn't allowed.
*/
enum class FireErrorType {
  FIRE_OK,        // Weapon is allowed to fire.
  FIRE_AMMO,      // No ammo available to fire?
  FIRE_FACING,    // Not correctly facing target?
  FIRE_REARM,     // It is busy rearming?
  FIRE_ROTATING,  // Is it in process of rotating?
  FIRE_ILLEGAL,   // Is it targeting something illegal?
  FIRE_CANT,      // Is this unit one that cannot fire anything?
  FIRE_MOVING,    // Is it moving and not allowed to fire while moving?
  FIRE_RANGE,     // Is the target out of range?
  FIRE_CLOAKED,   // Is the shooter currently cloaked?
  FIRE_BUSY       // Is shooter currently doing something else?
};
using enum FireErrorType;

/**********************************************************************
**	If an object can cloak, then it will be in one of these states.
**	For objects that cannot cloak, they will always be in the
**	UNCLOAKED state. This state controls how the object transitions between
**	cloaked and uncloaked conditions.
*/
enum class CloakType {
  UNCLOAKED,  // Completely visible (normal state).
  CLOAKING,   // In process of cloaking.
  CLOAKED,    // Completely cloaked (invisible).
  UNCLOAKING  // In process of uncloaking.
};
using enum CloakType;

/**********************************************************************
**	For units that are cloaking, these value specify the visual character
**	of the object.
*/
enum class VisualType {
  VISUAL_NORMAL,      // Completely visible -- normal.
  VISUAL_INDISTINCT,  // The edges shimmer and become indistinct.
  VISUAL_DARKEN,      // Color and texture is muted along with shimmering.
  VISUAL_SHADOWY,     // Body is translucent in addition to shimmering.
  VISUAL_RIPPLE,      // Just a ripple (true predator effect).
  VISUAL_HIDDEN       // Nothing at all is visible.
};
using enum VisualType;

/**********************************************************************
**	These missions enumerate the various state machines that can apply to
**	a game object. Only one of these state machines is active at any one
**	time.
*/
enum class MissionType {
  MISSION_NONE = -1,

  MISSION_SLEEP,           // Do nothing whatsoever.
  MISSION_ATTACK,          // Attack nearest enemy.
  MISSION_MOVE,            // Guard location or unit.
  MISSION_QMOVE,           // A queue list movement mission.
  MISSION_RETREAT,         // Return home for R & R.
  MISSION_GUARD,           // Stay still.
  MISSION_STICKY,          // Stay still -- never recruit.
  MISSION_ENTER,           // Move into object cooperatively.
  MISSION_CAPTURE,         // Move into in order to capture.
  MISSION_HARVEST,         // Hunt for and collect nearby Tiberium.
  MISSION_GUARD_AREA,      // Active guard of area.
  MISSION_RETURN,          // Head back to refinery.
  MISSION_STOP,            // Sit still.
  MISSION_AMBUSH,          // Wait until discovered.
  MISSION_HUNT,            // Active search and destroy.
  MISSION_UNLOAD,          // Search for and deliver cargo.
  MISSION_SABOTAGE,        // Move into in order to destroy.
  MISSION_CONSTRUCTION,    // Building buildup operation.
  MISSION_DECONSTRUCTION,  // Building builddown operation.
  MISSION_REPAIR,          // Repair process mission.
  MISSION_RESCUE,
  MISSION_MISSILE,
  MISSION_HARMLESS  // Sit around and don't appear like a threat.
};
using enum MissionType;

/**********************************************************************
**	These are the enumerated animation sequences that a building may
**	be processing. These serve to control the way that a building
**	appears.
*/
enum class BStateType {
  BSTATE_NONE = -1,
  BSTATE_CONSTRUCTION,  // Construction animation.
  BSTATE_IDLE,          // Idle animation.
  BSTATE_ACTIVE,        // Animation when building is "doing its thing".
  BSTATE_FULL,          // Special alternate active state.
  BSTATE_AUX1,          // Auxiliary animation.
  BSTATE_AUX2           // Auxiliary animation.
};
using enum BStateType;

/**********************************************************************
**	Whenever a unit is selected and a click occurs over another object
**	or terrain element, there is some action to initiate. This specifies
**	the different types of actions possible. This also controls how the
**	mouse cursor looks when "hovering" over the spot that clicking would
**	occur at.
*/
enum class ActionType {
  ACTION_NONE,    // Either undefined action or "do nothing".
  ACTION_MOVE,    // Can move there or at least try to.
  ACTION_NOMOVE,  // Special case for movable object, but illegal mouse
                  // position.
  ACTION_ENTER,   // Special case for infantry->APC or vehicle->Repair facility.
  ACTION_SELF,    // Self select special case.
  ACTION_ATTACK,  // Can attack or fire upon it in some fashion.
  ACTION_HARVEST,        // Special harvest mode.
  ACTION_SELECT,         // Would change selection to specified object.
  ACTION_TOGGLE_SELECT,  // Toggles select state of the object.
  ACTION_CAPTURE,        // The unit will try to capture the object.
  ACTION_REPAIR,         // The target object should be repaired.
  ACTION_SELL,           // The target building should be sold back.
  ACTION_SELL_UNIT,      // The target unit should be sold back.
  ACTION_NO_SELL,        // No sell or no repair.
  ACTION_NO_REPAIR,      // No sell or no repair.
  ACTION_SABOTAGE,       // The unit will try to sabotage/destroy the object.
  ACTION_PARA_BOMB,      // Parachute bomb strike.
  ACTION_PARA_INFANTRY,  // Parachute infantry strike.
  ACTION_PARA_SABOTEUR,  // Parachute saboteur strike.
  ACTION_NUKE_BOMB,      // That target object should be blasted.
  ACTION_AIR_STRIKE,     // That target object should be blasted.
  ACTION_CHRONOSPHERE,   // That target object should be teleported.
  ACTION_CHRONO2,        // Teleport it to the given coordinates now.
  ACTION_IRON_CURTAIN,   // That target object should be invulnerable.
  ACTION_SPY_MISSION,    // Photo recon mission.
  ACTION_GUARD_AREA,     // Guard the area/object clicked on.
  ACTION_HEAL,           // Heal the infantryman clicked on.
  ACTION_DAMAGE,         // Enter and damage building.
  ACTION_GREPAIR,        // Enter and complete repair building.
  ACTION_NO_DEPLOY,
  ACTION_NO_ENTER,
  ACTION_NO_GREPAIR
};
using enum ActionType;

/**********************************************************************
**	When a unit gets damaged, the result of the damage is returned as
**	this type. It can range from no damage taken to complete destruction.
*/
enum class ResultType {
  RESULT_NONE,   // No damage was taken by the target.
  RESULT_LIGHT,  // Some damage was taken, but no state change occurred.
  RESULT_HALF,  // Damaged to below half strength (only returned on transition).
  RESULT_MAJOR,     // Damaged down to 1 hit point.
  RESULT_DESTROYED  // Damaged to complete destruction.
};
using enum ResultType;

/**********************************************************************
**	Units that move can move at different speeds. These enumerate the
**	different speeds that a unit can move.
*/
enum class MPHType {
  MPH_IMMOBILE = 0,
  MPH_VERY_SLOW = 5,       //	2
  MPH_KINDA_SLOW = 6,      //	3
  MPH_SLOW = 8,            //	4
  MPH_SLOW_ISH = 10,       // 5
  MPH_MEDIUM_SLOW = 12,    // 6
  MPH_MEDIUM = 18,         // 9
  MPH_MEDIUM_FAST = 30,    // 12
  MPH_MEDIUM_FASTER = 35,  // 14
  MPH_FAST = 40,           // 16
  MPH_ROCKET = 60,         // 24
  MPH_VERY_FAST = 100,     // 40
  MPH_LIGHT_SPEED = 255    // 100
};
using enum MPHType;

/**********************************************************************
**	The houses that can be played are listed here. Each has their own
**	personality and strengths.
*/
enum class HousesType : int8_t {
  HOUSE_NONE = -1,
  HOUSE_SPAIN,    // Gold (unremapped)
  HOUSE_GREECE,   // LtBlue
  HOUSE_USSR,     // Red
  HOUSE_ENGLAND,  // Green
  HOUSE_UKRAINE,  // Orange
  HOUSE_GERMANY,  // Grey
  HOUSE_FRANCE,   // Blue
  HOUSE_TURKEY,   // Brown
  HOUSE_GOOD,     // Global Defense Initiative
  HOUSE_BAD,      // Brotherhood of Nod
  HOUSE_NEUTRAL,  // Civilians
  HOUSE_JP,       // Disaster Containment Team
  HOUSE_MULTI1,   // Multi-Player house #1
  HOUSE_MULTI2,   // Multi-Player house #2
  HOUSE_MULTI3,   // Multi-Player house #3
  HOUSE_MULTI4,   // Multi-Player house #4
  HOUSE_MULTI5,   // Multi-Player house #5
  HOUSE_MULTI6,   // Multi-Player house #6
  HOUSE_MULTI7,   // Multi-Player house #7
  HOUSE_MULTI8    // Multi-Player house #8
};
using enum HousesType;

// House bit masks over HousesType, for owner lists.
inline constexpr uint32_t kHouseFlagEngland =
    base::Bit<uint32_t>(HOUSE_ENGLAND);
inline constexpr uint32_t kHouseFlagSpain = base::Bit<uint32_t>(HOUSE_SPAIN);
inline constexpr uint32_t kHouseFlagGreece = base::Bit<uint32_t>(HOUSE_GREECE);
inline constexpr uint32_t kHouseFlagUssr = base::Bit<uint32_t>(HOUSE_USSR);
inline constexpr uint32_t kHouseFlagUkraine =
    base::Bit<uint32_t>(HOUSE_UKRAINE);
inline constexpr uint32_t kHouseFlagGermany =
    base::Bit<uint32_t>(HOUSE_GERMANY);
inline constexpr uint32_t kHouseFlagFrance = base::Bit<uint32_t>(HOUSE_FRANCE);
inline constexpr uint32_t kHouseFlagTurkey = base::Bit<uint32_t>(HOUSE_TURKEY);
inline constexpr uint32_t kHouseFlagGood = base::Bit<uint32_t>(HOUSE_GOOD);
inline constexpr uint32_t kHouseFlagBad = base::Bit<uint32_t>(HOUSE_BAD);
inline constexpr uint32_t kHouseFlagNeutral =
    base::Bit<uint32_t>(HOUSE_NEUTRAL);
inline constexpr uint32_t kHouseFlagJp = base::Bit<uint32_t>(HOUSE_JP);
inline constexpr uint32_t kHouseFlagMulti1 = base::Bit<uint32_t>(HOUSE_MULTI1);
inline constexpr uint32_t kHouseFlagMulti2 = base::Bit<uint32_t>(HOUSE_MULTI2);
inline constexpr uint32_t kHouseFlagMulti3 = base::Bit<uint32_t>(HOUSE_MULTI3);
inline constexpr uint32_t kHouseFlagMulti4 = base::Bit<uint32_t>(HOUSE_MULTI4);
inline constexpr uint32_t kHouseFlagMulti5 = base::Bit<uint32_t>(HOUSE_MULTI5);
inline constexpr uint32_t kHouseFlagMulti6 = base::Bit<uint32_t>(HOUSE_MULTI6);
inline constexpr uint32_t kHouseFlagMulti7 = base::Bit<uint32_t>(HOUSE_MULTI7);
inline constexpr uint32_t kHouseFlagMulti8 = base::Bit<uint32_t>(HOUSE_MULTI8);
inline constexpr uint32_t kHouseFlagNone = 0;
inline constexpr uint32_t kHouseFlagAllies =
    kHouseFlagEngland | kHouseFlagSpain | kHouseFlagGreece | kHouseFlagGermany |
    kHouseFlagFrance | kHouseFlagTurkey | kHouseFlagGood;
inline constexpr uint32_t kHouseFlagSoviet =
    kHouseFlagUssr | kHouseFlagUkraine | kHouseFlagBad;
inline constexpr uint32_t kHouseFlagOthers =
    kHouseFlagNeutral | kHouseFlagJp | kHouseFlagMulti1 | kHouseFlagMulti2 |
    kHouseFlagMulti3 | kHouseFlagMulti4 | kHouseFlagMulti5 | kHouseFlagMulti6 |
    kHouseFlagMulti7 | kHouseFlagMulti8;

enum class PlayerColorType {
  PCOLOR_NONE = -1,
  PCOLOR_GOLD,
  PCOLOR_LTBLUE,
  PCOLOR_RED,
  PCOLOR_GREEN,
  PCOLOR_ORANGE,
  PCOLOR_GREY,
  PCOLOR_BLUE,  // This is actually the red scheme used in the dialogs
  PCOLOR_BROWN,
  PCOLOR_TYPE,
  PCOLOR_REALLY_BLUE,
  PCOLOR_DIALOG_BLUE
};
using enum PlayerColorType;

/**********************************************************************
**	This enumerates the remap logic to be applied to an object type when
**	it appears in the construction sidebar.
*/
enum class RemapType { REMAP_NONE, REMAP_NORMAL, REMAP_ALTERNATE };
using enum RemapType;

/**********************************************************************
** These are the types of games that can be played.  GDI & NOD are the
** usual human-vs-computer games; 2-Player games are network or modem,
** with 2 players; multi-player games are network with > 2 players.
*/
enum class ScenarioPlayerType {
  SCEN_PLAYER_NONE = -1,
  SCEN_PLAYER_SPAIN,
  SCEN_PLAYER_GREECE,
  SCEN_PLAYER_USSR,
  SCEN_PLAYER_JP,
  SCEN_PLAYER_2PLAYER,
  SCEN_PLAYER_MPLAYER
};
using enum ScenarioPlayerType;

/**********************************************************************
** These are the directional parameters for a scenario.
*/
enum class ScenarioDirType { SCEN_DIR_NONE = -1, SCEN_DIR_EAST, SCEN_DIR_WEST };
using enum ScenarioDirType;

/**********************************************************************
** These are the random variations of a scenario.
*/
enum class ScenarioVarType {
  SCEN_VAR_NONE = -1,
  SCEN_VAR_A,
  SCEN_VAR_B,
  SCEN_VAR_C,
  SCEN_VAR_D,
  SCEN_VAR_LOSE
};
using enum ScenarioVarType;

/**********************************************************************
**	The objects to be drawn on the map are grouped into layers. These
**	enumerated values specify those layers. The ground layer is sorted
**	from back to front.
*/
enum class LayerType {
  LAYER_NONE = -1,
  LAYER_SURFACE,  // Flat on the ground (no sorting or apparent vertical
                  // height).
  LAYER_GROUND,   // Touching the ground type object (units & buildings).
  LAYER_AIR,      // Flying above the ground (explosions & flames).
  LAYER_TOP       // Topmost layer (aircraft & bullets).
};
using enum LayerType;

/**********************************************************************
**	This enumerates the various bullet types. These types specify bullet's
**	visual and explosive characteristics.
*/
enum class BulletType {
  BULLET_NONE = -1,

  BULLET_INVISIBLE,
  BULLET_CANNON,
  BULLET_ACK,
  BULLET_TORPEDO,
  BULLET_FROG,
  BULLET_HEAT_SEEKER,
  BULLET_LASER_GUIDED,
  BULLET_LOBBED,
  BULLET_BOMBLET,
  BULLET_BALLISTIC,
  BULLET_PARACHUTE,
  BULLET_FIREBALL,
  BULLET_DOG,
  BULLET_CATAPULT,
  BULLET_AAMISSILE,
  BULLET_GPS_SATELLITE,
  BULLET_NUKE_UP,
  BULLET_NUKE_DOWN
};
using enum BulletType;

/**********************************************************************
**	All game buildings (structures) are enumerated here. This includes
**	civilian structures as well.
*/
enum class StructType {
  STRUCT_NONE = -1,
  STRUCT_ADVANCED_TECH,
  STRUCT_IRON_CURTAIN,
  STRUCT_WEAP,
  STRUCT_CHRONOSPHERE,
  STRUCT_PILLBOX,
  STRUCT_CAMOPILLBOX,
  STRUCT_RADAR,
  STRUCT_GAP,
  STRUCT_TURRET,
  STRUCT_AAGUN,
  STRUCT_FLAME_TURRET,
  STRUCT_CONST,
  STRUCT_REFINERY,
  STRUCT_STORAGE,
  STRUCT_HELIPAD,
  STRUCT_SAM,
  STRUCT_AIRSTRIP,
  STRUCT_POWER,
  STRUCT_ADVANCED_POWER,
  STRUCT_SOVIET_TECH,
  STRUCT_HOSPITAL,
  STRUCT_BARRACKS,
  STRUCT_TENT,
  STRUCT_KENNEL,
  STRUCT_REPAIR,
  STRUCT_BIO_LAB,
  STRUCT_MISSION,
  STRUCT_SHIP_YARD,
  STRUCT_SUB_PEN,
  STRUCT_MSLO,
  STRUCT_FORWARD_COM,
  STRUCT_TESLA,

  /*
  **	All buildings that are never used as a prerequisite
  **	for construction, follow this point. Typically, this is
  **	limited to civilian structures. Also, the following
  **	buildings are NEVER used in the availability bit field
  **	record that each house maintains, i.e. no kStructFlag mask
  **	bit checking will never occur with the following
  **	building types.
  */
  STRUCT_FAKEWEAP,
  STRUCT_FAKECONST,
  STRUCT_FAKE_YARD,
  STRUCT_FAKE_PEN,
  STRUCT_FAKE_RADAR,

  STRUCT_SANDBAG_WALL,
  STRUCT_CYCLONE_WALL,
  STRUCT_BRICK_WALL,
  STRUCT_BARBWIRE_WALL,
  STRUCT_WOOD_WALL,
  STRUCT_FENCE,

  STRUCT_AVMINE,
  STRUCT_APMINE,
  STRUCT_V01,
  STRUCT_V02,
  STRUCT_V03,
  STRUCT_V04,
  STRUCT_V05,
  STRUCT_V06,
  STRUCT_V07,
  STRUCT_V08,
  STRUCT_V09,
  STRUCT_V10,
  STRUCT_V11,
  STRUCT_V12,
  STRUCT_V13,
  STRUCT_V14,
  STRUCT_V15,
  STRUCT_V16,
  STRUCT_V17,
  STRUCT_V18,
  STRUCT_PUMP,
  STRUCT_V20,
  STRUCT_V21,
  STRUCT_V22,
  STRUCT_V23,
  STRUCT_V24,
  STRUCT_V25,
  STRUCT_V26,
  STRUCT_V27,
  STRUCT_V28,
  STRUCT_V29,
  STRUCT_V30,
  STRUCT_V31,
  STRUCT_V32,
  STRUCT_V33,
  STRUCT_V34,
  STRUCT_V35,
  STRUCT_V36,
  STRUCT_V37,
  STRUCT_BARREL,
  STRUCT_BARREL3,

  STRUCT_QUEEN,
  STRUCT_LARVA1,
  STRUCT_LARVA2
};
using enum StructType;

// The bit for `type` in the 64-bit house scans (HouseClass::BScan and the
// unit, infantry, aircraft and vessel scans), or 0 for a building type past
// the 64 the scans can hold: the civilian buildings from STRUCT_V14 on, the
// barrels and the ant structures, which no prerequisite, trigger or AI test
// names. The original shifted by the full index and wrapped.
constexpr uint64_t ScanBit(int type) noexcept {
  return type < 64 ? base::Bit<uint64_t>(type) : uint64_t{0};
}

// Building bit masks over StructType, matching HouseClass::BScan. The enum
// has more than 32 entries, so the masks are 64 bits wide.
inline constexpr uint64_t kStructFlagNone = 0;
inline constexpr uint64_t kStructFlagAdvancedTech =
    base::Bit<uint64_t>(STRUCT_ADVANCED_TECH);
inline constexpr uint64_t kStructFlagIronCurtain =
    base::Bit<uint64_t>(STRUCT_IRON_CURTAIN);
inline constexpr uint64_t kStructFlagWeap = base::Bit<uint64_t>(STRUCT_WEAP);
inline constexpr uint64_t kStructFlagChronosphere =
    base::Bit<uint64_t>(STRUCT_CHRONOSPHERE);
inline constexpr uint64_t kStructFlagRadar = base::Bit<uint64_t>(STRUCT_RADAR);
inline constexpr uint64_t kStructFlagConst = base::Bit<uint64_t>(STRUCT_CONST);
inline constexpr uint64_t kStructFlagRefinery =
    base::Bit<uint64_t>(STRUCT_REFINERY);
inline constexpr uint64_t kStructFlagHelipad =
    base::Bit<uint64_t>(STRUCT_HELIPAD);
inline constexpr uint64_t kStructFlagSam = base::Bit<uint64_t>(STRUCT_SAM);
inline constexpr uint64_t kStructFlagAirstrip =
    base::Bit<uint64_t>(STRUCT_AIRSTRIP);
inline constexpr uint64_t kStructFlagPower = base::Bit<uint64_t>(STRUCT_POWER);
inline constexpr uint64_t kStructFlagAdvancedPower =
    base::Bit<uint64_t>(STRUCT_ADVANCED_POWER);
inline constexpr uint64_t kStructFlagSovietTech =
    base::Bit<uint64_t>(STRUCT_SOVIET_TECH);
inline constexpr uint64_t kStructFlagBarracks =
    base::Bit<uint64_t>(STRUCT_BARRACKS);
inline constexpr uint64_t kStructFlagTent = base::Bit<uint64_t>(STRUCT_TENT);
inline constexpr uint64_t kStructFlagRepair =
    base::Bit<uint64_t>(STRUCT_REPAIR);
inline constexpr uint64_t kStructFlagMslo = base::Bit<uint64_t>(STRUCT_MSLO);
inline constexpr uint64_t kStructFlagFakeConst =
    base::Bit<uint64_t>(STRUCT_FAKECONST);
inline constexpr uint64_t kStructFlagFakeWeap =
    base::Bit<uint64_t>(STRUCT_FAKEWEAP);

/**********************************************************************
**	The overlays are enumerated here. An overlay functions similarly to
**	a transparent icon. It is placed over the terrain but usually falls
**	"under" buildings, trees, and units.
*/
enum class OverlayType : int8_t {
  OVERLAY_NONE = -1,
  OVERLAY_SANDBAG_WALL,   // Piled sandbags.
  OVERLAY_CYCLONE_WALL,   // Chain-link fence.
  OVERLAY_BRICK_WALL,     // Solid concrete wall.
  OVERLAY_BARBWIRE_WALL,  // Barbed-wire wall.
  OVERLAY_WOOD_WALL,      // Wooden fence.
  OVERLAY_GOLD1,
  OVERLAY_GOLD2,
  OVERLAY_GOLD3,
  OVERLAY_GOLD4,
  OVERLAY_GEMS1,
  OVERLAY_GEMS2,
  OVERLAY_GEMS3,
  OVERLAY_GEMS4,
  OVERLAY_V12,          // Haystacks
  OVERLAY_V13,          // Haystack
  OVERLAY_V14,          // Wheat field
  OVERLAY_V15,          // Fallow field
  OVERLAY_V16,          //	Corn field
  OVERLAY_V17,          // Celery field
  OVERLAY_V18,          // Potato field
  OVERLAY_FLAG_SPOT,    // Flag start location.
  OVERLAY_WOOD_CRATE,   // Wooden goodie crate.
  OVERLAY_STEEL_CRATE,  //	Steel goodie crate.
  OVERLAY_FENCE,        // New fangled fence.
  OVERLAY_WATER_CRATE   //	Water goodie crate.
};
using enum OverlayType;

/**********************************************************************
**	This specifies the infantry in the game. The "E" designation is
**	similar to the army classification of enlisted soldiers.
*/
enum class InfantryType {
  INFANTRY_NONE = -1,
  INFANTRY_E1,         // Mini-gun armed.
  INFANTRY_E2,         // Grenade thrower.
  INFANTRY_E3,         // Rocket launcher.
  INFANTRY_E4,         // Flame thrower equipped.
  INFANTRY_RENOVATOR,  // Engineer.
  INFANTRY_TANYA,      // Saboteur.
  INFANTRY_SPY,        // Spy.
  INFANTRY_THIEF,      // Thief.
  INFANTRY_MEDIC,      // Field Medic.
  INFANTRY_GENERAL,    // Field Marshal.
  INFANTRY_DOG,        // Soviet attack dog

  INFANTRY_C1,        // Civilian
  INFANTRY_C2,        // Civilian
  INFANTRY_C3,        // Civilian
  INFANTRY_C4,        // Civilian
  INFANTRY_C5,        // Civilian
  INFANTRY_C6,        // Civilian
  INFANTRY_C7,        // Civilian
  INFANTRY_C8,        // Civilian
  INFANTRY_C9,        // Civilian
  INFANTRY_C10,       // Nikumba
  INFANTRY_EINSTEIN,  // Einstein
  INFANTRY_DELPHI,    // Agent "Delphi"
  INFANTRY_CHAN,      // Dr. Chan

  // CounterStrike II only!
  INFANTRY_SHOCK,  // Shock Trooper
  INFANTRY_MECHANIC
};
using enum InfantryType;

// Infantry bit mask over InfantryType, matching HouseClass::IScan.
inline constexpr uint64_t kInfantryFlagDog = base::Bit<uint64_t>(INFANTRY_DOG);

/**********************************************************************
**	The game units are enumerated here. These include not only traditional
**	vehicles, but also hovercraft and gunboats.
*/
enum class UnitType {
  UNIT_NONE = -1,
  UNIT_HTANK,        // Mammoth tank.
  UNIT_MTANK,        // Heavy tank.
  UNIT_MTANK2,       // Medium tank.
  UNIT_LTANK,        // Light tank ('Bradly').
  UNIT_APC,          // APC.
  UNIT_MINELAYER,    // Mine-laying vehicle.
  UNIT_JEEP,         // 4x4 jeep replacement.
  UNIT_HARVESTER,    // Resource gathering vehicle.
  UNIT_ARTY,         // Artillery unit.
  UNIT_MRJ,          // Mobile Radar Jammer.
  UNIT_MGG,          // Mobile Gap Generator
  UNIT_MCV,          // Mobile construction vehicle.
  UNIT_V2_LAUNCHER,  // V2 rocket launcher.
  UNIT_TRUCK,        // Convoy truck

  UNIT_ANT1,  // Warrior ant.
  UNIT_ANT2,  // Warrior ant.
  UNIT_ANT3,  // Warrior ant.

  // CS II ONLY!
  UNIT_CHRONOTANK,  // Chrono-shifting tank
  UNIT_TESLATANK,   // Tesla-equipped tank
  UNIT_MAD,         // Timequake tank
  UNIT_DEMOTRUCK,   // Jihad truck
  UNIT_PHASE        // cloaking APC for special missions
};
using enum UnitType;

// Unit bit masks over UnitType, matching HouseClass::UScan.
inline constexpr uint64_t kUnitFlagHarvester =
    base::Bit<uint64_t>(UNIT_HARVESTER);
inline constexpr uint64_t kUnitFlagMcv = base::Bit<uint64_t>(UNIT_MCV);

/**********************************************************************
**	The naval vessels are enumerated below.
*/
enum class VesselType {
  VESSEL_NONE = -1,

  VESSEL_SS,         // Submarine
  VESSEL_DD,         // Medium weapon patrol craft
  VESSEL_CA,         // Heavy weapon patrol craft
  VESSEL_TRANSPORT,  // Unit transporter
  VESSEL_PT,         // Light weapon patrol craft

  // CS II ONLY
  VESSEL_MISSILESUB,  // Missile-equipped submarine
  VESSEL_CARRIER
};
using enum VesselType;

/**********************************************************************
**	The various aircraft types are enumerated here. These include
*helicopters *	as well as traditional aircraft.
*/
enum class AircraftType {
  AIRCRAFT_TRANSPORT = 0,  // Transport helicopter.
  AIRCRAFT_BADGER = 1,     // Badger bomber.
  AIRCRAFT_U2 = 2,         // Photo recon plane.
  AIRCRAFT_MIG = 3,        // Mig attack plane.
  AIRCRAFT_YAK = 4,        // Yak attack plane.
  AIRCRAFT_LONGBOW = 5,    // Apache gunship.
  AIRCRAFT_HIND = 6,       // Soviet attach helicopter.

  AIRCRAFT_NONE = -1
};
using enum AircraftType;

/**********************************************************************
**	The game templates are enumerated here. These are the underlying
**	terrain art. This includes everything from water to cliffs. If the
**	terrain is broken up into icons, is not transparent, and is drawn
**	as the bottom most layer, then it is a template.
*/
enum class TemplateType : uint16_t {
  TEMPLATE_CLEAR1 = 0,
  TEMPLATE_WATER = 1,  // This must be the first non-clear template.
  TEMPLATE_WATER2 = 2,
  TEMPLATE_SHORE01 = 3,
  TEMPLATE_SHORE02 = 4,
  TEMPLATE_SHORE03 = 5,
  TEMPLATE_SHORE04 = 6,
  TEMPLATE_SHORE05 = 7,
  TEMPLATE_SHORE06 = 8,
  TEMPLATE_SHORE07 = 9,
  TEMPLATE_SHORE08 = 10,
  TEMPLATE_SHORE09 = 11,
  TEMPLATE_SHORE10 = 12,
  TEMPLATE_SHORE11 = 13,
  TEMPLATE_SHORE12 = 14,
  TEMPLATE_SHORE13 = 15,
  TEMPLATE_SHORE14 = 16,
  TEMPLATE_SHORE15 = 17,
  TEMPLATE_SHORE16 = 18,
  TEMPLATE_SHORE17 = 19,
  TEMPLATE_SHORE18 = 20,
  TEMPLATE_SHORE19 = 21,
  TEMPLATE_SHORE20 = 22,
  TEMPLATE_SHORE21 = 23,
  TEMPLATE_SHORE22 = 24,
  TEMPLATE_SHORE23 = 25,
  TEMPLATE_SHORE24 = 26,
  TEMPLATE_SHORE25 = 27,
  TEMPLATE_SHORE26 = 28,
  TEMPLATE_SHORE27 = 29,
  TEMPLATE_SHORE28 = 30,
  TEMPLATE_SHORE29 = 31,
  TEMPLATE_SHORE30 = 32,
  TEMPLATE_SHORE31 = 33,
  TEMPLATE_SHORE32 = 34,
  TEMPLATE_SHORE33 = 35,
  TEMPLATE_SHORE34 = 36,
  TEMPLATE_SHORE35 = 37,
  TEMPLATE_SHORE36 = 38,
  TEMPLATE_SHORE37 = 39,
  TEMPLATE_SHORE38 = 40,
  TEMPLATE_SHORE39 = 41,
  TEMPLATE_SHORE40 = 42,
  TEMPLATE_SHORE41 = 43,
  TEMPLATE_SHORE42 = 44,
  TEMPLATE_SHORE43 = 45,
  TEMPLATE_SHORE44 = 46,
  TEMPLATE_SHORE45 = 47,
  TEMPLATE_SHORE46 = 48,
  TEMPLATE_SHORE47 = 49,
  TEMPLATE_SHORE48 = 50,
  TEMPLATE_SHORE49 = 51,
  TEMPLATE_SHORE50 = 52,
  TEMPLATE_SHORE51 = 53,
  TEMPLATE_SHORE52 = 54,
  TEMPLATE_SHORE53 = 55,
  TEMPLATE_SHORE54 = 56,
  TEMPLATE_SHORE55 = 57,
  TEMPLATE_SHORE56 = 58,
  TEMPLATE_SHORECLIFF01 = 59,
  TEMPLATE_SHORECLIFF02 = 60,
  TEMPLATE_SHORECLIFF03 = 61,
  TEMPLATE_SHORECLIFF04 = 62,
  TEMPLATE_SHORECLIFF05 = 63,
  TEMPLATE_SHORECLIFF06 = 64,
  TEMPLATE_SHORECLIFF07 = 65,
  TEMPLATE_SHORECLIFF08 = 66,
  TEMPLATE_SHORECLIFF09 = 67,
  TEMPLATE_SHORECLIFF10 = 68,
  TEMPLATE_SHORECLIFF11 = 69,
  TEMPLATE_SHORECLIFF12 = 70,
  TEMPLATE_SHORECLIFF13 = 71,
  TEMPLATE_SHORECLIFF14 = 72,
  TEMPLATE_SHORECLIFF15 = 73,
  TEMPLATE_SHORECLIFF16 = 74,
  TEMPLATE_SHORECLIFF17 = 75,
  TEMPLATE_SHORECLIFF18 = 76,
  TEMPLATE_SHORECLIFF19 = 77,
  TEMPLATE_SHORECLIFF20 = 78,
  TEMPLATE_SHORECLIFF21 = 79,
  TEMPLATE_SHORECLIFF22 = 80,
  TEMPLATE_SHORECLIFF23 = 81,
  TEMPLATE_SHORECLIFF24 = 82,
  TEMPLATE_SHORECLIFF25 = 83,
  TEMPLATE_SHORECLIFF26 = 84,
  TEMPLATE_SHORECLIFF27 = 85,
  TEMPLATE_SHORECLIFF28 = 86,
  TEMPLATE_SHORECLIFF29 = 87,
  TEMPLATE_SHORECLIFF30 = 88,
  TEMPLATE_SHORECLIFF31 = 89,
  TEMPLATE_SHORECLIFF32 = 90,
  TEMPLATE_SHORECLIFF33 = 91,
  TEMPLATE_SHORECLIFF34 = 92,
  TEMPLATE_SHORECLIFF35 = 93,
  TEMPLATE_SHORECLIFF36 = 94,
  TEMPLATE_SHORECLIFF37 = 95,
  TEMPLATE_SHORECLIFF38 = 96,
  TEMPLATE_BOULDER1 = 97,
  TEMPLATE_BOULDER2 = 98,
  TEMPLATE_BOULDER3 = 99,
  TEMPLATE_BOULDER4 = 100,
  TEMPLATE_BOULDER5 = 101,
  TEMPLATE_BOULDER6 = 102,
  TEMPLATE_PATCH01 = 103,
  TEMPLATE_PATCH02 = 104,
  TEMPLATE_PATCH03 = 105,
  TEMPLATE_PATCH04 = 106,
  TEMPLATE_PATCH07 = 107,
  TEMPLATE_PATCH08 = 108,
  TEMPLATE_PATCH13 = 109,
  TEMPLATE_PATCH14 = 110,
  TEMPLATE_PATCH15 = 111,
  TEMPLATE_RIVER01 = 112,
  TEMPLATE_RIVER02 = 113,
  TEMPLATE_RIVER03 = 114,
  TEMPLATE_RIVER04 = 115,
  TEMPLATE_RIVER05 = 116,
  TEMPLATE_RIVER06 = 117,
  TEMPLATE_RIVER07 = 118,
  TEMPLATE_RIVER08 = 119,
  TEMPLATE_RIVER09 = 120,
  TEMPLATE_RIVER10 = 121,
  TEMPLATE_RIVER11 = 122,
  TEMPLATE_RIVER12 = 123,
  TEMPLATE_RIVER13 = 124,
  TEMPLATE_FALLS1 = 125,
  TEMPLATE_FALLS1A = 126,
  TEMPLATE_FALLS2 = 127,
  TEMPLATE_FALLS2A = 128,
  TEMPLATE_FORD1 = 129,
  TEMPLATE_FORD2 = 130,
  TEMPLATE_BRIDGE1 = 131,
  TEMPLATE_BRIDGE1D = 132,
  TEMPLATE_BRIDGE2 = 133,
  TEMPLATE_BRIDGE2D = 134,
  TEMPLATE_SLOPE01 = 135,
  TEMPLATE_SLOPE02 = 136,
  TEMPLATE_SLOPE03 = 137,
  TEMPLATE_SLOPE04 = 138,
  TEMPLATE_SLOPE05 = 139,
  TEMPLATE_SLOPE06 = 140,
  TEMPLATE_SLOPE07 = 141,
  TEMPLATE_SLOPE08 = 142,
  TEMPLATE_SLOPE09 = 143,
  TEMPLATE_SLOPE10 = 144,
  TEMPLATE_SLOPE11 = 145,
  TEMPLATE_SLOPE12 = 146,
  TEMPLATE_SLOPE13 = 147,
  TEMPLATE_SLOPE14 = 148,
  TEMPLATE_SLOPE15 = 149,
  TEMPLATE_SLOPE16 = 150,
  TEMPLATE_SLOPE17 = 151,
  TEMPLATE_SLOPE18 = 152,
  TEMPLATE_SLOPE19 = 153,
  TEMPLATE_SLOPE20 = 154,
  TEMPLATE_SLOPE21 = 155,
  TEMPLATE_SLOPE22 = 156,
  TEMPLATE_SLOPE23 = 157,
  TEMPLATE_SLOPE24 = 158,
  TEMPLATE_SLOPE25 = 159,
  TEMPLATE_SLOPE26 = 160,
  TEMPLATE_SLOPE27 = 161,
  TEMPLATE_SLOPE28 = 162,
  TEMPLATE_SLOPE29 = 163,
  TEMPLATE_SLOPE30 = 164,
  TEMPLATE_SLOPE31 = 165,
  TEMPLATE_SLOPE32 = 166,
  TEMPLATE_SLOPE33 = 167,
  TEMPLATE_SLOPE34 = 168,
  TEMPLATE_SLOPE35 = 169,
  TEMPLATE_SLOPE36 = 170,
  TEMPLATE_SLOPE37 = 171,
  TEMPLATE_SLOPE38 = 172,
  TEMPLATE_ROAD01 = 173,
  TEMPLATE_ROAD02 = 174,
  TEMPLATE_ROAD03 = 175,
  TEMPLATE_ROAD04 = 176,
  TEMPLATE_ROAD05 = 177,
  TEMPLATE_ROAD06 = 178,
  TEMPLATE_ROAD07 = 179,
  TEMPLATE_ROAD08 = 180,
  TEMPLATE_ROAD09 = 181,
  TEMPLATE_ROAD10 = 182,
  TEMPLATE_ROAD11 = 183,
  TEMPLATE_ROAD12 = 184,
  TEMPLATE_ROAD13 = 185,
  TEMPLATE_ROAD14 = 186,
  TEMPLATE_ROAD15 = 187,
  TEMPLATE_ROAD16 = 188,
  TEMPLATE_ROAD17 = 189,
  TEMPLATE_ROAD18 = 190,
  TEMPLATE_ROAD19 = 191,
  TEMPLATE_ROAD20 = 192,
  TEMPLATE_ROAD21 = 193,
  TEMPLATE_ROAD22 = 194,
  TEMPLATE_ROAD23 = 195,
  TEMPLATE_ROAD24 = 196,
  TEMPLATE_ROAD25 = 197,
  TEMPLATE_ROAD26 = 198,
  TEMPLATE_ROAD27 = 199,
  TEMPLATE_ROAD28 = 200,
  TEMPLATE_ROAD29 = 201,
  TEMPLATE_ROAD30 = 202,
  TEMPLATE_ROAD31 = 203,
  TEMPLATE_ROAD32 = 204,
  TEMPLATE_ROAD33 = 205,
  TEMPLATE_ROAD34 = 206,
  TEMPLATE_ROAD35 = 207,
  TEMPLATE_ROAD36 = 208,
  TEMPLATE_ROAD37 = 209,
  TEMPLATE_ROAD38 = 210,
  TEMPLATE_ROAD39 = 211,
  TEMPLATE_ROAD40 = 212,
  TEMPLATE_ROAD41 = 213,
  TEMPLATE_ROAD42 = 214,
  TEMPLATE_ROAD43 = 215,
  TEMPLATE_ROUGH01 = 216,
  TEMPLATE_ROUGH02 = 217,
  TEMPLATE_ROUGH03 = 218,
  TEMPLATE_ROUGH04 = 219,
  TEMPLATE_ROUGH05 = 220,
  TEMPLATE_ROUGH06 = 221,
  TEMPLATE_ROUGH07 = 222,
  TEMPLATE_ROUGH08 = 223,
  TEMPLATE_ROUGH09 = 224,
  TEMPLATE_ROUGH10 = 225,
  TEMPLATE_ROUGH11 = 226,
  TEMPLATE_ROAD44 = 227,
  TEMPLATE_ROAD45 = 228,
  TEMPLATE_RIVER14 = 229,
  TEMPLATE_RIVER15 = 230,
  TEMPLATE_RIVERCLIFF01 = 231,
  TEMPLATE_RIVERCLIFF02 = 232,
  TEMPLATE_RIVERCLIFF03 = 233,
  TEMPLATE_RIVERCLIFF04 = 234,
  TEMPLATE_BRIDGE_1A = 235,
  TEMPLATE_BRIDGE_1B = 236,
  TEMPLATE_BRIDGE_1C = 237,
  TEMPLATE_BRIDGE_2A = 238,
  TEMPLATE_BRIDGE_2B = 239,
  TEMPLATE_BRIDGE_2C = 240,
  TEMPLATE_BRIDGE_3A = 241,
  TEMPLATE_BRIDGE_3B = 242,
  TEMPLATE_BRIDGE_3C = 243,
  TEMPLATE_BRIDGE_3D = 244,
  TEMPLATE_BRIDGE_3E = 245,
  TEMPLATE_BRIDGE_3F = 246,
  TEMPLATE_F01 = 247,
  TEMPLATE_F02 = 248,
  TEMPLATE_F03 = 249,
  TEMPLATE_F04 = 250,
  TEMPLATE_F05 = 251,
  TEMPLATE_F06 = 252,

  // Custom interior pieces.
  TEMPLATE_ARRO0001 = 253,
  TEMPLATE_ARRO0002 = 254,
  TEMPLATE_ARRO0003 = 255,
  TEMPLATE_ARRO0004 = 256,
  TEMPLATE_ARRO0005 = 257,
  TEMPLATE_ARRO0006 = 258,
  TEMPLATE_ARRO0007 = 259,
  TEMPLATE_ARRO0008 = 260,
  TEMPLATE_ARRO0009 = 261,
  TEMPLATE_ARRO0010 = 262,
  TEMPLATE_ARRO0011 = 263,
  TEMPLATE_ARRO0012 = 264,
  TEMPLATE_ARRO0013 = 265,
  TEMPLATE_ARRO0014 = 266,
  TEMPLATE_ARRO0015 = 267,
  TEMPLATE_FLOR0001 = 268,
  TEMPLATE_FLOR0002 = 269,
  TEMPLATE_FLOR0003 = 270,
  TEMPLATE_FLOR0004 = 271,
  TEMPLATE_FLOR0005 = 272,
  TEMPLATE_FLOR0006 = 273,
  TEMPLATE_FLOR0007 = 274,
  TEMPLATE_GFLR0001 = 275,
  TEMPLATE_GFLR0002 = 276,
  TEMPLATE_GFLR0003 = 277,
  TEMPLATE_GFLR0004 = 278,
  TEMPLATE_GFLR0005 = 279,
  TEMPLATE_GSTR0001 = 280,
  TEMPLATE_GSTR0002 = 281,
  TEMPLATE_GSTR0003 = 282,
  TEMPLATE_GSTR0004 = 283,
  TEMPLATE_GSTR0005 = 284,
  TEMPLATE_GSTR0006 = 285,
  TEMPLATE_GSTR0007 = 286,
  TEMPLATE_GSTR0008 = 287,
  TEMPLATE_GSTR0009 = 288,
  TEMPLATE_GSTR0010 = 289,
  TEMPLATE_GSTR0011 = 290,
  TEMPLATE_LWAL0001 = 291,
  TEMPLATE_LWAL0002 = 292,
  TEMPLATE_LWAL0003 = 293,
  TEMPLATE_LWAL0004 = 294,
  TEMPLATE_LWAL0005 = 295,
  TEMPLATE_LWAL0006 = 296,
  TEMPLATE_LWAL0007 = 297,
  TEMPLATE_LWAL0008 = 298,
  TEMPLATE_LWAL0009 = 299,
  TEMPLATE_LWAL0010 = 300,
  TEMPLATE_LWAL0011 = 301,
  TEMPLATE_LWAL0012 = 302,
  TEMPLATE_LWAL0013 = 303,
  TEMPLATE_LWAL0014 = 304,
  TEMPLATE_LWAL0015 = 305,
  TEMPLATE_LWAL0016 = 306,
  TEMPLATE_LWAL0017 = 307,
  TEMPLATE_LWAL0018 = 308,
  TEMPLATE_LWAL0019 = 309,
  TEMPLATE_LWAL0020 = 310,
  TEMPLATE_LWAL0021 = 311,
  TEMPLATE_LWAL0022 = 312,
  TEMPLATE_LWAL0023 = 313,
  TEMPLATE_LWAL0024 = 314,
  TEMPLATE_LWAL0025 = 315,
  TEMPLATE_LWAL0026 = 316,
  TEMPLATE_LWAL0027 = 317,
  TEMPLATE_STRP0001 = 318,
  TEMPLATE_STRP0002 = 319,
  TEMPLATE_STRP0003 = 320,
  TEMPLATE_STRP0004 = 321,
  TEMPLATE_STRP0005 = 322,
  TEMPLATE_STRP0006 = 323,
  TEMPLATE_STRP0007 = 324,
  TEMPLATE_STRP0008 = 325,
  TEMPLATE_STRP0009 = 326,
  TEMPLATE_STRP0010 = 327,
  TEMPLATE_STRP0011 = 328,
  TEMPLATE_WALL0001 = 329,
  TEMPLATE_WALL0002 = 330,
  TEMPLATE_WALL0003 = 331,
  TEMPLATE_WALL0004 = 332,
  TEMPLATE_WALL0005 = 333,
  TEMPLATE_WALL0006 = 334,
  TEMPLATE_WALL0007 = 335,
  TEMPLATE_WALL0008 = 336,
  TEMPLATE_WALL0009 = 337,
  TEMPLATE_WALL0010 = 338,
  TEMPLATE_WALL0011 = 339,
  TEMPLATE_WALL0012 = 340,
  TEMPLATE_WALL0013 = 341,
  TEMPLATE_WALL0014 = 342,
  TEMPLATE_WALL0015 = 343,
  TEMPLATE_WALL0016 = 344,
  TEMPLATE_WALL0017 = 345,
  TEMPLATE_WALL0018 = 346,
  TEMPLATE_WALL0019 = 347,
  TEMPLATE_WALL0020 = 348,
  TEMPLATE_WALL0021 = 349,
  TEMPLATE_WALL0022 = 350,
  TEMPLATE_WALL0023 = 351,
  TEMPLATE_WALL0024 = 352,
  TEMPLATE_WALL0025 = 353,
  TEMPLATE_WALL0026 = 354,
  TEMPLATE_WALL0027 = 355,
  TEMPLATE_WALL0028 = 356,
  TEMPLATE_WALL0029 = 357,
  TEMPLATE_WALL0030 = 358,
  TEMPLATE_WALL0031 = 359,
  TEMPLATE_WALL0032 = 360,
  TEMPLATE_WALL0033 = 361,
  TEMPLATE_WALL0034 = 362,
  TEMPLATE_WALL0035 = 363,
  TEMPLATE_WALL0036 = 364,
  TEMPLATE_WALL0037 = 365,
  TEMPLATE_WALL0038 = 366,
  TEMPLATE_WALL0039 = 367,
  TEMPLATE_WALL0040 = 368,
  TEMPLATE_WALL0041 = 369,
  TEMPLATE_WALL0042 = 370,
  TEMPLATE_WALL0043 = 371,
  TEMPLATE_WALL0044 = 372,
  TEMPLATE_WALL0045 = 373,
  TEMPLATE_WALL0046 = 374,
  TEMPLATE_WALL0047 = 375,
  TEMPLATE_WALL0048 = 376,
  TEMPLATE_WALL0049 = 377,
  TEMPLATE_BRIDGE1H = 378,
  TEMPLATE_BRIDGE2H = 379,
  TEMPLATE_BRIDGE_1AX = 380,
  TEMPLATE_BRIDGE_2AX = 381,
  TEMPLATE_BRIDGE1X = 382,
  TEMPLATE_BRIDGE2X = 383,

  TEMPLATE_XTRA0001 = 384,
  TEMPLATE_XTRA0002 = 385,
  TEMPLATE_XTRA0003 = 386,
  TEMPLATE_XTRA0004 = 387,
  TEMPLATE_XTRA0005 = 388,
  TEMPLATE_XTRA0006 = 389,
  TEMPLATE_XTRA0007 = 390,
  TEMPLATE_XTRA0008 = 391,
  TEMPLATE_XTRA0009 = 392,
  TEMPLATE_XTRA0010 = 393,
  TEMPLATE_XTRA0011 = 394,
  TEMPLATE_XTRA0012 = 395,
  TEMPLATE_XTRA0013 = 396,
  TEMPLATE_XTRA0014 = 397,
  TEMPLATE_XTRA0015 = 398,
  TEMPLATE_XTRA0016 = 399,

  TEMPLATE_HILL01 = 400,

  TEMPLATE_NONE = 65535
};
using enum TemplateType;

// Templates outgrow magic_enum's default window; TEMPLATE_NONE (65535)
// stays outside it on purpose, it is not an index.
template <>
struct magic_enum::customize::enum_range<TemplateType> {
  static constexpr int min = 0;
  static constexpr int max = 512;
};

/**********************************************************************
**	The three dimensional terrain objects are enumerated here. These
**	objects function similar to buildings in that they can be driven
**	behind and can take damage on an individual basis.
*/
enum class TerrainType {
  TERRAIN_NONE = -1,
  TERRAIN_TREE1,
  TERRAIN_TREE2,
  TERRAIN_TREE3,
  TERRAIN_TREE5,
  TERRAIN_TREE6,
  TERRAIN_TREE7,
  TERRAIN_TREE8,
  TERRAIN_TREE10,
  TERRAIN_TREE11,
  TERRAIN_TREE12,
  TERRAIN_TREE13,
  TERRAIN_TREE14,
  TERRAIN_TREE15,
  TERRAIN_TREE16,
  TERRAIN_TREE17,
  TERRAIN_CLUMP1,
  TERRAIN_CLUMP2,
  TERRAIN_CLUMP3,
  TERRAIN_CLUMP4,
  TERRAIN_CLUMP5,

  TERRAIN_ICE01,
  TERRAIN_ICE02,
  TERRAIN_ICE03,
  TERRAIN_ICE04,
  TERRAIN_ICE05,

  TERRAIN_BOXES01,
  TERRAIN_BOXES02,
  TERRAIN_BOXES03,
  TERRAIN_BOXES04,
  TERRAIN_BOXES05,
  TERRAIN_BOXES06,
  TERRAIN_BOXES07,
  TERRAIN_BOXES08,
  TERRAIN_BOXES09,

  TERRAIN_MINE
};
using enum TerrainType;

/**********************************************************************
**	Smudges are enumerated here. Smudges are transparent icons that are
**	drawn over the underlying terrain in order to give the effect of
**	alterations to the terrain. Craters are a good example of this.
*/
enum class SmudgeType {
  SMUDGE_NONE = -1,
  SMUDGE_CRATER1,
  SMUDGE_CRATER2,
  SMUDGE_CRATER3,
  SMUDGE_CRATER4,
  SMUDGE_CRATER5,
  SMUDGE_CRATER6,
  SMUDGE_SCORCH1,
  SMUDGE_SCORCH2,
  SMUDGE_SCORCH3,
  SMUDGE_SCORCH4,
  SMUDGE_SCORCH5,
  SMUDGE_SCORCH6,
  SMUDGE_BIB1,
  SMUDGE_BIB2,
  SMUDGE_BIB3
};
using enum SmudgeType;

/**********************************************************************
**	Animations are enumerated here. Animations are the high speed and
**	short lived effects that occur with explosions and fire.
*/
enum class AnimType {
  ANIM_NONE = -1,
  ANIM_FBALL1,         // Large fireball explosion (bulges rightward).
  ANIM_FBALL_FADE,     // Fading fireball puff.
  ANIM_FRAG1,          // Medium fragment throwing explosion -- short decay.
  ANIM_VEH_HIT1,       //	Small fireball explosion (bulges rightward).
  ANIM_VEH_HIT2,       //	Small fragment throwing explosion -- pop & sparkles.
  ANIM_VEH_HIT3,       // Small fragment throwing explosion -- burn/exp mix.
  ANIM_ART_EXP1,       // Large fragment throwing explosion -- many sparkles.
  ANIM_NAPALM1,        // Small napalm burn.
  ANIM_NAPALM2,        // Medium napalm burn.
  ANIM_NAPALM3,        // Large napalm burn.
  ANIM_SMOKE_PUFF,     // Small rocket smoke trail puff.
  ANIM_PIFF,           // Machine gun impact piffs.
  ANIM_PIFFPIFF,       // Chaingun impact piffs.
  ANIM_FIRE_SMALL,     // Small flame animation.
  ANIM_FIRE_MED,       // Medium flame animation.
  ANIM_FIRE_MED2,      // Medium flame animation (oranger).
  ANIM_FIRE_TINY,      // Very tiny flames.
  ANIM_MUZZLE_FLASH,   // Big cannon flash (with translucency).
  ANIM_SMOKE_M,        // Smoke rising from ground.
  ANIM_BURN_SMALL,     // Small combustible fire effect (with trail off).
  ANIM_BURN_MED,       // Medium combustible fire effect (with trail off).
  ANIM_BURN_BIG,       // Large combustible fire effect (with trail off).
  ANIM_ON_FIRE_SMALL,  // Burning effect for buildings.
  ANIM_ON_FIRE_MED,    // Burning effect for buildings.
  ANIM_ON_FIRE_BIG,    // Burning effect for buildings.
  ANIM_SAM_N,
  ANIM_SAM_NE,
  ANIM_SAM_E,
  ANIM_SAM_SE,
  ANIM_SAM_S,
  ANIM_SAM_SW,
  ANIM_SAM_W,
  ANIM_SAM_NW,
  ANIM_GUN_N,
  ANIM_GUN_NE,
  ANIM_GUN_E,
  ANIM_GUN_SE,
  ANIM_GUN_S,
  ANIM_GUN_SW,
  ANIM_GUN_W,
  ANIM_GUN_NW,
  ANIM_LZ_SMOKE,
  ANIM_CRATE_DEVIATOR,  // Red finned missile.
  ANIM_CRATE_DOLLAR,    // Dollar sign.
  ANIM_CRATE_EARTH,     // Cracked Earth.
  ANIM_CRATE_EMPULSE,   // Plasma ball.
  ANIM_CRATE_INVUN,     // Orange sphere with green rings.
  ANIM_CRATE_MINE,      // Spiked mine.
  ANIM_CRATE_RAPID,     // Red skull.
  ANIM_CRATE_STEALTH,   // Cloaking sphere.
  ANIM_CRATE_MISSILE,   // Green finned missile.
  ANIM_MOVE_FLASH,
  ANIM_OILFIELD_BURN,
  ANIM_ELECT_DIE,      // Electrocution infantryman death from Tesla coil
  ANIM_PARACHUTE,      // Parachute (designed to be attached to object).
  ANIM_DOG_ELECT_DIE,  // Electrocution dog death from Tesla coil
  ANIM_CORPSE1,
  ANIM_CORPSE2,
  ANIM_CORPSE3,
  ANIM_SPUTDOOR,
  ANIM_ATOM_BLAST,
  ANIM_CHRONO_BOX,
  ANIM_GPS_BOX,
  ANIM_INVUL_BOX,
  ANIM_PARA_BOX,
  ANIM_SONAR_BOX,
  ANIM_TWINKLE1,
  ANIM_TWINKLE2,
  ANIM_TWINKLE3,
  ANIM_FLAK,
  ANIM_WATER_EXP1,
  ANIM_WATER_EXP2,
  ANIM_WATER_EXP3,
  ANIM_CRATE_ARMOR,
  ANIM_CRATE_SPEED,
  ANIM_CRATE_FPOWER,
  ANIM_CRATE_TQUAKE,
  ANIM_PARA_BOMB,
  ANIM_MINE_EXP1,

  ANIM_ANT_DEATH
};
using enum AnimType;

/****************************************************************************
**	Infantry can be performing various activities. These can range from
*simple *	idle animations to physical hand to hand combat.
*/
enum class DoType {
  DO_NOTHING = -1,  // Not performing any choreographed sequence.
  DO_STAND_READY = 0,
  DO_STAND_GUARD = 1,
  DO_PRONE = 2,
  DO_WALK = 3,
  DO_FIRE_WEAPON = 4,
  DO_LIE_DOWN = 5,
  DO_CRAWL = 6,
  DO_GET_UP = 7,
  DO_FIRE_PRONE = 8,
  DO_IDLE1 = 9,
  DO_IDLE2 = 10,
  DO_GUN_DEATH = 11,
  DO_EXPLOSION_DEATH = 12,
  DO_EXPLOSION2_DEATH = 13,
  DO_GRENADE_DEATH = 14,
  DO_FIRE_DEATH = 15,
  DO_GESTURE1 = 16,
  DO_SALUTE1 = 17,
  DO_GESTURE2 = 18,
  DO_SALUTE2 = 19,
  DO_DOG_MAUL = 20
};
using enum DoType;

/*
**	This structure is associated with each maneuver type. It tells whether
*the *	maneuver can be interrupted and the frame rate.
*/
struct DoStruct {
  bool Interrupt : 1;        // Can it be interrupted?
  bool IsMobile : 1;         // Can it move while doing this?
  bool RandomStart : 1;      // Should animation be "randomized"?
  unsigned char Rate;        // Frame rate.
};

struct DoInfoStruct {
  int Frame;            // Starting frame of the animation.
  unsigned char Count;  // Number of frames of animation.
  unsigned char Jump;   // Frames to jump between facings.
};

/****************************************************************************
**	These are the various radio message that can be transmitted between
**	units and buildings. Some of these require a response from the receiver
**	and some don't.
*/
enum class RadioMessageType {
  RADIO_STATIC,      // "hisssss" -- non-message
  RADIO_ROGER,       // "Roger."
  RADIO_HELLO,       // "Come in. I wish to talk."
  RADIO_OVER_OUT,    // "Something came up, bye."
  RADIO_PICK_UP,     // "Please pick me up."
  RADIO_ATTACH,      // "Attach to transport."
  RADIO_DELIVERY,    // "I've got a delivery for you."
  RADIO_HOLD_STILL,  // "I'm performing load/unload maneuver. Be careful."
  RADIO_UNLOADED,    // "I'm clear."
  RADIO_UNLOAD,      // "You are clear to unload. Please start driving off now."
  RADIO_NEGATIVE,    // "Am unable to comply."
  RADIO_BUILDING,    // "I'm starting construction now... act busy."
  RADIO_COMPLETE,    // "I've finished construction. You are free."
  RADIO_REDRAW,      // "Oops, sorry. I might have bumped you a little."
  RADIO_DOCKING,     // "I'm trying to load up now."
  RADIO_CAN_LOAD,    // "May I become a passenger?"
  RADIO_ARE_REFINERY,    // "Are you a refinery ready to take shipment?"
  RADIO_TRYING_TO_LOAD,  // "Are you trying to become a passenger?"
  RADIO_MOVE_HERE,       // "Move to location X."
  RADIO_NEED_TO_MOVE,    // "Do you need to move somewhere?"
  RADIO_YEA_NOW_WHAT,    // "All right already. Now what?"
  RADIO_IM_IN,           // "I'm a passenger now."
  RADIO_BACKUP_NOW,      // "Begin backup into refinery now."
  RADIO_RUN_AWAY,        // "Run away! Run away!"
  RADIO_TETHER,          // "Establish tether contact."
  RADIO_UNTETHER,        // "Break tether contact."
  RADIO_REPAIR,          // "Repair one step."
  RADIO_PREPARED,        // "Are you prepared to fight?"
  RADIO_ATTACK_THIS,     // "Attack this target please."
  RADIO_RELOAD,          // "Reload one step please."
  RADIO_CANT,            // "Circumstances prevent success."
  RADIO_ALL_DONE,        // "I have completed the task."
  RADIO_NEED_REPAIR,     // "Are you in need of service depot work?"
  RADIO_ON_DEPOT         // "Are you sitting on a service depot?"
};
using enum RadioMessageType;

/****************************************************************************
**	Various trigger events and actions require additional data. This
*enumeration is *	used to indicate what kind of additional data is
*required. This is also used *	for team mission types that might need
*additional data.
*/
enum class NeedType {
  NEED_NONE,       // No additional data is required.
  NEED_THEME,      // Need a musical theme.
  NEED_MOVIE,      // Need a movie to play.
  NEED_SOUND,      // Sound effect.
  NEED_SPEECH,     // Speech from EVA.
  NEED_INFANTRY,   // Infantry type class.
  NEED_UNIT,       // Unit type class.
  NEED_AIRCRAFT,   // Aircraft type class.
  NEED_STRUCTURE,  // Structure type class.
  NEED_WAYPOINT,   // Waypoint letter.
  NEED_NUMBER,     // General number.
  NEED_TRIGGER,    //	Trigger object reference.
  NEED_TEAM,       // Team type class.
  NEED_HOUSE,      // House type number.
  NEED_TIME,       // Time delay value required.
  NEED_QUARRY,     // Quarry type is needed.
  NEED_FORMATION,  // A formation type is needed.
  NEED_BOOL,       // Boolean value is needed.
  NEED_SPECIAL,    // Special weapon ability.
  NEED_MISSION,    // General unit mission type.
  NEED_HEX_NUMBER  // General number.
};
using enum NeedType;

/****************************************************************************
**	There are various target types that teams and special weapons can be
**	assigned to attack. These are general target categories since the actual
**	disposition of potential targets cannot be precisely predicted -- thus
*these *	serve as guidelines for the computer AI.
*/
enum class QuarryType {
  QUARRY_NONE,

  QUARRY_ANYTHING,    // Attack any enemy (same as "hunt").
  QUARRY_BUILDINGS,   // Attack buildings (in general).
  QUARRY_HARVESTERS,  // Attack harvesters or refineries.
  QUARRY_INFANTRY,    // Attack infantry.
  QUARRY_VEHICLES,    // Attack combat vehicles.
  QUARRY_VESSELS,     // Attach ships.
  QUARRY_FACTORIES,   // Attack factories (all types).
  QUARRY_DEFENSE,     // Attack base defense buildings.
  QUARRY_THREAT,      // Attack enemies near friendly base.
  QUARRY_POWER,       // Attack power facilities.
  QUARRY_FAKES        // Prefer to attack fake buildings.
};
using enum QuarryType;

/****************************************************************************
**	Teams can be assigned formations. This specifies the various formations
*that *	a team can be composed into.
*/
enum class FormationType {
  FORMATION_NONE,

  FORMATION_TIGHT,    // Tight grouping (vulnerable units in center).
  FORMATION_LOOSE,    // Loose grouping (one cell separation between units).
  FORMATION_WEDGE_N,  // Wedge shape.
  FORMATION_WEDGE_E,  // Wedge shape.
  FORMATION_WEDGE_S,  // Wedge shape.
  FORMATION_WEDGE_W,  // Wedge shape.
  FORMATION_LINE_NS,  // Column formation.
  FORMATION_LINE_EW   // Line formation.
};
using enum FormationType;

/****************************************************************************
**	Selected units have a special selected unit box around them. These are
*the *	defines for the two types of selected unit boxes. One is for infantry
*and *	the other is for regular units.
*/
// Selection box shapes in SelectShapes.
inline constexpr int kSelectNone = -1;
inline constexpr int kSelectInfantry = 0;  // Small infantry selection box.
inline constexpr int kSelectUnit = 1;      // Big unit selection box.
inline constexpr int kSelectBuilding =
    kSelectUnit;  // Custom box for buildings.
inline constexpr int kSelectTerrain =
    kSelectUnit;  // Custom box for terrain objects.
inline constexpr int kSelectWrench =
    2;  // A building is repairing overlay graphic.

/****************************************************************************
**	The pip shapes and text shapes are enumerated according to the following
**	type. These special shapes are drawn over special objects or in other
*places *	where shape technology is needed.
*/
enum class PipEnum {
  PIP_EMPTY,     // Empty pip spot.
  PIP_FULL,      // Full pip spot.
  PIP_PRIMARY,   // "Primary" building marker.
  PIP_READY,     // "Ready" construction information tag.
  PIP_HOLDING,   // "Hold"ing construction information tag.
  PIP_ENGINEER,  // Full pip with engineer coloring.
  PIP_CIVILIAN,  // Full pip with civilian coloring.
  PIP_COMMANDO,  // Full pip with commando coloring.
  PIP_NUMBERS,   // digit 0
  PIP_NUMBER1,   // digit 1
  PIP_NUMBER2,   // digit 2
  PIP_NUMBER3,   // digit 3
  PIP_NUMBER4,   // digit 4
  PIP_NUMBER5,   // digit 5
  PIP_NUMBER6,   // digit 6
  PIP_NUMBER7,   // digit 7
  PIP_NUMBER8,   // digit 8
  PIP_NUMBER9,   // digit 9
  PIP_DECOY,     // word "Decoy" for fake buildings
  PIP_LETTERF,   // letter 'F' for signifying in-formation
  PIP_MEDIC,     // Little medic red cross.
  PIP_PRI        // Abbreviated "Primary" for kennel
};
using enum PipEnum;

/****************************************************************************
**	The mouse cursor can be in different states. These states are listed
**	below. Some of these represent animating mouse cursors. The mouse
**	is controlled by passing one of these values to the appropriate
**	MouseClass member function.
*/
enum class MouseType {
  MOUSE_NORMAL,
  MOUSE_N,
  MOUSE_NE,
  MOUSE_E,
  MOUSE_SE,
  MOUSE_S,
  MOUSE_SW,
  MOUSE_W,
  MOUSE_NW,
  MOUSE_NO_N,
  MOUSE_NO_NE,
  MOUSE_NO_E,
  MOUSE_NO_SE,
  MOUSE_NO_S,
  MOUSE_NO_SW,
  MOUSE_NO_W,
  MOUSE_NO_NW,
  MOUSE_NO_MOVE,
  MOUSE_CAN_MOVE,
  MOUSE_ENTER,
  MOUSE_DEPLOY,
  MOUSE_CAN_SELECT,
  MOUSE_CAN_ATTACK,
  MOUSE_SELL_BACK,
  MOUSE_SELL_UNIT,
  MOUSE_REPAIR,
  MOUSE_NO_REPAIR,
  MOUSE_NO_SELL_BACK,
  MOUSE_RADAR_CURSOR,
  MOUSE_NUCLEAR_BOMB,
  MOUSE_AIR_STRIKE,
  MOUSE_DEMOLITIONS,
  MOUSE_AREA_GUARD,
  MOUSE_HEAL,
  MOUSE_DAMAGE,   // Engineer entering building to damage it.
  MOUSE_GREPAIR,  // Engineer entering friendly building to heal it.
  MOUSE_STAY_ATTACK,
  MOUSE_NO_DEPLOY,
  MOUSE_NO_ENTER,
  MOUSE_NO_GREPAIR,
  MOUSE_CHRONO_SELECT,
  MOUSE_CHRONO_DEST
};
using enum MouseType;

/**********************************************************************
**	This structure is used to control the box relief style drawn by
**	the Draw_Box() function.
*/
struct BoxStyleType {
  uint8_t Filler;     // Center box fill color.
  uint8_t Shadow;     // Shadow color (darker).
  uint8_t Highlight;  // Highlight color (lighter).
  uint8_t Corner;     // Corner color (transition).
};

enum class BoxStyleEnum {
  BOXSTYLE_DOWN,        // Typical depressed edge border.
  BOXSTYLE_RAISED,      // Typical raised edge border.
  BOXSTYLE_DIS_DOWN,    // Disabled but depressed.
  BOXSTYLE_DIS_RAISED,  // Disabled but raised.
  BOXSTYLE_BOX,         // list box.
  BOXSTYLE_BORDER,      // main dialog box.
};
using enum BoxStyleEnum;

/**********************************************************************
**	Damage, as inflicted by projectiles, has different characteristics.
**	These are the different "warhead" types that can be assigned to the
**	various projectiles in the game.
*/
enum class WarheadType {
  WARHEAD_NONE = -1,

  WARHEAD_SA,            // Small arms -- good against infantry.
  WARHEAD_HE,            //	High explosive -- good against buildings & infantry.
  WARHEAD_AP,            // Armor piercing -- good against armor.
  WARHEAD_FIRE,          // Incendiary -- Good against flammables.
  WARHEAD_HOLLOW_POINT,  // Sniper bullet type.
  WARHEAD_TESLA,         // Electrocution warhead for infantrymen
  WARHEAD_DOG,           // Slavering attack beast mauling infantryman
  WARHEAD_NUKE,          // Nuclear missile
  WARHEAD_MECHANICAL     // repair weapon for vehicles
};
using enum WarheadType;

/**********************************************************************
**	This enumerates the various weapon types. The weapon is characterized
**	by the projectile it launches, the damage it does, and the rate of
**	fire.
*/
enum class WeaponType {
  WEAPON_NONE = -1,

  WEAPON_COLT45,
  WEAPON_ACK_ACK,
  WEAPON_VULCAN,
  WEAPON_MAVERICK,
  WEAPON_CAMERA,
  WEAPON_FIREBALL,
  WEAPON_RIFLE,
  WEAPON_CHAIN_GUN,
  WEAPON_PISTOL,
  WEAPON_M16,
  WEAPON_DRAGON,
  WEAPON_HELLFIRE,
  WEAPON_GRENADE,
  WEAPON_75MM,
  WEAPON_90MM,
  WEAPON_105MM,
  WEAPON_120MM,
  WEAPON_TURRET_GUN,
  WEAPON_MAMMOTH_TUSK,
  WEAPON_155MM,
  WEAPON_M60MG,
  WEAPON_NAPALM,
  WEAPON_TESLA_ZAP,
  WEAPON_NIKE,
  WEAPON_8INCH,
  WEAPON_STINGER,
  WEAPON_TORPEDO,
  WEAPON_2INCH,
  WEAPON_DEPTH_CHARGE,
  WEAPON_PARA_BOMB,
  WEAPON_DOGJAW,
  WEAPON_HEAL,
  WEAPON_SCUD,
  WEAPON_FLAMER,
  WEAPON_REDEYE,

  WEAPON_MANDIBLE,

  WEAPON_PORTATESLA,
  WEAPON_GOODWRENCH,
  WEAPON_SUBSCUD,
  WEAPON_TTANKZAP,
  WEAPON_APTUSK,
  WEAPON_DEMOCHARGE,
  WEAPON_CARRIER
};
using enum WeaponType;

/**********************************************************************
**	The various armor types are best suited to defend against a limited
**	kind of warheads. The game strategy revolves around proper
**	combination of armor and weaponry. Each vehicle or building has armor
**	rated according to one of the following types.
*/
enum class ArmorType {
  ARMOR_NONE,      // Vulnerable to SA and HE.
  ARMOR_WOOD,      // Vulnerable to HE and Fire.
  ARMOR_ALUMINUM,  // Vulnerable to AP and SA.
  ARMOR_STEEL,     // Vulnerable to AP.
  ARMOR_CONCRETE   // Vulnerable to HE and AP.
};
using enum ArmorType;

// Working MCGA colors that give a pleasing effect for beveled edges and
// other purposes.
inline constexpr int kMagicColorCount = 13;      // Translucent colors.
inline constexpr int kShadowColorCount = 4;      // Terrain shroud translucency.
inline constexpr int kUnitShadowColorCount = 1;  // Unit shadow ghost colors.

// Palette range that cycles for water effects.
inline constexpr int kCycleColorStart = 6 * 16;
inline constexpr int kCycleColorCount = 7;

// Palette entries animated by the palette cycler: the pulsing selection colour
// and the ember colour right after the water cycle range.
inline constexpr int kPulseColor = 255;
inline constexpr int kEmberColor = kCycleColorStart + kCycleColorCount;

/**********************************************************************
**	These are the control flags for Fancy_Text_Print function.
*/
enum class CNC_FLAG_ENUM TextPrintType {
  TPF_LASTPOINT = 0x0000,     // Use previous font point value.
  TPF_6POINT = 0x0001,        // Use 6 point font.
  TPF_8POINT = 0x0002,        // Use 8 point font.
  TPF_3POINT = 0x0003,        // Use 3 point font.
  TPF_LED = 0x0004,           // Use LED font.
  TPF_VCR = 0x0005,           // Use VCR font.
  TPF_6PT_GRAD = 0x0006,      // Use 6 point gradient font.
  TPF_MAP = 0x0007,           // Font used for popup help text.
  TPF_METAL12 = 0x0008,       // Use 12 point tab font
  TPF_EFNT = 0x0009,          // Use scenario editor font.
  TPF_TYPE = 0x000A,          // Use teletype font
  TPF_SCORE = 0x000B,         // Use score screen font.
  TPF_LASTSHADOW = 0x0000,    // Use previous font palette.
  TPF_NOSHADOW = 0x0010,      // Don't have any shadow.
  TPF_DROPSHADOW = 0x0020,    //	Use a simple drop shadow.
  TPF_FULLSHADOW = 0x0040,    // Use a full outline shadow.
  TPF_LIGHTSHADOW = 0x0080,   // Use engraved drop 'shadow' color.
  TPF_CENTER = 0x0100,        // Center about the X axis.
  TPF_RIGHT = 0x0200,         // Right justify text.
  TPF_MEDIUM_COLOR = 0x1000,  // Use medium color for all text gradient
  TPF_BRIGHT_COLOR = 0x2000,  // Use bright color for all text gradient
  TPF_USE_GRAD_PAL = 0x4000   // Use a gradient palette based on fore color
};
using enum TextPrintType;

// Standard button text print flags.
inline constexpr TextPrintType kTpfButton =
    TPF_CENTER | TPF_6PT_GRAD | TPF_NOSHADOW;
inline constexpr auto kTpfEButton = TPF_CENTER | TPF_EFNT | TPF_NOSHADOW;
inline constexpr auto kTpfText = TPF_6PT_GRAD | TPF_NOSHADOW;

/**********************************************************************
**	These control the maximum number of objects in the game. Make sure that
*these *	maximums never exceed the maximum value for the "ID" element in
*the *	object class.
*/
inline constexpr int kBuildingMax = 500;  // Lasts for hours.
inline constexpr int kHouseMax =
    static_cast<int>(magic_enum::enum_count<HousesType>()) +
    1;                                    // Lasts entire scenario.
inline constexpr int kInfantryMax = 500;  // Lasts for minutes.
inline constexpr int kUnitMax = 500;      // Lasts for minutes.
inline constexpr int kVesselMax = 100;    // Lasts for minutes.
inline constexpr int kTeamTypeMax = 60;   // Lasts forever.

// Save filename description: 40 chars + CR + LF + CTRL-Z + NULL.
inline constexpr int kDescripMax = 44;

// Number of cells to look ahead for movement.
inline constexpr int kConquerPathMax = 12;

// Default maximum any one player can have or build.
inline constexpr int kEachInfantryMax = kInfantryMax / 5;
inline constexpr int kEachUnitMax = kUnitMax / 5;
inline constexpr int kEachBuildingMax = kBuildingMax / 5;
inline constexpr int kEachVesselMax = kVesselMax / 5;

/**********************************************************************
**	Terrain can be of these different classes. At any point in the game
**	a particular piece of ground must fall under one of these
*classifications. *	This is true, even if it is undergoing a temporary
*transition.
*/
enum class LandType {
  LAND_CLEAR = 0,     // "Clear" terrain.
  LAND_ROAD = 1,      // Road terrain.
  LAND_WATER = 2,     // Water.
  LAND_ROCK = 3,      // Impassable rock.
  LAND_WALL = 4,      // Wall (blocks movement).
  LAND_TIBERIUM = 5,  // Tiberium field.
  LAND_BEACH = 6,     //	Beach terrain.
  LAND_ROUGH = 7,     // Rocky terrain.
  LAND_RIVER = 8,     // Rocky riverbed.

  LAND_NONE = -1
};
using enum LandType;

/**********************************************************************
**	The theaters of operation are as follows.
*/
enum class TheaterType {
  THEATER_NONE = -1,
  THEATER_TEMPERATE = 0,
  THEATER_SNOW = 1,
  THEATER_INTERIOR = 2
};
using enum TheaterType;

// Theater bit masks over TheaterType, for the object type tables.
inline constexpr uint32_t kTheaterFlagTemperate =
    base::Bit<uint32_t>(THEATER_TEMPERATE);
inline constexpr uint32_t kTheaterFlagSnow = base::Bit<uint32_t>(THEATER_SNOW);
inline constexpr uint32_t kTheaterFlagInterior =
    base::Bit<uint32_t>(THEATER_INTERIOR);

struct TheaterDataType {
  char Name[16];
  char Root[10];
  char Suffix[4];
};

/**********************************************************************
**	Each building has a predetermined size. These are the size numbers.
**	The trailing number is this define is the width and height
*(respectively) *	of the building in cells.
*/
enum class BSizeType {
  BSIZE_NONE = -1,
  BSIZE_11 = 0,
  BSIZE_21 = 1,
  BSIZE_12 = 2,
  BSIZE_22 = 3,
  BSIZE_23 = 4,
  BSIZE_32 = 5,
  BSIZE_33 = 6,
  BSIZE_42 = 7,
  BSIZE_55 = 8
};
using enum BSizeType;

/**********************************************************************
** When objects are manipulated on the map that are marked as being
**	removed (up), placed down (down), or just to be redrawn (change);
** or when an object's rendering (not logical) size changes, due to
** its being selected or having an animation attached (overlap up/down).
*/
enum class MarkType {
  MARK_UP,             //	Removed from the map.
  MARK_DOWN,           //	Placed on the map.
  MARK_CHANGE,         //	Altered in place on the map.
  MARK_CHANGE_REDRAW,  //	Redraw because of animation change.
  MARK_OVERLAP_DOWN,   // Mark overlap cells on the map
  MARK_OVERLAP_UP      // Clear overlap cells on the map
};
using enum MarkType;

/****************************************************************************
**	Window number definition list. Each window should be referred to by
**	the value given in this list.
*/
// Allow window number enums to be passed to library functions.
enum class WindowNumberType {
  WINDOW_MAIN,      // Full screen window.
  WINDOW_ERROR,     // Library error window.
  WINDOW_TACTICAL,  // Tactical map window.
  WINDOW_MENU,      // Main selection menu.
  WINDOW_SIDEBAR,   // Sidebar (buildable list) window.
  WINDOW_EDITOR,    // Scenario editor window.
  WINDOW_PARTIAL,   // Partial object draw sub-window.
};
using enum WindowNumberType;

/****************************************************************************
**	For every cell there are 8 adjacent cells. Use these direction numbers
**	when referring to adjacent cells. This comes into play when moving
**	between cells and in the Desired_Facing() algorithm.
*/
enum class FacingType : int8_t {
  FACING_NONE = -1,
  FACING_N,   // North
  FACING_NE,  // North-East
  FACING_E,   // East
  FACING_SE,  // South-East
  FACING_S,   // South
  FACING_SW,  // South-West
  FACING_W,   // West
  FACING_NW   // North-West
};
using enum FacingType;

// Wraps a facing sum or difference onto the eight compass facings. Negative
// values wrap the same way the old low-three-bits mask did (-1 is FACING_NW).
constexpr FacingType WrapFacing(const int facing) {
  return static_cast<FacingType>(((facing % 8) + 8) % 8);
}

inline FacingType operator+(const FacingType f1, const FacingType f2) {
  return WrapFacing(static_cast<int>(f1) + static_cast<int>(f2));
}
inline FacingType operator+(const FacingType f1, const int f2) {
  return WrapFacing(static_cast<int>(f1) + f2);
}

inline FacingType operator-(const FacingType f1, const FacingType f2) {
  return WrapFacing(static_cast<int>(f1) - static_cast<int>(f2));
}
inline FacingType operator-(const FacingType f1, const int f2) {
  return WrapFacing(static_cast<int>(f1) - f2);
}

inline FacingType operator+=(FacingType& f1, const FacingType f2) {
  f1 = WrapFacing(static_cast<int>(f1) + static_cast<int>(f2));
  return f1;
}
inline FacingType operator+=(FacingType& f1, const int f2) {
  f1 = WrapFacing(static_cast<int>(f1) + f2);
  return f1;
}

inline int operator*(const FacingType f1, const FacingType f2) {
  return static_cast<int>(f1) * static_cast<int>(f2);
}

/****************************************************************************
**	Timer constants. These are used when setting the countdown timer.
**	Note that this is based upon a timer that ticks every 60th of a second.
*/
// Rate of the system timer, in ticks per second.
inline constexpr int kTimerSecond = 60;

// Fade durations, in system timer ticks.
inline constexpr int kFadePaletteFast = kTimerSecond / 8;
inline constexpr int kFadePaletteMedium = kTimerSecond / 4;
inline constexpr int kFadePaletteSlow = kTimerSecond / 2;

// Rate of the game logic clock, in ticks per second.
inline constexpr int kTicksPerSecond = 15;

// The derived durations are int64_t because that is what the timer classes
// store; computing them in int and widening afterwards is what
// bugprone-implicit-widening-of-multiplication-result flags. The two base rates
// stay int: they are also divided into int quantities (the fade durations
// above, frame delays) and would otherwise push narrowing into those.
inline constexpr int64_t kTimerMinute = int64_t{kTimerSecond} * 60;
inline constexpr int64_t kTicksPerMinute = int64_t{kTicksPerSecond} * 60;
inline constexpr int64_t kTicksPerHour = kTicksPerMinute * 60;
inline constexpr int64_t kGrayFadeTime = kTicksPerSecond;

/****************************************************************************
** Each vehicle is give a speed rating. This is a combination of not only
**	its physical speed, but the means by which it travels (wheels, tracks,
**	wings, etc). This is used to determine the movement table.
*/
enum class SpeedType {
  SPEED_NONE = -1,

  SPEED_FOOT,    // Bipedal.
  SPEED_TRACK,   // Tracked locomotion.
  SPEED_WHEEL,   // Balloon tires.
  SPEED_WINGED,  // Lifter's, 'thopters, and rockets.
  SPEED_FLOAT    // Ships.
};
using enum SpeedType;

/**********************************************************************
**	These are the sound effect digitized sample file names.
*/
enum class VocType {
  VOC_NONE = -1,

  VOC_GIRL_OKAY,  // "okay"
  VOC_GIRL_YEAH,  // "yeah?"
  VOC_GUY_OKAY,   //	"okay"
  VOC_GUY_YEAH,   // "yeah?"

  VOC_MINELAY1,    // mine layer sound
  VOC_ACKNOWL,     //	"acknowledged"
  VOC_AFFIRM,      //	"affirmative"
  VOC_AWAIT,       //	"awaiting orders"
  VOC_ENG_AFFIRM,  // Engineer: "affirmative"
  VOC_ENG_ENG,     // Engineer: "engineering"
  VOC_NO_PROB,     //	"not a problem"
  VOC_READY,       //	"ready and waiting"
  VOC_REPORT,      //	"reporting"
  VOC_RIGHT_AWAY,  //	"right away sir"
  VOC_ROGER,       //	"roger"
  VOC_UGOTIT,      //	"you got it"
  VOC_VEHIC,       //	"vehicle reporting"
  VOC_YESSIR,      //	"yes sir"

  VOC_SCREAM1,   //	short infantry scream
  VOC_SCREAM3,   //	short infantry scream
  VOC_SCREAM4,   //	short infantry scream
  VOC_SCREAM5,   //	short infantry scream
  VOC_SCREAM6,   //	short infantry scream
  VOC_SCREAM7,   //	short infantry scream
  VOC_SCREAM10,  //	short infantry scream
  VOC_SCREAM11,  //	short infantry scream
  VOC_YELL1,     //	long infantry scream

  VOC_CHRONO,   //	Chronosphere sound.
  VOC_CANNON1,  // Cannon sound (medium).
  VOC_CANNON2,  // Cannon sound (short).
  VOC_IRON1,
  VOC_ENG_MOVEOUT,  // Engineer: "movin' out"
  VOC_SONAR,        // sonar pulse
  VOC_SANDBAG,      // sand bag crunch
  VOC_MINEBLOW,
  VOC_CHUTE1,        // wind swoosh sound
  VOC_DOG_BARK,      // dog bark
  VOC_DOG_WHINE,     // dog whine
  VOC_DOG_GROWL2,    // strong dog growl
  VOC_FIRE_LAUNCH,   // fireball launch sound
  VOC_FIRE_EXPLODE,  // fireball explode sound
  VOC_GRENADE_TOSS,  // grenade toss
  VOC_GUN_5,         // 5 round gun burst (slow).
  VOC_GUN_7,         // 7 round gun burst (fast).
  VOC_ENG_YES,       // Engineer: "yes sir"
  VOC_GUN_RIFLE,     // Rifle shot.
  VOC_HEAL,          // Healing effect.
  VOC_DOOR,          // Hyrdrolic door.
  VOC_INVULNERABLE,  // Invulnerability effect.
  VOC_KABOOM1,       // Long explosion (muffled).
  VOC_KABOOM12,      // Very long explosion (muffled).
  VOC_KABOOM15,      // Very long explosion (muffled).
  VOC_SPLASH,        // Water splash
  VOC_KABOOM22,      // Long explosion (sharp).
  VOC_AACANON3,      // AA-Cannon
  VOC_TANYA_DIE,     // Tanya: scream
  VOC_GUN_5F,        // 5 round gun burst (fast).
  VOC_MISSILE_1,     // Missile with high tech effect.
  VOC_MISSILE_2,     // Long missile launch.
  VOC_MISSILE_3,     // Short missile launch.
  VOC_x6,
  VOC_GUN_5R,               // 5 round gun burst (rattles).
  VOC_BEEP,                 // Generic beep sound.
  VOC_CLICK,                //	Generic click sound.
  VOC_SILENCER,             // Silencer.
  VOC_CANNON6,              // Long muffled cannon shot.
  VOC_CANNON7,              // Sharp mechanical cannon fire.
  VOC_TORPEDO,              // Torpedo launch.
  VOC_CANNON8,              // Sharp cannon fire.
  VOC_TESLA_POWER_UP,       // Hum charge up.
  VOC_TESLA_ZAP,            // Tesla zap effect.
  VOC_SQUISH,               // Squish effect.
  VOC_SCOLD,                // Scold bleep.
  VOC_RADAR_ON,             // Powering up electronics.
  VOC_RADAR_OFF,            // B movie power down effect.
  VOC_PLACE_BUILDING_DOWN,  // Building slam down sound.
  VOC_KABOOM30,             // Short explosion (HE).
  VOC_KABOOM25,             // Short growling explosion.
  VOC_x7,
  VOC_DOG_HURT,          //	Dog whine.
  VOC_DOG_YES,           // Dog 'yes sir'.
  VOC_CRUMBLE,           // Building crumble.
  VOC_MONEY_UP,          // Rising money tick.
  VOC_MONEY_DOWN,        // Falling money tick.
  VOC_CONSTRUCTION,      // Building construction sound.
  VOC_GAME_CLOSED,       // Long bleep.
  VOC_INCOMING_MESSAGE,  // Soft happy warble.
  VOC_SYS_ERROR,         // Sharp soft warble.
  VOC_OPTIONS_CHANGED,   // Mid range soft warble.
  VOC_GAME_FORMING,      // Long warble.
  VOC_PLAYER_LEFT,       // Chirp sequence.
  VOC_PLAYER_JOINED,     // Reverse chirp sequence.
  VOC_DEPTH_CHARGE,      // Distant explosion sound.
  VOC_CASHTURN,          // Airbrake.

  VOC_TANYA_CHEW,   // Tanya: "Chew on this"
  VOC_TANYA_ROCK,   // Tanya: "Let's rock"
  VOC_TANYA_LAUGH,  // Tanya: "ha ha ha"
  VOC_TANYA_SHAKE,  // Tanya: "Shake it baby"
  VOC_TANYA_CHING,  // Tanya: "Cha Ching"
  VOC_TANYA_GOT,    // Tanya: "That's all you got"
  VOC_TANYA_KISS,   // Tanya: "Kiss it bye bye"
  VOC_TANYA_THERE,  // Tanya: "I'm there"
  VOC_TANYA_GIVE,   // Tanya: "Give it to me"
  VOC_TANYA_YEA,    // Tanya: "Yea?"
  VOC_TANYA_YES,    // Tanya: "Yes sir?"
  VOC_TANYA_WHATS,  // Tanya: "What's up."
  VOC_WALLKILL2,    // Crushing wall sound.
  VOC_x8,
  VOC_TRIPLE_SHOT,  // Three quick shots in succession.
  VOC_SUBSHOW,      // Submarine surfacing.
  VOC_E_AH,         // Einstein "ah"
  VOC_E_OK,         // Einstein "ok"
  VOC_E_YES,        // Einstein "yes"
  VOC_TRIP_MINE,    // mine explosion sound

  VOC_SPY_COMMANDER,  // Spy: "commander?"
  VOC_SPY_YESSIR,     // Spy: "yes sir"
  VOC_SPY_INDEED,     // Spy: "indeed"
  VOC_SPY_ONWAY,      // Spy: "on my way"
  VOC_SPY_KING,       // Spy: "for king and country"
  VOC_MED_REPORTING,  // Medic: "reporting"
  VOC_MED_YESSIR,     // Medic: "yes sir"
  VOC_MED_AFFIRM,     // Medic: "affirmative"
  VOC_MED_MOVEOUT,    // Medic: "movin' out"
  VOC_BEEP_SELECT,    // map selection beep

  VOC_THIEF_YEA,  // Thief: "yea?"

  VOC_ANTDIE,
  VOC_ANTBITE,

  VOC_THIEF_MOVEOUT,  // Thief: "movin' out"
  VOC_THIEF_OKAY,     // Thief: "ok"
  VOC_x11,
  VOC_THIEF_WHAT,    // Thief: "what"
  VOC_THIEF_AFFIRM,  // Thief: "affirmative"

  VOC_STAVCMDR,
  VOC_STAVCRSE,
  VOC_STAVYES,
  VOC_STAVMOV,
  VOC_BUZZY1,
  VOC_RAMBO1,
  VOC_RAMBO2,
  VOC_RAMBO3,

  VOC_MECHYES1,
  VOC_MECHHOWDY1,
  VOC_MECHRISE1,
  VOC_MECHHUH1,
  VOC_MECHHEAR1,
  VOC_MECHLAFF1,
  VOC_MECHBOSS1,
  VOC_MECHYEEHAW1,
  VOC_MECHHOTDIG1,
  VOC_MECHWRENCH1,
  VOC_STBURN1,
  VOC_STCHRGE1,
  VOC_STCRISP1,
  VOC_STDANCE1,
  VOC_STJUICE1,
  VOC_STJUMP1,
  VOC_STLIGHT1,
  VOC_STPOWER1,
  VOC_STSHOCK1,
  VOC_STYES1,

  VOC_CHRONOTANK1,
  VOC_MECH_FIXIT1,
  VOC_MAD_CHARGE,
  VOC_MAD_EXPLODE,
  VOC_SHOCK_TROOP1
};
using enum VocType;

// Sound effects outgrow magic_enum's default window of 127.
template <>
struct magic_enum::customize::enum_range<VocType> {
  static constexpr int min = 0;
  static constexpr int max = 256;
};

/*
**	EVA voices are specified by these identifiers.
*/
enum class VoxType : int8_t {
  VOX_NONE = -1,
  VOX_ACCOMPLISHED,         //	mission accomplished
  VOX_FAIL,                 //	your mission has failed
  VOX_NO_FACTORY,           //	unable to comply, building in progress
  VOX_CONSTRUCTION,         //	construction complete
  VOX_UNIT_READY,           // unit ready
  VOX_NEW_CONSTRUCT,        //	new construction options
  VOX_DEPLOY,               //	cannot deploy here
  VOX_STRUCTURE_DESTROYED,  // structure destroyed
  VOX_INSUFFICIENT_POWER,   // insufficient power
  VOX_NO_CASH,              //	insufficient funds
  VOX_CONTROL_EXIT,         //	battle control terminated
  VOX_REINFORCEMENTS,       //	reinforcements have arrived
  VOX_CANCELED,             //	canceled
  VOX_BUILDING,             //	building
  VOX_LOW_POWER,            //	low power
  VOX_NEED_MO_MONEY,        //	need more funds
  VOX_BASE_UNDER_ATTACK,    //	our base is under attack
  VOX_UNABLE_TO_BUILD,      //	unable to build more
  VOX_PRIMARY_SELECTED,     //	primary building selected
  VOX_MADTANK_DEPLOYED,     // M.A.D. Tank Deployed (English speech set only)
  VOX_none4,
  VOX_UNIT_LOST,         //	unit lost
  VOX_SELECT_TARGET,     // select target
  VOX_PREPARE,           //	enemy approaching
  VOX_NEED_MO_CAPACITY,  //	silos needed
  VOX_SUSPENDED,         //	on hold
  VOX_REPAIRING,         //	repairing
  VOX_none5,
  VOX_none6,
  VOX_AIRCRAFT_LOST,
  VOX_none7,
  VOX_ALLIED_FORCES_APPROACHING,
  VOX_ALLIED_APPROACHING,
  VOX_none8,
  VOX_none9,
  VOX_BUILDING_INFILTRATED,
  VOX_CHRONO_CHARGING,
  VOX_CHRONO_READY,
  VOX_CHRONO_TEST,
  VOX_HQ_UNDER_ATTACK,
  VOX_CENTER_DEACTIVATED,
  VOX_CONVOY_APPROACHING,
  VOX_CONVOY_UNIT_LOST,
  VOX_EXPLOSIVE_PLACED,
  VOX_MONEY_STOLEN,
  VOX_SHIP_LOST,
  VOX_SATALITE_LAUNCHED,
  VOX_SONAR_AVAILABLE,
  VOX_none10,
  VOX_SOVIET_FORCES_APPROACHING,
  VOX_SOVIET_REINFORCEMENTS,
  VOX_TRAINING,
  VOX_ABOMB_READY,
  VOX_ABOMB_LAUNCH,
  VOX_ALLIES_N,
  VOX_ALLIES_S,
  VOX_ALLIES_E,
  VOX_ALLIES_W,
  VOX_OBJECTIVE1,
  VOX_OBJECTIVE2,
  VOX_OBJECTIVE3,
  VOX_IRON_CHARGING,
  VOX_IRON_READY,
  VOX_RESCUED,
  VOX_OBJECTIVE_NOT,
  VOX_SIGNAL_N,
  VOX_SIGNAL_S,
  VOX_SIGNAL_E,
  VOX_SIGNAL_W,
  VOX_SPY_PLANE,
  VOX_FREED,
  VOX_UPGRADE_ARMOR,
  VOX_UPGRADE_FIREPOWER,
  VOX_UPGRADE_SPEED,
  VOX_MISSION_TIMER,
  VOX_UNIT_FULL,
  VOX_UNIT_REPAIRED,
  VOX_TIME_40,
  VOX_TIME_30,
  VOX_TIME_20,
  VOX_TIME_10,
  VOX_TIME_5,
  VOX_TIME_4,
  VOX_TIME_3,
  VOX_TIME_2,
  VOX_TIME_1,
  VOX_TIME_STOP,
  VOX_UNIT_SOLD,
  VOX_TIMER_STARTED,
  VOX_TARGET_RESCUED,
  VOX_TARGET_FREED,
  VOX_TANYA_RESCUED,
  VOX_STRUCTURE_SOLD,
  VOX_SOVIET_FORCES_FALLEN,
  VOX_SOVIET_SELECTED,
  VOX_SOVIET_EMPIRE_FALLEN,
  VOX_OPERATION_TERMINATED,
  VOX_OBJECTIVE_REACHED,
  VOX_OBJECTIVE_NOT_REACHED,
  VOX_OBJECTIVE_MET,
  VOX_MERCENARY_RESCUED,
  VOX_MERCENARY_FREED,
  VOX_KOSOYGEN_FREED,
  VOX_FLARE_DETECTED,
  VOX_COMMANDO_RESCUED,
  VOX_COMMANDO_FREED,
  VOX_BUILDING_IN_PROGRESS,
  VOX_ATOM_PREPPING,
  VOX_ALLIED_SELECTED,
  VOX_ABOMB_PREPPING,
  VOX_ATOM_LAUNCHED,
  VOX_ALLIED_FORCES_FALLEN,
  VOX_ABOMB_AVAILABLE,
  VOX_ALLIED_REINFORCEMENTS,
  VOX_SAVE1,
  VOX_LOAD1
};
using enum VoxType;

/****************************************************************************
**	Game reinforcements are each controlled by the following structure. The
**	data originates in the scenario INI file but is then carried throughout
**	any saved games.
*/
enum class SourceType {
  SOURCE_NONE = -1,  // No defined source (error condition).
  SOURCE_NORTH,      // From north edge.
  SOURCE_EAST,       // From east edge.
  SOURCE_SOUTH,      // From south edge.
  SOURCE_WEST,       // From west edge.
  SOURCE_AIR         // Dropped by air (someplace).
};
using enum SourceType;

/****************************************************************************
**	This entry defines a complete color scheme, with the player's remap
*table,
** the font remap table, and a color scheme for dialog boxes and buttons.
*/
struct RemapControlType {
  unsigned char BrightColor;      // Highlight (bright) color index.
  unsigned char Color;            // Normal color index.
  unsigned char RemapTable[256];  // Actual remap table.
  unsigned char FontRemap[16];    // Remap table for gradient font.
  unsigned char Shadow;           // Color of shadowed edge of a raised button.
  unsigned char Background;       // Background fill color for buttons.
  unsigned char Corners;    // Transition color between shadow and highlight.
  unsigned char Highlight;  // Bright edge of raised button.
  unsigned char Box;        // Color for dialog box border.
  unsigned char Bright;     // Color used for highlighted text.
  unsigned char Underline;  // Color for underlining dialog box titles.
  unsigned char Bar;        // Selected entry list box background color.
};

/****************************************************************************
**	Each type of terrain has certain characteristics. These are indicated
**	by the structure below. For every element of terrain there is a
**	corresponding GroundType structure.
*/
struct GroundType {
  base::EnumArray<SpeedType, fixed> Cost;  // Terrain effect cost
                                           // (normal).
  bool Build = false;  // Can build on this terrain?
};

/**************************************************************************
**	Find_Path returns with a pointer to this structure.
*/
struct PathType {
  CELL Start = 0;                 // Starting cell number.
  int Cost = 0;                   // Accumulated terrain cost.
  int Length = 0;                 // Command string length.
  std::span<FacingType> Command;  // Pointer to command string.
  std::span<uint32_t> Overlap;    // Overlap bitmap, see ra/path_overlap.h.
  CELL LastOverlap = 0;           // stores position of last overlap
  CELL LastFixup = 0;             // stores position of last overlap
};

/****************************************************************************
**	This is the max number of events supported on one frame.
*/
inline constexpr int kMaxEvents = 64;

// Frame flags in the high byte of a key-frame shape's offset table.
inline constexpr uint8_t kKfNumber = 0x08;
inline constexpr uint8_t kKfLcw = 0x10;
inline constexpr uint8_t kKfDelta = 0x20;
inline constexpr uint8_t kKfKeyDelta = 0x40;
inline constexpr uint8_t kKfKeyFrame = 0x80;
inline constexpr uint8_t kKfMask = 0xF0;

/*
** New Config structure for .CFG files
*/
struct NewConfigType {
  unsigned DigitCard;      // SoundCardType.
  unsigned Port;           // SoundCardType.
  unsigned IRQ;            // SoundCardType.
  unsigned DMA;            // SoundCardType.
  unsigned BitsPerSample;  // bits per sound sample
  unsigned Channels;       // stereo/mono sound card
  unsigned Speed;          // stereo/mono sound card
  bool Reverse;            // Reverse left/right speakers
  char Language[4];
};

/****************************************************************************
**	These are the types of dialogs that can pop up outside of the main loop,
** an call the game in the background.
*/
enum class SpecialDialogType {
  SDLG_NONE,
  SDLG_OPTIONS,
  SDLG_SURRENDER,
  SDLG_SPECIAL
};
using enum SpecialDialogType;

// Palette entry reserved for the mouse cursor; brightness adjustment leaves
// it alone.
inline constexpr int kMouseColor = 16;

/****************************************************************************
**	These specify the shape numbers in the OPTIONS.SHP file. These shapes
**	are used to dress up the dialog boxes. Many of these shapes come in
*pairs. *	For dialog box shapes, they are left image / right image paired.
*For buttons, *	they are up / down paired.
*/

inline constexpr int kMaxLogLevel = 10;

// Maximum number of multiplayer players.
inline constexpr int kMaxPlayers = 8;

#endif  // CNC_RED_ALERT_RA_DEFINES_H_
