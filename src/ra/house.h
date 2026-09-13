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

/* $Header: /CounterStrike/HOUSE.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : HOUSE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 21, 1994 *
 *                                                                                             *
 *                  Last Update : May 21, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_HOUSE_H_
#define CNC_RED_ALERT_RA_HOUSE_H_

#include <cstddef>
#include <cstdint>

#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/defines.h"
#include "ra/heap.h"
#include "ra/jshell.h"
#include "ra/monoc.h"
#include "ra/object.h"
#include "ra/region.h"
#include "ra/super.h"
#include "ra/target.h"
#include "ra/type.h"
#include "ra/utracker.h"
#include "tech/fixed.h"
#include "tech/ftimer.h"

class FootClass;
class FactoryClass;

#define HOUSE_NAME_MAX 12

/****************************************************************************
**	Certain aspects of the house "country" are initially set by the scenario
**	control file. This information is static for the duration of the current
**	scenario, but is dynamic between scenarios. As such, it can't be placed
*in *	the static HouseTypeClass structure, but is embedded into the house
**	class instead.
*/
class HouseStaticClass {
 public:
  HouseStaticClass();

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(IQ, TechLevel, Allies, MaxUnit, MaxBuilding, MaxInfantry, MaxVessel,
       MaxAircraft, InitialCredits, Edge);
  }

  /*
  **	This value indicates the degree of smartness to assign to this house.
  **	A value is zero is presumed for human controlled houses.
  */
  int IQ;

  /*
  **	This is the buildable tech level for this house. This value is used
  **	for when the computer is deciding what objects to build.
  */
  int TechLevel;

  /*
  **	This is the original ally specification to use at scenario
  **	start. Various forces during play may adjust the ally state
  **	of this house.
  */
  int Allies;

  /*
  **	This is the maximum number allowed to be built by this house. The
  **	value depends on the scenario being played.
  */
  unsigned MaxUnit;
  unsigned MaxBuilding;
  int MaxInfantry;
  unsigned MaxVessel;
  int MaxAircraft;

  /*
  **	This records the initial credits assigned to this house when the
  *scenario *	was loaded.
  */
  int64_t InitialCredits;

  /*
  **	For generic (unspecified) reinforcements, they arrive by a common
  *method. This *	specifies which method is to be used.
  */
  SourceType Edge;
};

/****************************************************************************
**	Player control structure. Each player (computer or human) has one of
**	these structures associated. These are located in a global array.
*/
class HouseClass {
 public:
  // Aftermath appended its units after the original lists, but the saved
  // per-type quantity arrays keep the original sizes and Aftermath IDs wrap
  // into them. These mark where the originals end.
  static constexpr int kOriginalUnitCount = UNIT_CHRONOTANK;
  static constexpr int kOriginalInfantryCount = INFANTRY_SHOCK;
  static constexpr int kOriginalVesselCount = VESSEL_MISSILESUB;

  RTTIType RTTI;
  int ID;

  /*
  **	Pointer to the HouseTypeClass that this house is "owned" by.
  **	All constant data for a house type is stored in that class.
  */
  CCPtr<HouseTypeClass> Class;

  /*
  **	This is the handicap (difficulty level) assigned to this house.
  */
  DiffType Difficulty;

  /*
  **	Override handicap control values.
  */
  fixed FirepowerBias{1, 1};
  fixed GroundspeedBias{1, 1};
  fixed AirspeedBias{1, 1};
  fixed ArmorBias{1, 1};
  fixed ROFBias{1, 1};
  fixed CostBias{1, 1};
  fixed BuildSpeedBias{1, 1};
  fixed RepairDelay{0, 1};
  fixed BuildDelay{0, 1};

  /*
  **	The initial house data as loaded from the scenario control file is
  **	stored here. Although this data changes for each scenario, it remains
  **	static for the duration of the current scenario.
  */
  HouseStaticClass Control;

  /*
  **	This is the house type that this house object should act like. This
  **	value controls production choices and radar cover plate imagery.
  */
  HousesType ActLike;

  /*
  **	Is this player active?  Usually that answer is true, but for civilians,
  *it *	might possibly be false.
  */
  unsigned IsActive : 1 = true;

  /*
  **	If this house is controlled by the player, then this flag will be true.
  *The *	computer controls all other active houses.
  */
  unsigned IsHuman : 1 = false;

  /*
  **	If the player can control units of this house even if the player doesn't
  **	own units of this house, then this flag will be true.
  */
  unsigned IsPlayerControl : 1 = false;

  /*
  **	This flag enables production. If the flag is false, production is
  *disabled. *	By timing when this flag gets set, the player can be given some
  *breathing room.
  */
  unsigned IsStarted : 1 = false;

  /*
  **	When alerted, the house will create teams of the special "auto" type and
  **	will generate appropriate units to fill those team types.
  */
  unsigned IsAlerted : 1 = false;

  /*
  **	If automatic base building is on, then this flag will be set to true.
  */
  unsigned IsBaseBuilding : 1 = false;

  /*
  **	If the house has been discovered, then this flag will be set
  **	to true. However, the trigger even associated with discovery
  **	will only be executed during the next house AI process.
  */
  unsigned IsDiscovered : 1 = false;

  /*
  **	If Tiberium storage is maxed out, then this flag will be set. At some
  *point *	the player is told of this fact and then this flag is cleared.
  *This allows the *	player to be told, but only occasionally rather than
  *continuously.
  */
  unsigned IsMaxedOut : 1 = false;

  /*
  ** If this house is played by a human in a multiplayer game, this flag
  ** keeps track of whether this house has been defeated or not.
  */
  unsigned IsDefeated : 1 = false;

  /*
  **	These flags are used in conjunction with the BorrowedTime timer. When
  **	that timer expires and one of these flags are set, then that event is
  **	applied to the house. This allows a dramatic pause between the event
  **	trigger and the result.
  */
  unsigned IsToDie : 1 = false;
  unsigned IsToWin : 1 = false;
  unsigned IsToLose : 1 = false;

  /*
  **	This flag is set when a transport carrying a civilian has been
  **	successfully evacuated. It is presumed that a possible trigger
  **	event will be sprung by this event.
  */
  unsigned IsCivEvacuated : 1 = false;

  /*
  **	If potentially something changed that might affect the sidebar list of
  **	buildable objects, then this flag indicates that at the first LEGAL
  *opportunity, *	the sidebar will be recalculated.
  */
  unsigned IsRecalcNeeded : 1 = true;

  /*
  **	If the map has been completely revealed to the player, then this flag
  **	will be set to true. By examining this flag, a second "reveal all map"
  **	crate won't be given to the player.
  */
  unsigned IsVisionary : 1 = false;

  /*
  **	This flag is set to true when the house has determined that
  **	there is insufficient Tiberium to keep the harvesters busy.
  **	In such a case, the further refinery/harvester production
  **	should cease. This is one of the first signs that the endgame
  **	has begun.
  */
  unsigned IsTiberiumShort : 1 = false;

  /*
  **	These flags are used for the general house trigger events of being
  **	spied and thieved. The appropriate flag will be set when the event
  **	occurs.
  */
  unsigned IsSpied : 1 = false;
  unsigned IsThieved : 1 = false;

  /*
  ** This flag is used to control non-human repairing of buildings.  Each
  ** house gets to repair one building per loop, and this flag controls
  ** whether this house has 'spent' its repair option this time through.
  */
  unsigned DidRepair : 1 = false;

  /*
  ** This flag is used to control whether or not this house has the GPS
  ** satellite in orbit.  If the satellite's there, they have unlimited
  ** radar and the map is fully revealed.
  */
  unsigned IsGPSActive : 1 = false;

  /*
  **	If the JustBuilt??? variable has changed, then this flag will
  **	be set to true.
  */
  unsigned IsBuiltSomething : 1 = false;

  /*
  ** Did this house lose via resignation?
  */
  unsigned IsResigner : 1 = false;

  /*
  ** Did this house lose because the player quit?
  */
  unsigned IsGiverUpper : 1 = false;

  /*
  **	If this computer controlled house has reason to be mad at humans,
  **	then this flag will be true. Such a condition prevents alliances with
  **	a human and encourages the computers players to ally amongst themselves.
  */
  unsigned IsParanoid : 1 = false;

  /*
  **	A gap generator shrouded cells and all units of this house must perform
  **	a look just in case their look radius intersects the shroud area.
  */
  unsigned IsToLook : 1 = true;

  /*
  **	This value indicates the degree of smartness to assign to this house.
  **	A value of zero indicates that the player controls everything.
  */
  int IQ;

  /*
  **	This records the current state of the base. This state is used to
  *control *	what action the base will perform and directly affects
  *production and *	unit disposition. The state will change according to
  *time and combat *	events.
  */
  StateType State = STATE_BUILDUP;

  /*
  **	These super weapon control objects are used to control the recharge
  **	and availability of these special weapons to this house.
  */
  SuperClass SuperWeapon[magic_enum::enum_count<SpecialWeaponType>()];

  /*
  **	This is a record of the last building that was built. For buildings that
  **	were built as a part of scenario creation, it will be the last one
  **	discovered.
  */
  StructType JustBuiltStructure = STRUCT_NONE;
  InfantryType JustBuiltInfantry = INFANTRY_NONE;
  UnitType JustBuiltUnit = UNIT_NONE;
  AircraftType JustBuiltAircraft = AIRCRAFT_NONE;
  VesselType JustBuiltVessel = VESSEL_NONE;

  /*
  **	This records the number of triggers associated with this house that are
  **	blocking a win condition. A win will only occur if all the blocking
  **	triggers have been deleted.
  */
  int Blockage = 0;

  /*
  **	For computer controlled houses, there is an artificial delay between
  **	performing repair actions. This timer regulates that delay. If the
  **	timer has not expired, then no repair initiation is allowed.
  */
  Timer<FrameTickSource> RepairTimer{0};

  /*
  **	This timer controls the computer auto-attack logic. When this timer
  *expires *	and the house has been alerted, then it will create a set of
  *attack *	teams.
  */
  Timer<FrameTickSource> AlertTime{0};

  /*
  **	This timer is used to handle the delay between some catastrophic
  **	event trigger and when it is actually carried out.
  */
  Timer<FrameTickSource> BorrowedTime{0};

  /*
  **	This is the last working scan bits for buildings. If a building is
  **	active and owned by this house, it will have a bit set in this element
  **	that corresponds to the building type number. Since this value is
  **	accumulated over time, the "New" element contains the under-construction
  **	version.
  */
  uint64_t BScan = 0;
  uint64_t ActiveBScan = 0;
  uint64_t OldBScan = 0;

  /*
  **	This is the last working scan bits for units. For every existing unit
  **	type owned by this house, a corresponding bit is set in this element. As
  **	the scan bits are being constructed, they are built into the "New"
  *element *	and then duplicated into the regular element at the end of every
  *logic cycle.
  */
  uint64_t UScan = 0;
  uint64_t ActiveUScan = 0;
  uint64_t OldUScan = 0;

  /*
  **	Infantry type existence bits. Similar to unit and building bits.
  */
  uint64_t IScan = 0;
  uint64_t ActiveIScan = 0;
  uint64_t OldIScan = 0;

  /*
  **	Aircraft type existence bits. Similar to unit and building bits.
  */
  uint64_t AScan = 0;
  uint64_t ActiveAScan = 0;
  uint64_t OldAScan = 0;

  /*
  **	Vessel type existence bits. Similar to unit and building bits.
  */
  uint64_t VScan = 0;
  uint64_t ActiveVScan = 0;
  uint64_t OldVScan = 0;

  /*
  **	Record of gains and losses for this house during the course of the
  **	scenario.
  */
  unsigned CreditsSpent = 0;
  unsigned HarvestedCredits = 0;
  int StolenBuildingsCredits = 0;

  /*
  **	This is the running count of the number of units owned by this house.
  *This *	value is used to keep track of ownership limits.
  */
  unsigned CurUnits = 0;
  int CurBuildings = 0;
  int CurInfantry = 0;
  unsigned CurVessels = 0;
  int CurAircraft = 0;

  /*
  **	This is the running total of the number of credits this house has
  *accumulated.
  */
  // Serialized as 64-bit values to preserve accumulated credits.
  int64_t Tiberium = 0;
  int64_t Credits = 0;
  int64_t Capacity = 0;

  /*
  ** Stuff to keep track of the total number of units built by this house.
  */
  UnitTrackerClass* AircraftTotals = nullptr;
  UnitTrackerClass* InfantryTotals = nullptr;
  UnitTrackerClass* UnitTotals = nullptr;
  UnitTrackerClass* BuildingTotals = nullptr;
  UnitTrackerClass* VesselTotals = nullptr;

  /*
  ** Total number of units destroyed by this house
  */
  UnitTrackerClass* DestroyedAircraft = nullptr;
  UnitTrackerClass* DestroyedInfantry = nullptr;
  UnitTrackerClass* DestroyedUnits = nullptr;
  UnitTrackerClass* DestroyedBuildings = nullptr;
  UnitTrackerClass* DestroyedVessels = nullptr;

  /*
  ** Total number of enemy buildings captured by this house
  */
  UnitTrackerClass* CapturedBuildings = nullptr;

  /*
  ** Total number of crates found by this house
  */
  UnitTrackerClass* TotalCrates = nullptr;

  /*
  **	Records the number of infantry and vehicle factories active. This value
  *is *	used to regulate the speed of production.
  */
  int AircraftFactories = 0;
  int InfantryFactories = 0;
  int UnitFactories = 0;
  int VesselFactories = 0;
  int BuildingFactories = 0;

  /*
  **	This is the accumulation of the total power and drain factors. From
  *these *	values a ratio can be derived. This ratio is used to control the
  *rate *	of building decay.
  */
  int Power = 0;  // Current power output.
  int Drain = 0;  // Power consumption.

  /*
  **	For human controlled houses, only one type of unit can be produced
  **	at any one instant. These factory objects control this production.
  */
  int AircraftFactory = -1;
  int InfantryFactory = -1;
  int UnitFactory = -1;
  int VesselFactory = -1;
  int BuildingFactory = -1;

  /*
  **	This target value specifies where the flag is located. It might be a
  *cell *	or it might be an object.
  */
  TARGET FlagLocation = kTargetNone;

  /*
  ** This is the flag-home-cell for this house.  This is where we must bring
  ** another house's flag back to, to defeat that house.
  */
  CELL FlagHome = 0;

  /*
  ** For multiplayer games, each house needs to keep track of how many
  ** objects of each other house they've killed.
  */
  unsigned UnitsKilled[magic_enum::enum_count<HousesType>()] = {};
  unsigned UnitsLost = 0;
  unsigned BuildingsKilled[magic_enum::enum_count<HousesType>()] = {};
  unsigned BuildingsLost = 0;

  /*
  ** This keeps track of the last house to destroy one of my units.
  ** It's used for scoring multiplayer games.
  */
  HousesType WhoLastHurtMe;

  /*
  **	This records information about the location and size of
  **	the base.
  */
  COORDINATE Center = 0;  // Center of the base.
  int Radius = 0;         // Average building distance from center (leptons).
  struct {
    int AirDefense;
    int ArmorDefense;
    int InfantryDefense;

    template <class Archive>
    void Serialize(Archive& ar) {
      ar(AirDefense, ArmorDefense, InfantryDefense);
    }
  } ZoneInfo[magic_enum::enum_count<ZoneType>()]{};

  /*
  **	This records information about the last time a building of this
  **	side was attacked. This information is used to determine proper
  **	response.
  */
  int LATime = 0;          // Time of attack.
  RTTIType LAType = RTTI_NONE;     // Type of attacker.
  ZoneType LAZone = ZONE_NONE;     // Last zone that was attacked.
  HousesType LAEnemy = HOUSE_NONE;  // Owner of attacker.

  /*
  **	This target value is the building that must be captured as soon as
  *possible. *	Typically, this will be one of the buildings of this house that
  *has been *	captured and needs to be recaptured.
  */
  TARGET ToCapture = kTargetNone;

  /*
  ** This value shows who is spying on this house's radar facilities.
  ** This is used for the other side to be able to update their radar
  ** map based on the cells that this house's units reveal.
  */
  int RadarSpied = 0;

  /*
  ** Running score, based on units destroyed and units lost.
  */
  int PointTotal = 0;

  /*
  **	This is the targeting directions for when this house gets a
  **	special weapon.
  */
  QuarryType PreferredTarget = QUARRY_ANYTHING;

 private:
  /*
  **	Tracks number of each building type owned by this house. Even if the
  **	building is in construction, it will be reflected in this total.
  */
  int BQuantity[magic_enum::enum_count<StructType>() - 3] = {};
  int UQuantity[kOriginalUnitCount - 3] = {};

  int IQuantity[kOriginalInfantryCount] = {};
  int AQuantity[magic_enum::enum_count<AircraftType>()] = {};
  int VQuantity[kOriginalVesselCount] = {};

  /*
  **	This timer keeps track of when an all out attack should be performed.
  **	When this timer expires, send most of this house's units in an
  **	attack.
  */
  Timer<FrameTickSource> Attack{0};

 public:
  /*
  **	This records the overriding enemy that the computer will try to
  **	destroy. Typically, this is the last house to attack, but can be
  **	influenced by nearness.
  */
  HousesType Enemy = HOUSE_NONE;

  /*
  **	The house expert system is regulated by this timer. Each computer
  *controlled *	house will process the Expert System AI at intermittent
  *intervals. Not only will *	this distribute the overhead more evenly, but
  *will add variety to play.
  */
  Timer<FrameTickSource> AITimer{0};

  /*
  ** For the moebius effect, this is a pointer to the unit that we
  ** selected to teleport.  Only one teleporter should be active per house.
  */
  TARGET UnitToTeleport = 0;

  /*
  **	This elaborates the suggested objects to construct. When the specified
  *object *	is constructed, then this corresponding value will be reset to
  *nill state. The *	expert system decides what should be produced, and then
  *records the *	recommendation in these variables.
  */
  StructType BuildStructure = STRUCT_NONE;
  UnitType BuildUnit = UNIT_NONE;
  InfantryType BuildInfantry = INFANTRY_NONE;
  AircraftType BuildAircraft = AIRCRAFT_NONE;
  VesselType BuildVessel = VESSEL_NONE;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t, void* ptr) noexcept { return ptr; }
  void operator delete(void* ptr);
  explicit HouseClass(HousesType house);
  ~HouseClass();
  HouseClass(const HouseClass&) = delete;
  HouseClass& operator=(const HouseClass&) = delete;
  HouseClass(HouseClass&&) = delete;
  HouseClass& operator=(HouseClass&&) = delete;
  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator HousesType() const;

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  [[nodiscard]] CELL Random_Cell_In_Zone(ZoneType zone) const;
  static void Computer_Paranoid();
  [[nodiscard]] bool Is_Allowed_To_Ally(HousesType house) const;
  void Do_All_To_Hunt() const;
  void Super_Weapon_Handler();
  int* Factory_Counter(RTTIType rtti);
  [[nodiscard]] int Factory_Count(RTTIType rtti) const;
  DiffType Assign_Handicap(DiffType handicap);
  [[nodiscard]] TARGET Find_Juicy_Target(COORDINATE coord) const;
  void Print_Zone_Stats(int x, int y, ZoneType zone, MonoClass* mono) const;
  CELL Where_To_Go(const FootClass* object) const;
  [[nodiscard]] CELL Zone_Cell(ZoneType zone) const;
  [[nodiscard]] ZoneType Which_Zone(COORDINATE coord) const;
  ZoneType Which_Zone(const ObjectClass* object) const;
  [[nodiscard]] ZoneType Which_Zone(CELL cell) const;
  CELL Find_Cell_In_Zone(const TechnoClass* techno, ZoneType zone) const;
  ProdFailType Begin_Production(RTTIType type, int id);
  ProdFailType Suspend_Production(RTTIType type);
  ProdFailType Abandon_Production(RTTIType type);
  bool Place_Object(RTTIType type, CELL cell);
  bool Manual_Place(BuildingClass* builder, BuildingClass* object);
  void Special_Weapon_AI(SpecialWeaponType id);
  bool Place_Special_Blast(SpecialWeaponType id, CELL cell);
  bool Flag_Attach(CELL cell, bool set_home = false);
  bool Flag_Attach(UnitClass* object, bool set_home = false);
  bool Flag_Remove(TARGET target, bool set_home = false);
  void Init_Data(PlayerColorType color, HousesType house, int credits);
  COORDINATE Find_Build_Location(BuildingClass* building) const;
  [[nodiscard]] BuildingClass* Find_Building(StructType type,
                                             ZoneType zone = ZONE_NONE) const;
  [[nodiscard]] const char* Name() const { return Class->Name(); }

  bool Fire_Sale();
  [[nodiscard]] bool Is_Hack_Prevented(RTTIType rtti, int value) const;
  [[nodiscard]] bool Is_No_YakMig() const;
  int Expert_AI();
  void Production_Begun(const TechnoClass* product);
  void Sell_Wall(CELL cell);
  bool Flag_To_Die();
  bool Flag_To_Win();
  bool Flag_To_Lose();
  void Make_Ally(HousesType house);
  void Make_Ally(ObjectClass* object) {
    if (object) {
      Make_Ally(object->Owner());
    }
  }
  void Make_Enemy(HousesType house);
  void Make_Enemy(ObjectClass* object) {
    if (object) {
      Make_Enemy(object->Owner());
    }
  }
  [[nodiscard]] bool Is_Ally(HousesType house) const;
  bool Is_Ally(const HouseClass* house) const;
  bool Is_Ally(const ObjectClass* object) const;
  void Debug_Dump(MonoClass* mono) const;
  void AI();
  [[nodiscard]] bool Can_Build(RTTIType rtti, int type, HousesType house) const;

  // Factory controls.
  [[nodiscard]] FactoryClass* Fetch_Factory(RTTIType rtti) const;
  void Set_Factory(RTTIType rtti, FactoryClass* factory);

  bool Can_Build(const ObjectTypeClass* type, HousesType house) const;

  int Get_Quantity(AircraftType aircraft);
  int Get_Quantity(StructType building);
  [[nodiscard]] const unsigned char* Remap_Table(
      bool blushing = false, RemapType remap = REMAP_NORMAL) const;

  [[nodiscard]] const TechnoTypeClass* Suggest_New_Object(
      RTTIType objecttype, bool kennel = false) const;
  [[nodiscard]] const BuildingTypeClass* Suggest_New_Building() const;
  void Recalc_Center();
  [[nodiscard]] bool Does_Enemy_Building_Exist(StructType) const;
  void Harvested(unsigned tiberium);
  void Stole(unsigned worth);
  [[nodiscard]] long Available_Money() const;
  void Spend_Money(unsigned money);
  void Refund_Money(unsigned money);
  void Attacked();
  void Adjust_Power(int adjust);
  void Adjust_Drain(int adjust);
  void Update_Spied_Power_Plants();
  int Adjust_Capacity(int adjust, bool inanger = false);
  [[nodiscard]] fixed Power_Fraction() const;
  [[nodiscard]] fixed Tiberium_Fraction() const;
  void Begin_Production() { IsStarted = true; }
  const TeamTypeClass* Suggested_New_Team(bool alertcheck = false);
  void Adjust_Threat(int region, int threat);
  void Tracking_Remove(const TechnoClass* techno);
  void Tracking_Add(const TechnoClass* techno);
  void Active_Remove(const TechnoClass* techno);
  void Active_Add(const TechnoClass* techno);

  [[nodiscard]] UrgencyType Check_Attack() const;
  [[nodiscard]] UrgencyType Check_Build_Power() const;
  [[nodiscard]] UrgencyType Check_Build_Defense() const;
  [[nodiscard]] UrgencyType Check_Build_Offense() const;
  [[nodiscard]] UrgencyType Check_Build_Income() const;
  [[nodiscard]] UrgencyType Check_Fire_Sale() const;
  [[nodiscard]] UrgencyType Check_Build_Engineer() const;
  [[nodiscard]] UrgencyType Check_Raise_Money() const;
  [[nodiscard]] UrgencyType Check_Raise_Power() const;
  [[nodiscard]] UrgencyType Check_Lower_Power() const;

  bool AI_Attack(UrgencyType urgency);
  // AI helpers report whether they acted; the planner ignores it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  bool AI_Build_Power(UrgencyType urgency) const;
  // AI helpers report whether they acted; the planner ignores it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  bool AI_Build_Defense(UrgencyType urgency) const;
  // AI helpers report whether they acted; the planner ignores it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  bool AI_Build_Offense(UrgencyType urgency) const;
  // AI helpers report whether they acted; the planner ignores it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  bool AI_Build_Income(UrgencyType urgency) const;
  bool AI_Fire_Sale(UrgencyType urgency);
  // AI helpers report whether they acted; the planner ignores it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  bool AI_Build_Engineer(UrgencyType urgency) const;
  // AI helpers report whether they acted; the planner ignores it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  bool AI_Raise_Money(UrgencyType urgency) const;
  // AI helpers report whether they acted; the planner ignores it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  bool AI_Raise_Power(UrgencyType urgency) const;
  // AI helpers report whether they acted; the planner ignores it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  bool AI_Lower_Power(UrgencyType urgency) const;

  [[nodiscard]] bool Can_Make_Money() const {
    return Available_Money() > 300 || BScan & kStructFlagRefinery;
  }

  static void Init();
  static void One_Time();
  static HouseClass* As_Pointer(HousesType house);
  static void Recalc_Attributes();

  /*
  **	File I/O.
  */
  static void Read_INI(CCINIClass& ini);
  static void Write_INI(CCINIClass& ini);
  static void Read_Flag_INI(char* buffer);
  static void Write_Flag_INI(char* buffer);
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	Special house actions.
  */
  void Detach(TARGET target, bool all);

  /*
  **	This vector holds the recorded status of the map regions. It is through
  **	this region information that team paths are calculated.
  */
  RegionClass Regions[kMapTotalRegions];

  /*
  **	This count down timer class decrements and then changes
  ** the Atomic Bomb state.
  */
  CELL NukeDest = 0;

  /*
  ** This routine completely removes this house & all its objects from the game.
  */
  void Clobber_All();

  /*
  ** This routine blows up everything in this house.  Fun!
  */
  void Blowup_All();

  /*
  ** This routine gets called in multiplayer games when every unit, building,
  ** and infantry for a house is destroyed.
  */
  void MPlayer_Defeated();

  /*
  ** When the game's over, this routine assigns everyone their score.
  */
  void Tally_Score();

  friend class MapEditClass;

 private:
  void Silo_Redraw_Check(long oldtib, long oldcap);
  int AI_Building();
  int AI_Unit();
  int AI_Vessel();
  int AI_Infantry();
  int AI_Aircraft();

  /*
  **	This is a bit field record of all the other houses that are allies with
  **	this house. It is presumed that any house that isn't an ally, is
  *therefore *	an enemy. A house is always considered allied with itself.
  */
  unsigned Allies = 0;

  /*
  **	General low-power related damaged is doled out whenever this timer
  **	expires.
  */
  Timer<FrameTickSource> DamageTime;

  /*
  **	Team creation is done whenever this timer expires.
  */
  Timer<FrameTickSource> TeamTime;

  /*
  **	This controls the rate that the trigger time logic is processed.
  */
  Timer<FrameTickSource> TriggerTime{0};

  /*
  **	At various times, the computer may announce the player's condition. The
  *following *	variables are used as countdown timers so that these
  *announcements are paced *	far enough apart to reduce annoyance.
  */
  Timer<FrameTickSource> SpeakAttackDelay{1};
  Timer<FrameTickSource> SpeakPowerDelay{1};
  Timer<FrameTickSource> SpeakMoneyDelay{1};
  Timer<FrameTickSource> SpeakMaxedDelay{1};

  /*
  **	This structure is used to record a build request as determined by
  **	the house AI processing. Higher priority build requests take precidence.
  */
  struct BuildChoiceClass {
    void* operator new(size_t, void* ptr) noexcept { return ptr; }
    UrgencyType Urgency;   // The urgency of the build request
    StructType Structure;  // The type of building to produce.

    BuildChoiceClass(UrgencyType u, StructType s) : Urgency(u), Structure(s) {}
  };

  static TFixedIHeapClass<BuildChoiceClass> BuildChoice;

  // Shell for TFixedIHeapClass::Load; Serialize() supplies every value.
  HouseClass();
  friend class TFixedIHeapClass<HouseClass>;

  // Allocates the per-type statistics trackers an Internet game keeps.
  // Called by both constructors; the trackers are not saved.
  void Init_Trackers();

  /*
  ** These values are for multiplay only.
  */
 public:
  friend class TFixedIHeapClass<BuildChoiceClass>;
  /*
  ** For multiplayer games, each house instance has a remap table; the table
  ** in the HousesTypeClass isn't used.  This variable is set to the remap
  ** table for the color the player wants to play.
  */
  PlayerColorType RemapColor;

  /*
  ** This is the name ("handle") the player has chosen for himself.
  */
  char IniName[HOUSE_NAME_MAX]{};

  // The name this player started the game with. Unlike IniName it is never
  // rewritten to "Computer" when the computer takes over for a player who has
  // left, so end-of-game reporting can still say who was here. Internet games
  // are the only ones that fill it in.
  char InitialName[HOUSE_NAME_MAX]{};

  int QuantityB(int index) { return BQuantity[index]; }
  int QuantityU(int index) {
    if (index >= kOriginalUnitCount) {
      index -= kOriginalUnitCount;
    }
    return UQuantity[index];
  }
  int QuantityI(int index) {
    if (index >= kOriginalInfantryCount) {
      index -= kOriginalInfantryCount;
    }
    return IQuantity[index];
  }
  int QuantityA(int index) { return AQuantity[index]; }
  int QuantityV(int index) {
    if (index >= kOriginalVesselCount) {
      index -= kOriginalVesselCount;
    }
    return VQuantity[index];
  }
};

class ArchiveReader;
class ArchiveWriter;
extern template void HouseClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void HouseClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_RA_HOUSE_H_
