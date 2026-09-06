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

/* $Header: /CounterStrike/IOOBJ.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : IOOBJ.CPP *
 *                                                                                             *
 *                   Programmer : Bill Randolph *
 *                                                                                             *
 *                   Start Date : January 16, 1995 *
 *                                                                                             *
 *                  Last Update : May 13, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * BulletClass::Code_Pointers -- codes class's pointers for
 *load/save                        * BulletClass::Decode_Pointers -- decodes
 *pointers for load/save                            * CargoClass::Code_Pointers
 *-- codes class's pointers for load/save                         *
 *   CargoClass::Decode_Pointers -- decodes pointers for load/save *
 *   FactoryClass::Code_Pointers -- codes class's pointers for load/save *
 *   FactoryClass::Decode_Pointers -- decodes pointers for load/save *
 *   FootClass::Code_Pointers -- codes class's pointers for load/save *
 *   FootClass::Decode_Pointers -- decodes pointers for load/save *
 *   HouseClass::Code_Pointers -- codes class's pointers for load/save *
 *   HouseClass::Decode_Pointers -- decodes pointers for load/save *
 *   LayerClass::Code_Pointers -- codes class's pointers for load/save *
 *   LayerClass::Decode_Pointers -- decodes pointers for load/save *
 *   LayerClass::Load -- Reads from a save game file. * LayerClass::Save --
 *Write to a save game file.                                            *
 *   ObjectClass::Code_Pointers -- codes class's pointers for load/save *
 *   ObjectClass::Decode_Pointers -- decodes pointers for load/save *
 *   RadioClass::Code_Pointers -- codes class's pointers for load/save *
 *   RadioClass::Decode_Pointers -- decodes pointers for load/save *
 *   ReinforcementClass::Code_Pointers -- codes class's pointers for load/save *
 *   ReinforcementClass::Decode_Pointers -- decodes pointers for load/save *
 *   ScoreClass::Code_Pointers -- codes class's pointers for load/save *
 *   ScoreClass::Decode_Pointers -- decodes pointers for load/save *
 *   TeamClass::Code_Pointers -- codes class's pointers for load/save *
 *   TeamClass::Decode_Pointers -- decodes pointers for load/save *
 *   TeamTypeClass::Code_Pointers -- codes class's pointers for load/save *
 *   TeamTypeClass::Decode_Pointers -- decodes pointers for load/save *
 *   TechnoClass::Code_Pointers -- codes class's pointers for load/save *
 *   TechnoClass::Decode_Pointers -- decodes pointers for load/save *
 *   TriggerClass::Code_Pointers -- codes class's pointers for load/save *
 *   TriggerClass::Decode_Pointers -- decodes pointers for load/save *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */
#include <bit>
#include <cassert>
#include <cstdint>

#include "ra/aircraft.h"
#include "ra/anim.h"
#include "ra/building.h"
#include "ra/bullet.h"
#include "ra/cargo.h"
#include "ra/defines.h"
#include "ra/factory.h"
#include "ra/overlay.h"
#include "ra/serialize.h"
#include "ra/smudge.h"
#include "ra/template.h"
#include "ra/terrain.h"
#include "ra/foot.h"
#include "ra/house.h"
#include "ra/infantry.h"
#include "ra/jshell.h"
#include "ra/layer.h"
#include "ra/mission.h"
#include "ra/object.h"
#include "ra/radio.h"
#include "ra/score.h"
#include "ra/taction.h"
#include "ra/target.h"
#include "ra/team.h"
#include "ra/teamtype.h"
#include "ra/techno.h"
#include "ra/trigger.h"
#include "ra/trigtype.h"
#include "ra/type.h"
#include "ra/vector.h"
#include "tech/ftimer.h"
#include "tech/pipe.h"
#include "tech/straw.h"

template <class Archive>
void TeamMemberClass::Serialize(Archive& ar) {
  ar(Quantity, TechnoTypePtr(Class));
}
template void TeamMemberClass::Serialize(ArchiveWriter&);
template void TeamMemberClass::Serialize(ArchiveReader&);

template <class Archive>
void TeamTypeClass::Serialize(Archive& ar) {
  AbstractTypeClass::Serialize(ar);
  bool is_active = IsActive;
  bool is_round_about = IsRoundAbout;
  bool is_suicide = IsSuicide;
  bool is_autocreate = IsAutocreate;
  bool is_prebuilt = IsPrebuilt;
  bool is_reinforcable = IsReinforcable;
  bool is_transient = IsTransient;
  ar(is_active, is_round_about, is_suicide, is_autocreate, is_prebuilt,
     is_reinforcable, is_transient, RecruitPriority, InitNum, MaxAllowed, Fear,
     House, Trigger, Origin, Number, MissionCount, MissionList, ClassCount,
     Members);
  IsActive = is_active;
  IsRoundAbout = is_round_about;
  IsSuicide = is_suicide;
  IsAutocreate = is_autocreate;
  IsPrebuilt = is_prebuilt;
  IsReinforcable = is_reinforcable;
  IsTransient = is_transient;
}
template void TeamTypeClass::Serialize(ArchiveWriter&);
template void TeamTypeClass::Serialize(ArchiveReader&);

template <class Archive>
void TeamClass::Serialize(Archive& ar) {
  AbstractClass::Serialize(ar);
  bool is_forced_active = IsForcedActive;
  bool is_has_been = IsHasBeen;
  bool is_full_strength = IsFullStrength;
  bool is_under_strength = IsUnderStrength;
  bool is_reforming = IsReforming;
  bool is_lagging = IsLagging;
  bool is_altered = IsAltered;
  bool just_altered = JustAltered;
  bool is_moving = IsMoving;
  bool is_next_mission = IsNextMission;
  bool is_leave_map = IsLeaveMap;
  bool suspended = Suspended;
  ar(Class, House, is_forced_active, is_has_been, is_full_strength,
     is_under_strength, is_reforming, is_lagging, is_altered, just_altered,
     is_moving, is_next_mission, is_leave_map, suspended, Zone, ClosestMember,
     MissionTarget, Target, Total, Risk, Formation, SuspendTimer, Trigger,
     CurrentMission, TimeOut, ObjectPtr(Member), Quantity);
  IsForcedActive = is_forced_active;
  IsHasBeen = is_has_been;
  IsFullStrength = is_full_strength;
  IsUnderStrength = is_under_strength;
  IsReforming = is_reforming;
  IsLagging = is_lagging;
  IsAltered = is_altered;
  JustAltered = just_altered;
  IsMoving = is_moving;
  IsNextMission = is_next_mission;
  IsLeaveMap = is_leave_map;
  Suspended = suspended;
}
template void TeamClass::Serialize(ArchiveWriter&);
template void TeamClass::Serialize(ArchiveReader&);

template <class Archive>
void TriggerClass::Serialize(Archive& ar) {
  bool is_active = IsActive;
  ar(RTTI, ID, Class, Event1, Event2, is_active, AttachCount, Cell);
  IsActive = is_active;
}
template void TriggerClass::Serialize(ArchiveWriter&);
template void TriggerClass::Serialize(ArchiveReader&);

template <class Archive>
void TriggerTypeClass::Serialize(Archive& ar) {
  AbstractTypeClass::Serialize(ar);
  bool is_active = IsActive;
  ar(is_active, IsPersistant, House, Event1, Event2, EventControl, Action1,
     Action2, ActionControl);
  IsActive = is_active;
}
template void TriggerTypeClass::Serialize(ArchiveWriter&);
template void TriggerTypeClass::Serialize(ArchiveReader&);

template <class Archive>
void BulletClass::Serialize(Archive& ar) {
  ObjectClass::Serialize(ar);
  FlyClass::Serialize(ar);
  FuseClass::Serialize(ar);
  bool is_inaccurate = IsInaccurate;
  bool is_to_animate = IsToAnimate;
  bool is_locked = IsLocked;
  ar(Class, ObjectPtr(Payback), PrimaryFacing, is_inaccurate, is_to_animate,
     is_locked, TarCom, MaxSpeed, Warhead);
  IsInaccurate = is_inaccurate;
  IsToAnimate = is_to_animate;
  IsLocked = is_locked;
}
template void BulletClass::Serialize(ArchiveWriter&);
template void BulletClass::Serialize(ArchiveReader&);

template <class Archive>
void ObjectClass::Serialize(Archive& ar) {
  AbstractClass::Serialize(ar);
  bool is_down = IsDown;
  bool is_to_damage = IsToDamage;
  bool is_to_display = IsToDisplay;
  bool is_in_limbo = IsInLimbo;
  bool is_selected = IsSelected;
  bool is_anim_attached = IsAnimAttached;
  bool is_falling = IsFalling;
  ar(is_down, is_to_damage, is_to_display, is_in_limbo, is_selected,
     is_anim_attached, is_falling, Riser, ObjectPtr(Next), Trigger, Strength);
  IsDown = is_down;
  IsToDamage = is_to_damage;
  IsToDisplay = is_to_display;
  IsInLimbo = is_in_limbo;
  IsSelected = is_selected;
  IsAnimAttached = is_anim_attached;
  IsFalling = is_falling;
}
template void ObjectClass::Serialize(ArchiveWriter&);
template void ObjectClass::Serialize(ArchiveReader&);

template <class Archive>
void TemplateClass::Serialize(Archive& ar) {
  ObjectClass::Serialize(ar);
  ar(Class);
}
template void TemplateClass::Serialize(ArchiveWriter&);
template void TemplateClass::Serialize(ArchiveReader&);

template <class Archive>
void OverlayClass::Serialize(Archive& ar) {
  ObjectClass::Serialize(ar);
  ar(Class);
}
template void OverlayClass::Serialize(ArchiveWriter&);
template void OverlayClass::Serialize(ArchiveReader&);

template <class Archive>
void SmudgeClass::Serialize(Archive& ar) {
  ObjectClass::Serialize(ar);
  ar(Class);
}
template void SmudgeClass::Serialize(ArchiveWriter&);
template void SmudgeClass::Serialize(ArchiveReader&);

template <class Archive>
void AnimClass::Serialize(Archive& ar) {
  ObjectClass::Serialize(ar);
  StageClass::Serialize(ar);
  bool is_to_delete = IsToDelete;
  bool is_brand_new = IsBrandNew;
  bool is_invisible = IsInvisible;
  ar(Class, xObject, OwnerHouse, Loops, is_to_delete, is_brand_new,
     is_invisible, Delay, Accum);
  IsToDelete = is_to_delete;
  IsBrandNew = is_brand_new;
  IsInvisible = is_invisible;
}
template void AnimClass::Serialize(ArchiveWriter&);
template void AnimClass::Serialize(ArchiveReader&);

template <class Archive>
void TerrainClass::Serialize(Archive& ar) {
  ObjectClass::Serialize(ar);
  StageClass::Serialize(ar);
  bool is_on_fire = IsOnFire;
  bool is_crumbling = IsCrumbling;
  ar(Class, is_on_fire, is_crumbling);
  IsOnFire = is_on_fire;
  IsCrumbling = is_crumbling;
}
template void TerrainClass::Serialize(ArchiveWriter&);
template void TerrainClass::Serialize(ArchiveReader&);

template <class Archive>
void RadioClass::Serialize(Archive& ar) {
  MissionClass::Serialize(ar);
  ar(Old, ObjectPtr(Radio));
}
template void RadioClass::Serialize(ArchiveWriter&);
template void RadioClass::Serialize(ArchiveReader&);

template <class Archive>
void CargoClass::Serialize(Archive& ar) {
  ar(Quantity, ObjectPtr(CargoHold));
}
template void CargoClass::Serialize(ArchiveWriter&);
template void CargoClass::Serialize(ArchiveReader&);

template <class Archive>
void TechnoClass::Serialize(Archive& ar) {
  RadioClass::Serialize(ar);
  FlasherClass::Serialize(ar);
  StageClass::Serialize(ar);
  CargoClass::Serialize(ar);
  DoorClass::Serialize(ar);
  bool is_useless = IsUseless;
  bool is_ticked_off = IsTickedOff;
  bool is_cloakable = IsCloakable;
  bool is_leader = IsLeader;
  bool is_a_loaner = IsALoaner;
  bool is_locked = IsLocked;
  bool is_in_recoil_state = IsInRecoilState;
  bool is_tethered = IsTethered;
  bool is_owned_by_player = IsOwnedByPlayer;
  bool is_discovered_by_player = IsDiscoveredByPlayer;
  bool is_discovered_by_computer = IsDiscoveredByComputer;
  bool is_a_lemon = IsALemon;
  bool is_second_shot = IsSecondShot;
  ar(Crew, is_useless, is_ticked_off, is_cloakable, is_leader, is_a_loaner,
     is_locked, is_in_recoil_state, is_tethered, is_owned_by_player,
     is_discovered_by_player, is_discovered_by_computer, is_a_lemon,
     is_second_shot, ArmorBias, FirepowerBias, IdleTimer, IronCurtainCountDown,
     SpiedBy, ArchiveTarget, House, Cloak, CloakingDevice, CloakDelay, TarCom,
     SuspendedTarCom, PrimaryFacing, Arm, Ammo, PurchasePrice);
  IsUseless = is_useless;
  IsTickedOff = is_ticked_off;
  IsCloakable = is_cloakable;
  IsLeader = is_leader;
  IsALoaner = is_a_loaner;
  IsLocked = is_locked;
  IsInRecoilState = is_in_recoil_state;
  IsTethered = is_tethered;
  IsOwnedByPlayer = is_owned_by_player;
  IsDiscoveredByPlayer = is_discovered_by_player;
  IsDiscoveredByComputer = is_discovered_by_computer;
  IsALemon = is_a_lemon;
  IsSecondShot = is_second_shot;
}
template void TechnoClass::Serialize(ArchiveWriter&);
template void TechnoClass::Serialize(ArchiveReader&);

template <class Archive>
void BuildingClass::Serialize(Archive& ar) {
  TechnoClass::Serialize(ar);
  bool is_to_rebuild = IsToRebuild;
  bool is_to_repair = IsToRepair;
  bool is_allowed_to_sell = IsAllowedToSell;
  bool is_ready_to_commence = IsReadyToCommence;
  bool is_repairing = IsRepairing;
  bool is_wrench_visible = IsWrenchVisible;
  bool is_going_to_blow = IsGoingToBlow;
  bool is_survivorless = IsSurvivorless;
  bool is_charging = IsCharging;
  bool is_charged = IsCharged;
  bool is_captured = IsCaptured;
  bool is_jamming = IsJamming;
  bool is_jammed = IsJammed;
  bool has_fired = HasFired;
  bool has_opened = HasOpened;
  ar(Class, Factory, ActLike, is_to_rebuild, is_to_repair, is_allowed_to_sell,
     is_ready_to_commence, is_repairing, is_wrench_visible, is_going_to_blow,
     is_survivorless, is_charging, is_charged, is_captured, is_jamming,
     is_jammed, has_fired, has_opened, CountDown, BState, QueueBState,
     WhoLastHurtMe, WhomToRepay, LastStrength, AnimToTrack, PlacementDelay);
  IsToRebuild = is_to_rebuild;
  IsToRepair = is_to_repair;
  IsAllowedToSell = is_allowed_to_sell;
  IsReadyToCommence = is_ready_to_commence;
  IsRepairing = is_repairing;
  IsWrenchVisible = is_wrench_visible;
  IsGoingToBlow = is_going_to_blow;
  IsSurvivorless = is_survivorless;
  IsCharging = is_charging;
  IsCharged = is_charged;
  IsCaptured = is_captured;
  IsJamming = is_jamming;
  IsJammed = is_jammed;
  HasFired = has_fired;
  HasOpened = has_opened;
}
template void BuildingClass::Serialize(ArchiveWriter&);
template void BuildingClass::Serialize(ArchiveReader&);

template <class Archive>
void FootClass::Serialize(Archive& ar) {
  TechnoClass::Serialize(ar);
  bool is_scan_limited = IsScanLimited;
  bool is_initiated = IsInitiated;
  bool is_new_nav_com = IsNewNavCom;
  bool is_planning_to_look = IsPlanningToLook;
  bool is_deploying = IsDeploying;
  bool is_firing = IsFiring;
  bool is_rotating = IsRotating;
  bool is_driving = IsDriving;
  bool is_unloading = IsUnloading;
  bool is_formation_move = IsFormationMove;
  bool is_nav_queue_loop = IsNavQueueLoop;
  bool is_scattering = IsScattering;
  ar(is_scan_limited, is_initiated, is_new_nav_com, is_planning_to_look,
     is_deploying, is_firing, is_rotating, is_driving, is_unloading,
     is_formation_move, is_nav_queue_loop, is_scattering, Speed, SpeedBias,
     XFormOffset, YFormOffset, NavCom, SuspendedNavCom, NavQueue, Team, Group,
     ObjectPtr(Member), Path, PathThreshhold, PathDelay, TryTryAgain,
     BaseAttackTimer, FormationSpeed, FormationMaxSpeed, HeadToCoord);
  IsScanLimited = is_scan_limited;
  IsInitiated = is_initiated;
  IsNewNavCom = is_new_nav_com;
  IsPlanningToLook = is_planning_to_look;
  IsDeploying = is_deploying;
  IsFiring = is_firing;
  IsRotating = is_rotating;
  IsDriving = is_driving;
  IsUnloading = is_unloading;
  IsFormationMove = is_formation_move;
  IsNavQueueLoop = is_nav_queue_loop;
  IsScattering = is_scattering;
}
template void FootClass::Serialize(ArchiveWriter&);
template void FootClass::Serialize(ArchiveReader&);

template <class Archive>
void InfantryClass::Serialize(Archive& ar) {
  FootClass::Serialize(ar);
  bool is_technician = IsTechnician;
  bool is_stoked = IsStoked;
  bool is_prone = IsProne;
  bool is_zone_cheat = IsZoneCheat;
  bool was_selected = WasSelected;
  ar(Class, Doing, Comment, is_technician, is_stoked, is_prone, is_zone_cheat,
     was_selected, Fear);
  IsTechnician = is_technician;
  IsStoked = is_stoked;
  IsProne = is_prone;
  IsZoneCheat = is_zone_cheat;
  WasSelected = was_selected;
}
template void InfantryClass::Serialize(ArchiveWriter&);
template void InfantryClass::Serialize(ArchiveReader&);

template <class Archive>
void AircraftClass::Serialize(Archive& ar) {
  FootClass::Serialize(ar);
  FlyClass::Serialize(ar);
  bool is_landing = IsLanding;
  bool is_taking_off = IsTakingOff;
  bool is_homing = IsHoming;
  bool is_hovering = IsHovering;
  ar(Class, SecondaryFacing, Passenger, is_landing, is_taking_off, is_homing,
     is_hovering, Jitter, SightTimer, AttacksRemaining);
  IsLanding = is_landing;
  IsTakingOff = is_taking_off;
  IsHoming = is_homing;
  IsHovering = is_hovering;
}
template void AircraftClass::Serialize(ArchiveWriter&);
template void AircraftClass::Serialize(ArchiveReader&);

template <class Archive>
void FactoryClass::Serialize(Archive& ar) {
  StageClass::Serialize(ar);
  bool is_active = IsActive;
  bool is_suspended = IsSuspended;
  bool is_different = IsDifferent;
  ar(RTTI, ID, is_active, is_suspended, is_different, Balance, OriginalBalance,
     ObjectPtr(Object), SpecialItem, House);
  IsActive = is_active;
  IsSuspended = is_suspended;
  IsDifferent = is_different;
}
template void FactoryClass::Serialize(ArchiveWriter&);
template void FactoryClass::Serialize(ArchiveReader&);

/***********************************************************************************************
 * LayerClass::Load -- Loads from a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to read the cell's data from. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool LayerClass::Load(Straw& file) {
  /*
  **	Read # elements in the layer
  */
  int32_t count;
  if (file.Get(&count, sizeof(count)) != sizeof(count)) {
    return false;
  }

  /*
  **	Clear the array
  */
  Clear();

  /*
  **	Read in all array elements
  */
  for (int index = 0; index < count; index++) {
    ObjectClass* ptr;
    if (file.Get(static_cast<void*>(&ptr), sizeof(ObjectClass*)) !=
        sizeof(ObjectClass*)) {
      return false;
    }
    Add(ptr);
  }

  return true;
}

/***********************************************************************************************
 * LayerClass::Save -- Write to a save game file. *
 *                                                                                             *
 * INPUT:   file  -- The file to write the cell's data to. *
 *                                                                                             *
 * OUTPUT:  true = success, false = failure *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
bool LayerClass::Save(Pipe& file) const {
  /*
  **	Save # array elements
  */
  // Same width as Load reads; base::ssize is 8 bytes here and Load read 4.
  int32_t count = static_cast<int32_t>(Count());
  file.Put(&count, sizeof(count));

  /*
  **	Save all elements
  */
  for (int index = 0; index < count; index++) {
    ObjectClass* ptr = (*this)[index];
    file.Put(static_cast<const void*>(&ptr), sizeof(ObjectClass*));
  }

  return true;
}

/***********************************************************************************************
 * LayerClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void LayerClass::Code_Pointers() {
  for (int index = 0; index < Count(); index++) {
    ObjectClass* obj = (*this)[index];
    assert(obj != nullptr);
    (*this)[index] = (ObjectClass*)obj->As_Target();
  }
}

/***********************************************************************************************
 * LayerClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void LayerClass::Decode_Pointers() {
  for (int index = 0; index < Count(); index++) {
    TARGET target = static_cast<TARGET>((intptr_t)(*this)[index]);
    (*this)[index] = As_Object(target);
    assert((*this)[index] != nullptr);
  }
}

template <class Archive>
void HouseClass::Serialize(Archive& ar) {
  bool is_active = IsActive;
  bool is_human = IsHuman;
  bool is_player_control = IsPlayerControl;
  bool is_started = IsStarted;
  bool is_alerted = IsAlerted;
  bool is_base_building = IsBaseBuilding;
  bool is_discovered = IsDiscovered;
  bool is_maxed_out = IsMaxedOut;
  bool is_defeated = IsDefeated;
  bool is_to_die = IsToDie;
  bool is_to_win = IsToWin;
  bool is_to_lose = IsToLose;
  bool is_civ_evacuated = IsCivEvacuated;
  bool is_recalc_needed = IsRecalcNeeded;
  bool is_visionary = IsVisionary;
  bool is_tiberium_short = IsTiberiumShort;
  bool is_spied = IsSpied;
  bool is_thieved = IsThieved;
  bool did_repair = DidRepair;
  bool is_gps_active = IsGPSActive;
  bool is_built_something = IsBuiltSomething;
  bool is_resigner = IsResigner;
  bool is_giver_upper = IsGiverUpper;
  bool is_paranoid = IsParanoid;
  bool is_to_look = IsToLook;
  ar(RTTI, ID, Class, Difficulty, FirepowerBias, GroundspeedBias, AirspeedBias,
     ArmorBias, ROFBias, CostBias, BuildSpeedBias, RepairDelay, BuildDelay,
     Control, ActLike, is_active, is_human, is_player_control, is_started, is_alerted, is_base_building, is_discovered, is_maxed_out, is_defeated, is_to_die, is_to_win, is_to_lose, is_civ_evacuated, is_recalc_needed, is_visionary, is_tiberium_short, is_spied, is_thieved, did_repair, is_gps_active, is_built_something, is_resigner, is_giver_upper, is_paranoid, is_to_look);
  ar(IQ, State, SuperWeapon, JustBuiltStructure, JustBuiltInfantry,
     JustBuiltUnit, JustBuiltAircraft, JustBuiltVessel, Blockage, RepairTimer,
     AlertTime, BorrowedTime, BScan, ActiveBScan, OldBScan, UScan, ActiveUScan,
     OldUScan, IScan, ActiveIScan, OldIScan, AScan, ActiveAScan, OldAScan,
     VScan, ActiveVScan, OldVScan, CreditsSpent, HarvestedCredits,
     StolenBuildingsCredits, CurUnits, CurBuildings, CurInfantry, CurVessels,
     CurAircraft, Tiberium, Credits, Capacity);
  // The UnitTrackerClass statistics are runtime-only; Init_Trackers()
  // recreates them on the shell object before Serialize() runs.
  ar(AircraftFactories, InfantryFactories, UnitFactories, VesselFactories,
     BuildingFactories, Power, Drain, AircraftFactory, InfantryFactory,
     UnitFactory, VesselFactory, BuildingFactory, FlagLocation, FlagHome,
     UnitsKilled, UnitsLost, BuildingsKilled, BuildingsLost, WhoLastHurtMe,
     Center, Radius, ZoneInfo, LATime, LAType, LAZone, LAEnemy, ToCapture,
     RadarSpied, PointTotal, PreferredTarget, BQuantity, UQuantity, IQuantity,
     AQuantity, VQuantity, Attack, Enemy, AITimer, UnitToTeleport,
     BuildStructure, BuildUnit, BuildInfantry, BuildAircraft, BuildVessel,
     Regions, NukeDest, Allies, DamageTime, TeamTime, TriggerTime,
     SpeakAttackDelay, SpeakPowerDelay, SpeakMoneyDelay, SpeakMaxedDelay,
     RemapColor, IniName, InitialName);
  IsActive = is_active;
  IsHuman = is_human;
  IsPlayerControl = is_player_control;
  IsStarted = is_started;
  IsAlerted = is_alerted;
  IsBaseBuilding = is_base_building;
  IsDiscovered = is_discovered;
  IsMaxedOut = is_maxed_out;
  IsDefeated = is_defeated;
  IsToDie = is_to_die;
  IsToWin = is_to_win;
  IsToLose = is_to_lose;
  IsCivEvacuated = is_civ_evacuated;
  IsRecalcNeeded = is_recalc_needed;
  IsVisionary = is_visionary;
  IsTiberiumShort = is_tiberium_short;
  IsSpied = is_spied;
  IsThieved = is_thieved;
  DidRepair = did_repair;
  IsGPSActive = is_gps_active;
  IsBuiltSomething = is_built_something;
  IsResigner = is_resigner;
  IsGiverUpper = is_giver_upper;
  IsParanoid = is_paranoid;
  IsToLook = is_to_look;
}
template void HouseClass::Serialize(ArchiveWriter&);
template void HouseClass::Serialize(ArchiveReader&);

/***********************************************************************************************
 * ScoreClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void ScoreClass::Code_Pointers() { RealTime.Stop(); }

/***********************************************************************************************
 * ScoreClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void ScoreClass::Decode_Pointers() { RealTime.Start(); }

/***********************************************************************************************
 * FootClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void FootClass::Code_Pointers() {
  if (Member != nullptr && Member->IsActive) {
    Member = (FootClass*)Member->As_Target();
  } else {
    Member = (FootClass*)kTargetNone;
  }

  TechnoClass::Code_Pointers();
}

/***********************************************************************************************
 * FootClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void FootClass::Decode_Pointers() {
  if (static_cast<TARGET>((intptr_t)Member) != kTargetNone) {
    Member = dynamic_cast<FootClass*>(As_Techno((TARGET)(intptr_t)Member));
    assert(Member != nullptr);
  }

  TechnoClass::Decode_Pointers();
}

/***********************************************************************************************
 * RadioClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void RadioClass::Code_Pointers() {
  /*
  **	Code 'Radio'
  */
  if (Radio) {
    Radio = (RadioClass*)Radio->As_Target();
  }

  MissionClass::Code_Pointers();
}

/***********************************************************************************************
 * RadioClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void RadioClass::Decode_Pointers() {
  /*
  **	Decode 'Radio'
  */
  if (Radio) {
    Radio = As_Techno(static_cast<TARGET>((intptr_t)Radio));
    assert(Radio != nullptr);
  }

  MissionClass::Decode_Pointers();
}

/***********************************************************************************************
 * TechnoClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void TechnoClass::Code_Pointers() {
  CargoClass::Code_Pointers();
  RadioClass::Code_Pointers();
}

/***********************************************************************************************
 * TechnoClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void TechnoClass::Decode_Pointers() {
  CargoClass::Decode_Pointers();
  RadioClass::Decode_Pointers();
}

/***********************************************************************************************
 * CargoClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void CargoClass::Code_Pointers() {
  /*
  **	Code 'CargoHold'
  */
  if (CargoHold) {
    CargoHold = (FootClass*)CargoHold->As_Target();
  }
}

/***********************************************************************************************
 * CargoClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void CargoClass::Decode_Pointers() {
  /*
  **	Decode 'CargoHold'
  */
  if (CargoHold) {
    CargoHold =
        dynamic_cast<FootClass*>(As_Techno((TARGET)(intptr_t)CargoHold));
    assert(CargoHold != nullptr);
  }
}

/***********************************************************************************************
 * ObjectClass::Code_Pointers -- codes class's pointers for load/save *
 *                                                                                             *
 * This routine "codes" the pointers in the class by converting them to a number
 ** that still represents the object pointed to, but isn't actually a pointer.
 *This            * allows a saved game to properly load without relying on the
 *games data still                * being in the exact same location. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void ObjectClass::Code_Pointers() {
  if (Next) {
    Next = (ObjectClass*)Next->As_Target();
  }
}

/***********************************************************************************************
 * ObjectClass::Decode_Pointers -- decodes pointers for load/save *
 *                                                                                             *
 * This routine "decodes" the pointers coded in Code_Pointers by converting the
 ** code values back into object pointers. *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/02/1995 BR : Created. *
 *=============================================================================================*/
void ObjectClass::Decode_Pointers() {
  if (Next) {
    Next = As_Object(static_cast<TARGET>(std::bit_cast<intptr_t>(Next)));
    assert(Next != nullptr);
  }
}
