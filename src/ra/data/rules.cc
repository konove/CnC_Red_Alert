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

/* $Header: /CounterStrike/RULES.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : RULES.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/12/96 *
 *                                                                                             *
 *                  Last Update : September 10, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Difficulty_Get -- Fetch the difficulty bias values. *
 *   RulesClass::AI -- Processes the AI control constants from the database. *
 *   RulesClass::General -- Process the general main game rules. *
 *   RulesClass::Heap_Maximums -- Fetch and process the heap override values. *
 *   RulesClass::IQ -- Fetches the IQ control values from the INI database. *
 *   RulesClass::Land_Types -- Inits the land type values. * RulesClass::MPlayer
 *-- Fetch and process the multiplayer default settings.                *
 *   RulesClass::Powerups -- Process the powerup values from the database. *
 *   RulesClass::Process -- Fetch the bulk of the rule data from the control
 *file.             * RulesClass::Recharge -- Process the super weapon recharge
 *statistics.                     * RulesClass::RulesClass -- Default
 *constructor for rules class object.                     * RulesClass::Themes
 *-- Fetches the theme control values from the INI database.             *
 *   Techno_Get -- Get rule data common for all techno type objects. *
 *   _Scale_To_256 -- Scales a 1..100 number into a 1..255 number. *
 *   RulesClass::Difficulty -- Fetch the various difficulty group settings. *
 *   RulesClass::Objects -- Fetch all the object characteristic values. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/data/rules.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

#include "absl/strings/ascii.h"
#include "base/enum_array.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "engine/file/ini_file.h"
#include "game/core/cell_size.h"
#include "game/core/object_heap.h"
#include "game/sim/enums.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/assets/theme.h"
#include "ra/core/const.h"
#include "ra/core/defines.h"
#include "ra/core/type_heaps.h"
#include "ra/data/ini_types.h"
#include "ra/map/vortex.h"
#include "ra/map/world.h"
#include "ra/objects/anim.h"
#include "ra/objects/mission.h"
#include "ra/objects/type.h"
#include "ra/objects/warhead.h"
#include "ra/objects/weapon.h"

namespace ra {

/***********************************************************************************************
 * _Scale_To_256 -- Scales a 1..100 number into a 1..255 number. *
 *                                                                                             *
 *    This is a helper routine that will take a decimal percentage number and
 *convert it       * into a game based fixed point number. *
 *                                                                                             *
 * INPUT:   val   -- Decimal percent number to convert. *
 *                                                                                             *
 * OUTPUT:  Returns with the decimal percent number converted to a game fixed
 *point number.    *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/17/1996 JLB : Created. *
 *=============================================================================================*/
static inline int Scale_To_256(int val) {
  val = Fixed(100, 256) * val;
  val = std::min(val, 255);
  return val;
}

/***********************************************************************************************
 * RulesClass::RulesClass -- Default constructor for rules class object. *
 *                                                                                             *
 *    This is the default constructor for the rules class object. Although it
 *initializes the  * rule data with default values, it is expected that they
 *will all be overridden by the    * rules control file. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/17/1996 JLB : Created. *
 *=============================================================================================*/
RulesClass::RulesClass()
    : TurboBoost(Fixed::FromString("1.5")),
      AttackInterval(3),
      AttackDelay(5),
      PowerEmergencyFraction(Fixed(3, 4)),

      AirstripRatio(Fixed::FromString(".12")),

      HelipadRatio(Fixed::FromString(".12")),

      TeslaRatio(Fixed::FromString(".16")),

      AARatio(Fixed::FromString(".14")),

      DefenseRatio(Fixed::FromString(".5")),

      WarRatio(Fixed::FromString(".1")),

      BarracksRatio(Fixed::FromString(".16")),

      RefineryRatio(Fixed::FromString(".16")),

      ChronoDuration(3),
      WaterCrateChance(Fixed::FromString(".2")),

      PatrolTime(Fixed::FromString(".016")),
      TeamDelay(Fixed::FromString(".6")),
      CloakDelay(0),
      GameSpeedBias(1),
      NervousBias(1),

      VortexChance(Fixed::FromString(".2")),
      ExplosionSpread(Fixed(1, 2)),

      MessageDelay(Fixed::FromString(".6")),
      SavourDelay(Fixed::FromString(".03")),

      BaseDefenseDelay(Fixed(1, 4)),

      SuspendDelay(2),
      SurvivorFraction(Fixed(1, 2)),
      ReloadRate(Fixed::FromString(".05")),
      AutocreateTime(5),
      BuildupTime(Fixed::FromString(".05")),

      ProneDamageBias(Fixed(1, 2)),
      QuakeDamagePercent(Fixed::FromString(".33")),
      QuakeChance(Fixed::FromString(".2")),
      GrowthRate(2),
      ShroudRate(4),
      CrateTime(10),
      TimerWarning(2),
      SonarTime(14),
      ChronoTime(3),
      ParaBombTime(14),
      ParaInfantryTime(2),
      ParaSaboteurTime(14),
      SpyTime(2),
      IronCurtainTime(14),
      GPSTime(1),
      NukeTime(14),
      SpeakDelay(2),
      DamageDelay(1),

      GapRegenInterval(Fixed::FromString(".1")),

      RepairPercent(Fixed(1, 4)),

      URepairPercent(Fixed(1, 4)),
      RepairRate(Fixed::FromString(".016")),
      ConditionGreen(1),
      ConditionYellow(Fixed(1, 2)),
      ConditionRed(Fixed(1, 4)),
      RandomAnimateTime(Fixed::FromString(".083")),

      RefundPercent(Fixed(1, 2)),
      IronCurtainDuration(Fixed(1, 2)),

      BuildSpeedBias(1),
      C4Delay(Fixed::FromString(".03")),

      PathDelay(Fixed::FromString(".016")),
      MovieTime(Fixed(1, 4)),
      // Declared after ConditionRed, so it picks up that rule's default.
      // Reading a different ConditionRed from the rules file does not move
      // it, which is how it has always behaved.
      EngineerCaptureLevel(ConditionRed) {}

/***********************************************************************************************
 * Difficulty_Get -- Fetch the difficulty bias values. *
 *                                                                                             *
 *    This will fetch the difficulty bias values for the section specified. *
 *                                                                                             *
 * INPUT:   ini   -- Reference the INI database to fetch the values from. *
 *                                                                                             *
 *          diff  -- Reference to the difficulty class object to fill in with
 *the values.      *
 *                                                                                             *
 *          section  -- The section identifier to lift the values from. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/11/1996 JLB : Created. *
 *=============================================================================================*/
static void Difficulty_Get(const engine::file::IniFile& ini,
                           DifficultyClass& diff, const char* section) {
  if (ini.HasSection(section)) {
    diff.FirepowerBias = ini.GetFixed(section, "FirePower", Fixed(1));
    diff.GroundspeedBias = ini.GetFixed(section, "Groundspeed", Fixed(1));
    diff.AirspeedBias = ini.GetFixed(section, "Airspeed", Fixed(1));
    diff.ArmorBias = ini.GetFixed(section, "Armor", Fixed(1));
    diff.ROFBias = ini.GetFixed(section, "ROF", Fixed(1));
    diff.CostBias = ini.GetFixed(section, "Cost", Fixed(1));
    diff.RepairDelay =
        ini.GetFixed(section, "RepairDelay", Fixed::FromString(".02"));
    diff.BuildDelay =
        ini.GetFixed(section, "BuildDelay", Fixed::FromString(".03"));
    diff.IsBuildSlowdown = ini.GetBool(section, "BuildSlowdown", false);
    diff.BuildSpeedBias = ini.GetFixed(section, "BuildTime", Fixed(1));
    diff.IsWallDestroyer = ini.GetBool(section, "DestroyWalls", true);
    diff.IsContentScan = ini.GetBool(section, "ContentScan", false);
  }
}

/***********************************************************************************************
 * RulesClass::Process -- Fetch the bulk of the rule data from the control file.
 **
 *                                                                                             *
 *    This routine will fetch the rule data from the control file. *
 *                                                                                             *
 * INPUT:   file  -- Reference to the rule file to process. *
 *                                                                                             *
 * OUTPUT:  bool; Was the rule file processed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/17/1996 JLB : Created. *
 *=============================================================================================*/
bool RulesClass::Process(engine::file::IniFile& ini) {
  General(ini);
  MPlayer(ini);
  Recharge(ini);
  Heap_Maximums(ini);
  AI(ini);
  Powerups(ini);
  Land_Types(ini);
  Themes(ini);
  IQ(ini);
  Objects(ini);
  Difficulty(ini);

  return true;
}

/***********************************************************************************************
 * RulesClass::General -- Process the general main game rules. *
 *                                                                                             *
 *    This fetches the control constants uses for regular game processing. Any
 *game behavior   * controlling values that don't properly fit in any of the
 *other catagories will be        * stored here. *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the database to fetch the values from. *
 *                                                                                             *
 * OUTPUT:  bool; Was the general section found and processed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1996 JLB : Created. *
 *=============================================================================================*/
bool RulesClass::General(engine::file::IniFile& ini) {
  static const char* const GENERAL = "General";
  static const char* const AFTERMATH = "Aftermath";

  if (ini.HasSection(AFTERMATH)) {
    // debugprint( "NewUnitsEnabled previously %i\n", NewUnitsEnabled );
    NewUnitsEnabled = ini.GetInt(AFTERMATH, "NewUnitsEnabled", 0) != 0;
    // debugprint( "NewUnitsEnabled set to %i by Rules\n", NewUnitsEnabled );
    MTankDistance = ini.GetInt(AFTERMATH, "MTankDistance", MTankDistance);
    QuakeUnitDamage =
        ini.GetFixed(AFTERMATH, "QuakeUnitDamage", QuakeUnitDamage);
    QuakeBuildingDamage =
        ini.GetFixed(AFTERMATH, "QuakeBuildingDamage", QuakeBuildingDamage);
    QuakeInfantryDamage =
        ini.GetInt(AFTERMATH, "QuakeInfantryDamage", QuakeInfantryDamage);
    QuakeDelay = ini.GetInt(AFTERMATH, "QuakeDelay", QuakeDelay);
    ChronoTankDuration =
        ini.GetFixed(AFTERMATH, "ChronoTankDuration", ChronoTankDuration);
    CarrierLaunchDelay = ini.GetInt(AFTERMATH, "CarrierLaunchDelay", 120);
    //	Engineer changing fields were specifically left out of Aftrmath.ini,
    // thus these values are not found to set. ajw 	Implies interesting
    // security hole if user creates a separate Aftrmath.ini file!
    EngineerDamage =
        ini.GetFixed(AFTERMATH, "EngineerDamage",
                     EngineerDamage);  // Amount of damage an engineer does
    EngineerCaptureLevel =
        ini.GetFixed(AFTERMATH, "EngineerCaptureLevel",
                     EngineerCaptureLevel);  // Building damage level before
                                             // engineer can capture
  }

  if (ini.HasSection(GENERAL)) {
    TurboBoost = ini.GetFixed(GENERAL, "TurboBoost", TurboBoost);
    BadgerBombCount = ini.GetInt(GENERAL, "BadgerBombCount", BadgerBombCount);
    IsCurleyShuffle = ini.GetBool(GENERAL, "CurleyShuffle", IsCurleyShuffle);
    IsFlashLowPower = ini.GetBool(GENERAL, "FlashLowPower", IsFlashLowPower);
    IsChronoKill = ini.GetBool(GENERAL, "ChronoKillCargo", IsChronoKill);
    ChronoDuration = ini.GetFixed(GENERAL, "ChronoDuration", ChronoDuration);
    IsFineDifficulty =
        ini.GetBool(GENERAL, "FineDiffControl", IsFineDifficulty);
    WaterCrateChance =
        ini.GetFixed(GENERAL, "WaterCrateChance", WaterCrateChance);
    SoloCrateMoney = ini.GetInt(GENERAL, "SoloCrateMoney", SoloCrateMoney);
    ParaBombTechLevel = ini.GetInt(GENERAL, "ParabombTech", ParaBombTechLevel);
    GPSTechLevel = ini.GetInt(GENERAL, "GPSTechLevel", GPSTechLevel);
    UnitCrateType = GetUnitType(ini, GENERAL, "UnitCrateType", UnitCrateType);
    IsExplosiveHarvester = ini.GetFixed(GENERAL, "OreExplosive",
                                        Fixed(IsExplosiveHarvester ? 1 : 0))
                               .ToInt() != 0;
    GapRegenInterval =
        ini.GetFixed(GENERAL, "GapRegenInterval", GapRegenInterval);
    TeamDelay = ini.GetFixed(GENERAL, "TeamDelay", TeamDelay);
    CloakDelay = ini.GetFixed(GENERAL, "SubmergeDelay", CloakDelay);
    GameSpeedBias = ini.GetFixed(GENERAL, "GameSpeedBias", GameSpeedBias);
    NervousBias = ini.GetFixed(GENERAL, "BaseBias", NervousBias);
    ExplosionSpread = ini.GetFixed(GENERAL, "ExpSpread", ExplosionSpread);
    SupressRadius = GetLepton(ini, GENERAL, "FireSupress", SupressRadius);
    ParaInfantryTechLevel =
        ini.GetInt(GENERAL, "ParaTech", ParaInfantryTechLevel);
    SpyPlaneTechLevel = ini.GetInt(GENERAL, "SpyPlaneTech", SpyPlaneTechLevel);
    SilverCrate = GetCrateType(ini, GENERAL, "SilverCrate", SilverCrate);
    WoodCrate = GetCrateType(ini, GENERAL, "WoodCrate", WoodCrate);
    WaterCrate = GetCrateType(ini, GENERAL, "WaterCrate", WaterCrate);
    CrateMinimum = ini.GetInt(GENERAL, "CrateMinimum", CrateMinimum);
    CrateMaximum = ini.GetInt(GENERAL, "CrateMaximum", CrateMaximum);
    IsScatter = ini.GetBool(GENERAL, "PlayerScatter", IsScatter);
    IsSmartDefense = ini.GetBool(GENERAL, "PlayerReturnFire", IsSmartDefense);
    IsAutoCrush = ini.GetBool(GENERAL, "PlayerAutoCrush", IsAutoCrush);
    IsNamed = ini.GetBool(GENERAL, "NamedCivilians", IsNamed);
    IsTGrowth = ini.GetBool(GENERAL, "OreGrows", IsTGrowth);
    IsTSpread = ini.GetBool(GENERAL, "OreSpreads", IsTSpread);
    IsMineAware = ini.GetBool(GENERAL, "MineAware", IsMineAware);
    IsTreeTarget = ini.GetBool(GENERAL, "TreeTargeting", IsTreeTarget);
    IsSeparate = ini.GetBool(GENERAL, "SeparateAircraft", IsSeparate);
    DropZoneRadius = GetLepton(ini, GENERAL, "DropZoneRadius", DropZoneRadius);
    MessageDelay = ini.GetFixed(GENERAL, "MessageDelay", MessageDelay);
    SavourDelay = ini.GetFixed(GENERAL, "SavourDelay", SavourDelay);
    AVMineDamage = ini.GetInt(GENERAL, "AVMineDamage", AVMineDamage);
    APMineDamage = ini.GetInt(GENERAL, "APMineDamage", APMineDamage);
    BaseDefenseDelay =
        ini.GetFixed(GENERAL, "BaseDefenseDelay", BaseDefenseDelay);
    SuspendPriority = ini.GetInt(GENERAL, "SuspendPriority", SuspendPriority);
    SuspendDelay = ini.GetFixed(GENERAL, "SuspendDelay", SuspendDelay);
    SurvivorFraction = ini.GetFixed(GENERAL, "SurvivorRate", SurvivorFraction);
    RadarJamRadius = GetLepton(ini, GENERAL, "RadarJamRadius", RadarJamRadius);
    ReloadRate = ini.GetFixed(GENERAL, "ReloadRate", ReloadRate);
    RandomAnimateTime =
        ini.GetFixed(GENERAL, "IdleActionFrequency", RandomAnimateTime);
    BuildupTime = ini.GetFixed(GENERAL, "BuildupTime", BuildupTime);
    OreDumpRate = ini.GetInt(GENERAL, "OreTruckRate", OreDumpRate);
    AtomDamage = ini.GetInt(GENERAL, "AtomDamage", AtomDamage);
    BailCount = ini.GetInt(GENERAL, "BailCount", BailCount);
    BallisticScatter =
        GetLepton(ini, GENERAL, "BallisticScatter", BallisticScatter);
    BridgeStrength = ini.GetInt(GENERAL, "BridgeStrength", BridgeStrength);
    BuildSpeedBias = ini.GetFixed(GENERAL, "BuildSpeed", BuildSpeedBias);
    ConditionGreen = Fixed(1);
    ConditionRed = ini.GetFixed(GENERAL, "ConditionRed", ConditionRed);
    ConditionYellow = ini.GetFixed(GENERAL, "ConditionYellow", ConditionYellow);
    CrateRadius = GetLepton(ini, GENERAL, "CrateRadius", CrateRadius);
    CrushDistance = GetLepton(ini, GENERAL, "Crush", CrushDistance);
    DamageDelay = ini.GetFixed(GENERAL, "DamageDelay", DamageDelay);
    GapShroudRadius = ini.GetInt(GENERAL, "GapRadius", GapShroudRadius);
    GemValue = ini.GetInt(GENERAL, "GemValue", GemValue);
    GoldValue = ini.GetInt(GENERAL, "GoldValue", GoldValue);
    Gravity = ini.GetInt(GENERAL, "Gravity", Gravity);
    GrowthRate = ini.GetFixed(GENERAL, "GrowthRate", GrowthRate);
    HomingScatter = GetLepton(ini, GENERAL, "HomingScatter", HomingScatter);
    Incoming = GetMPHType(ini, GENERAL, "Incoming", game::MPH_IMMOBILE);
    IronCurtainDuration =
        ini.GetFixed(GENERAL, "IronCurtain", IronCurtainDuration);
    IsAllyReveal = ini.GetBool(GENERAL, "AllyReveal", IsAllyReveal);
    IsHealthBar = ini.GetBool(GENERAL, "EnemyHealth", IsHealthBar);
    IsMCVDeploy = ini.GetBool(GENERAL, "MCVUndeploy", IsMCVDeploy);
    MaxDamage = ini.GetInt(GENERAL, "MaxDamage", MaxDamage);
    MinDamage = ini.GetInt(GENERAL, "MinDamage", MinDamage);
    ProneDamageBias = ini.GetFixed(GENERAL, "ProneDamage", ProneDamageBias);
    QuakeDamagePercent =
        ini.GetFixed(GENERAL, "QuakeDamage", QuakeDamagePercent);
    QuakeChance = ini.GetFixed(GENERAL, "QuakeChance", QuakeChance);
    RefundPercent = ini.GetFixed(GENERAL, "RefundPercent", RefundPercent);
    RepairPercent = ini.GetFixed(GENERAL, "RepairPercent", RepairPercent);
    RepairStep = ini.GetInt(GENERAL, "RepairStep", RepairStep);
    URepairPercent = ini.GetFixed(GENERAL, "URepairPercent", URepairPercent);
    URepairStep = ini.GetInt(GENERAL, "URepairStep", URepairStep);
    RepairRate = ini.GetFixed(GENERAL, "RepairRate", RepairRate);
    ShroudRate = ini.GetFixed(GENERAL, "ShroudRate", ShroudRate);
    SpeakDelay = ini.GetFixed(GENERAL, "SpeakDelay", SpeakDelay);
    StrayDistance = GetLepton(ini, GENERAL, "Stray", StrayDistance);
    CloseEnoughDistance =
        GetLepton(ini, GENERAL, "CloseEnough", CloseEnoughDistance);
    TimerWarning = ini.GetFixed(GENERAL, "TimerWarning", TimerWarning);
    MovieTime = ini.GetFixed(GENERAL, "MovieTime", MovieTime);
    C4Delay = ini.GetFixed(GENERAL, "C4Delay", C4Delay);
    ChronoTechLevel = ini.GetInt(GENERAL, "ChronoTechLevel", ChronoTechLevel);
    CrateTime = ini.GetFixed(GENERAL, "CrateRegen", CrateTime);
    VortexRange = GetLepton(ini, GENERAL, "VortexRange", VortexRange);
    VortexSpeed = static_cast<game::MPHType>(Scale_To_256(
        ini.GetInt(GENERAL, "VortexSpeed", static_cast<int>(VortexSpeed))));
    VortexDamage = ini.GetInt(GENERAL, "VortexDamage", VortexDamage);
    VortexChance = ini.GetFixed(GENERAL, "VortexChance", VortexChance);

    TheWorld().chronal_vortex().Set_Range(VortexRange /
                                          game::kCellWidthLeptons);
    TheWorld().chronal_vortex().Set_Speed(static_cast<int>(VortexSpeed));
    TheWorld().chronal_vortex().Set_Damage(VortexDamage);

    // ChronalVortex.Set_Range ( ini.GetInt (GENERAL, "VortexRange",
    // ChronalVortex.Get_Range() ) ); ChronalVortex.Set_Speed ( ini.GetInt
    // (GENERAL, "VortexSpeed", ChronalVortex.Get_Speed() ) );
    // ChronalVortex.Set_Damage ( ini.GetInt (GENERAL, "VortexDamage",
    // ChronalVortex.Get_Damage() ) );

    return true;
  }
  return false;
}

/***********************************************************************************************
 * RulesClass::MPlayer -- Fetch and process the multiplayer default settings. *
 *                                                                                             *
 *    This is used to set the default settings for the multiplayer system. *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database. *
 *                                                                                             *
 * OUTPUT:  bool; Was the multiplayer default override section found and
 *processed?            *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1996 JLB : Created. *
 *=============================================================================================*/
bool RulesClass::MPlayer(engine::file::IniFile& ini) {
  static const char* const MPLAYER = "MultiplayerDefaults";
  if (ini.HasSection(MPLAYER)) {
    MPDefaultMoney = ini.GetInt(MPLAYER, "Money", MPDefaultMoney);
    MPMaxMoney = ini.GetInt(MPLAYER, "MaxMoney", MPMaxMoney);
    IsMPShadowGrow = ini.GetBool(MPLAYER, "ShadowGrow", IsMPShadowGrow);
    IsMPBasesOn = ini.GetBool(MPLAYER, "Bases", IsMPBasesOn);
    IsMPTiberiumGrow = ini.GetBool(MPLAYER, "OreGrows", IsMPTiberiumGrow);
    IsMPCrates = ini.GetBool(MPLAYER, "Crates", IsMPCrates);
    IsMPCaptureTheFlag =
        ini.GetBool(MPLAYER, "CaptureTheFlag", IsMPCaptureTheFlag);
    return true;
  }
  return false;
}

/***********************************************************************************************
 * RulesClass::Recharge -- Process the super weapon recharge statistics. *
 *                                                                                             *
 *    Use this to set the recharge times for the various super weapons
 *available.              *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the database. *
 *                                                                                             *
 * OUTPUT:  bool; Was the recharge section found and processed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1996 JLB : Created. *
 *=============================================================================================*/
bool RulesClass::Recharge(engine::file::IniFile& ini) {
  static const char* const RECHARGE = "Recharge";
  if (ini.HasSection(RECHARGE)) {
    SonarTime = ini.GetFixed(RECHARGE, "Sonar", SonarTime);
    ChronoTime = ini.GetFixed(RECHARGE, "Chrono", ChronoTime);
    ParaBombTime = ini.GetFixed(RECHARGE, "ParaBomb", ParaBombTime);
    ParaInfantryTime = ini.GetFixed(RECHARGE, "Paratrooper", ParaInfantryTime);
    SpyTime = ini.GetFixed(RECHARGE, "SpyPlane", SpyTime);
    IronCurtainTime = ini.GetFixed(RECHARGE, "IronCurtain", IronCurtainTime);
    GPSTime = ini.GetFixed(RECHARGE, "GPS", GPSTime);
    NukeTime = ini.GetFixed(RECHARGE, "Nuke", NukeTime);
    return true;
  }
  return false;
}

/***********************************************************************************************
 * RulesClass::Heap_Maximums -- Fetch and process the heap override values. *
 *                                                                                             *
 *    This fetches the maximum heap sizes from the database specified. The heaps
 *will be       * initialized by this routine as indicated. *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database. *
 *                                                                                             *
 * OUTPUT:  bool; Was the maximum section found and processed? *
 *                                                                                             *
 * WARNINGS:   This process is catastrophic to any data currently existing in
 *the heaps        * modified. This should only be processed during the game
 *initialization stage.   *
 *                                                                                             *
 * HISTORY: * 08/08/1996 JLB : Created. *
 *=============================================================================================*/
bool RulesClass::Heap_Maximums(engine::file::IniFile& ini) {
  /*
  **	Heap maximum values.
  */
  static const char* const MAXIMUMS = "Maximums";
  if (ini.HasSection(MAXIMUMS)) {
    MaxPlayers = ini.GetInt(MAXIMUMS, "Players", MaxPlayers);
    AircraftMax = ini.GetInt(MAXIMUMS, "Aircraft", AircraftMax);
    AnimMax = ini.GetInt(MAXIMUMS, "Anim", AnimMax);
    BuildingMax = ini.GetInt(MAXIMUMS, "Building", BuildingMax);
    BulletMax = ini.GetInt(MAXIMUMS, "Bullet", BulletMax);
    FactoryMax = ini.GetInt(MAXIMUMS, "Factory", FactoryMax);
    InfantryMax = ini.GetInt(MAXIMUMS, "Infantry", InfantryMax);
    OverlayMax = ini.GetInt(MAXIMUMS, "Overlay", OverlayMax);
    SmudgeMax = ini.GetInt(MAXIMUMS, "Smudge", SmudgeMax);
    TeamMax = ini.GetInt(MAXIMUMS, "Team", TeamMax);
    TeamTypeMax = ini.GetInt(MAXIMUMS, "TeamType", TeamTypeMax);
    TemplateMax = ini.GetInt(MAXIMUMS, "Template", TemplateMax);
    TerrainMax = ini.GetInt(MAXIMUMS, "Terrain", TerrainMax);
    TriggerMax = ini.GetInt(MAXIMUMS, "Trigger", TriggerMax);
    UnitMax = ini.GetInt(MAXIMUMS, "Unit", UnitMax);
    VesselMax = ini.GetInt(MAXIMUMS, "Vessel", VesselMax);
    ProjectileMax = ini.GetInt(MAXIMUMS, "Projectile", ProjectileMax);
    WeaponMax = ini.GetInt(MAXIMUMS, "Weapon", WeaponMax);
    WarheadMax = ini.GetInt(MAXIMUMS, "Warhead", WarheadMax);
    TrigTypeMax = ini.GetInt(MAXIMUMS, "TrigType", TrigTypeMax);
  }

  /*
  **	Any heaps that use the maximums that were just loaded, must
  **	be initialized as necessary.
  */
  TheTypeHeaps().warhead().Reset(WarheadMax);
  new WarheadTypeClass("SA");
  new WarheadTypeClass("HE");
  new WarheadTypeClass("AP");
  new WarheadTypeClass("Fire");
  new WarheadTypeClass("HollowPoint");
  new WarheadTypeClass("Super");
  new WarheadTypeClass("Organic");
  new WarheadTypeClass("Nuke");
  new WarheadTypeClass("Mechanical");

  TheTypeHeaps().weapon().Reset(WeaponMax);
  new WeaponTypeClass("Colt45");
  new WeaponTypeClass("ZSU-23");
  new WeaponTypeClass("Vulcan");
  new WeaponTypeClass("Maverick");
  new WeaponTypeClass("Camera");
  new WeaponTypeClass("FireballLauncher");
  new WeaponTypeClass("Sniper");
  new WeaponTypeClass("ChainGun");
  new WeaponTypeClass("Pistol");
  new WeaponTypeClass("M1Carbine");
  new WeaponTypeClass("Dragon");
  new WeaponTypeClass("Hellfire");
  new WeaponTypeClass("Grenade");
  new WeaponTypeClass("75mm");
  new WeaponTypeClass("90mm");
  new WeaponTypeClass("105mm");
  new WeaponTypeClass("120mm");
  new WeaponTypeClass("TurretGun");
  new WeaponTypeClass("MammothTusk");
  new WeaponTypeClass("155mm");
  new WeaponTypeClass("M60mg");
  new WeaponTypeClass("Napalm");
  new WeaponTypeClass("TeslaZap");
  new WeaponTypeClass("Nike");
  new WeaponTypeClass("8Inch");
  new WeaponTypeClass("Stinger");
  new WeaponTypeClass("TorpTube");
  new WeaponTypeClass("2Inch");
  new WeaponTypeClass("DepthCharge");
  new WeaponTypeClass("ParaBomb");
  new WeaponTypeClass("DogJaw");
  new WeaponTypeClass("Heal");
  new WeaponTypeClass("SCUD");
  new WeaponTypeClass("Flamer");
  new WeaponTypeClass("RedEye");
  new WeaponTypeClass("Mandible");
  new WeaponTypeClass("PortaTesla");
  new WeaponTypeClass("GoodWrench");
  new WeaponTypeClass("SubSCUD");
  new WeaponTypeClass("TTankZap");
  new WeaponTypeClass("APTusk");
  new WeaponTypeClass("Democharge");
  new WeaponTypeClass("AirAssault");

  return true;
}

/***********************************************************************************************
 * RulesClass::AI -- Processes the AI control constants from the database. *
 *                                                                                             *
 *    This will examine the database specified and set the AI override values
 *accordingly.     *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database that holds the AI overrides.
 **
 *                                                                                             *
 * OUTPUT:  bool; Was the AI section found and processed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1996 JLB : Created. *
 *=============================================================================================*/
bool RulesClass::AI(engine::file::IniFile& ini) {
  static const char* const AI = "AI";
  if (ini.HasSection(AI)) {
    AttackInterval = ini.GetFixed(AI, "AttackInterval", AttackInterval);
    AttackDelay = ini.GetFixed(AI, "AttackDelay", AttackDelay);
    PatrolTime = ini.GetFixed(AI, "PatrolScan", PatrolTime);
    RepairThreshhold = ini.GetInt(AI, "CreditReserve", RepairThreshhold);
    PathDelay = ini.GetFixed(AI, "PathDelay", PathDelay);
    TiberiumShortScan = GetLepton(ini, AI, "OreNearScan", TiberiumShortScan);
    TiberiumLongScan = GetLepton(ini, AI, "OreFarScan", TiberiumLongScan);
    AutocreateTime = ini.GetFixed(AI, "AutocreateTime", AutocreateTime);
    InfantryReserve = ini.GetInt(AI, "InfantryReserve", InfantryReserve);
    InfantryBaseMult = ini.GetInt(AI, "InfantryBaseMult", InfantryBaseMult);
    PowerSurplus = ini.GetInt(AI, "PowerSurplus", PowerSurplus);
    BaseSizeAdd = ini.GetInt(AI, "BaseSizeAdd", BaseSizeAdd);
    RefineryRatio = ini.GetFixed(AI, "RefineryRatio", RefineryRatio);
    RefineryLimit = ini.GetInt(AI, "RefineryLimit", RefineryLimit);
    BarracksRatio = ini.GetFixed(AI, "BarracksRatio", BarracksRatio);
    BarracksLimit = ini.GetInt(AI, "BarracksLimit", BarracksLimit);
    WarRatio = ini.GetFixed(AI, "WarRatio", WarRatio);
    WarLimit = ini.GetInt(AI, "WarLimit", WarLimit);
    DefenseRatio = ini.GetFixed(AI, "DefenseRatio", DefenseRatio);
    DefenseLimit = ini.GetInt(AI, "DefenseLimit", DefenseLimit);
    AARatio = ini.GetFixed(AI, "AARatio", AARatio);
    AALimit = ini.GetInt(AI, "AALimit", AALimit);
    TeslaRatio = ini.GetFixed(AI, "TeslaRatio", TeslaRatio);
    TeslaLimit = ini.GetInt(AI, "TeslaLimit", TeslaLimit);
    HelipadRatio = ini.GetFixed(AI, "HelipadRatio", HelipadRatio);
    HelipadLimit = ini.GetInt(AI, "HelipadLimit", HelipadLimit);
    AirstripRatio = ini.GetFixed(AI, "AirstripRatio", AirstripRatio);
    AirstripLimit = ini.GetInt(AI, "AirstripLimit", AirstripLimit);
    IsCompEasyBonus = ini.GetBool(AI, "CompEasyBonus", IsCompEasyBonus);
    IsComputerParanoid = ini.GetBool(AI, "Paranoid", IsComputerParanoid);
    PowerEmergencyFraction =
        ini.GetFixed(AI, "PowerEmergency", PowerEmergencyFraction);
    return true;
  }
  return false;
}

/***********************************************************************************************
 * RulesClass::Powerups -- Process the powerup values from the database. *
 *                                                                                             *
 *    This will examine the database and initialize the powerup override values
 *accordingly.   *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database the the powerup values are to
 *be            * initialized from. *
 *                                                                                             *
 * OUTPUT:  bool; Was the powerup section found and processed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1996 JLB : Created. *
 *=============================================================================================*/
bool RulesClass::Powerups(engine::file::IniFile& ini) {
  static const char* const POWERUPS = "Powerups";
  if (ini.HasSection(POWERUPS)) {
    for (const CrateType crate : magic_enum::enum_values<CrateType>()) {
      const std::string value =
          ini.GetString(POWERUPS, CrateNames.at(crate), "0,NONE");
      if (!value.empty()) {
        /*
        **	Share odds.
        */
        base::Tokenizer tokens(value);
        std::string_view token = tokens.Next();
        if (!token.empty()) {
          crate_shares_.at(crate) = base::ParseIntegerOr<int>(token, 0);
        }

        /*
        **	Animation to use.
        */
        token = tokens.Next();
        if (!token.empty()) {
          crate_anims_.at(crate) =
              Anim_From_Name(absl::StripAsciiWhitespace(token));
        }

        /*
        **	Optional data number.
        */
        token = tokens.Next();
        if (!token.empty()) {
          if (token.contains('.') || token.contains('%')) {
            crate_data_.at(crate) =
                Fixed::FromString(absl::StripAsciiWhitespace(token)) * 256;
          } else {
            crate_data_.at(crate) = base::ParseIntegerOr<int>(token, 0);
          }
        }
      }
    }
    return true;
  }
  return false;
}

/***********************************************************************************************
 * RulesClass::Land_Types -- Inits the land type values. *
 *                                                                                             *
 *    This will set the land movement attributes from the database specified. *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the database that has the land value
 *overrides.              *
 *                                                                                             *
 * OUTPUT:  bool; Was the land type sections found and processed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1996 JLB : Created. *
 *=============================================================================================*/
bool RulesClass::Land_Types(engine::file::IniFile& ini) {
  /*
  **	Fetch the movement characteristic data for terrain types.
  */
  for (const LandType land : magic_enum::enum_values<LandType>()) {
    static constexpr base::EnumArray<LandType, const char*> _lands = {
        "Clear", "Road",  "Water", "Rock", "Wall",
        "Ore",   "Beach", "Rough", "River"};

    GroundType* gptr = &ground_.at(land);

    if (ini.HasSection(_lands.at(land))) {
      gptr->Cost.at(SPEED_FOOT) =
          ini.GetFixed(_lands.at(land), "Foot", Fixed(1)).SaturateBelow(1);
      gptr->Cost.at(SPEED_TRACK) =
          ini.GetFixed(_lands.at(land), "Track", Fixed(1)).SaturateBelow(1);
      gptr->Cost.at(SPEED_WHEEL) =
          ini.GetFixed(_lands.at(land), "Wheel", Fixed(1)).SaturateBelow(1);
      gptr->Cost.at(SPEED_WINGED) = Fixed(1).SaturateBelow(1);
      gptr->Cost.at(SPEED_FLOAT) =
          ini.GetFixed(_lands.at(land), "Float", Fixed(1)).SaturateBelow(1);
      gptr->Build = ini.GetBool(_lands.at(land), "Buildable", false);
    }
  }
  return true;
}

/***********************************************************************************************
 * RulesClass::Themes -- Fetches the theme control values from the INI database.
 **
 *                                                                                             *
 *    The musical theme availability is controlled by the scenario and the
 *player's house      * choice. These controls can be specified in the theme
 *control section of the INI          * database. *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database to process. *
 *                                                                                             *
 * OUTPUT:  bool; Was the theme section found and processed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/11/1996 JLB : Created. *
 *=============================================================================================*/
bool RulesClass::Themes(engine::file::IniFile& ini) {
  static const char* const THEMECONTROL = "ThemeControl";

  if (ini.HasSection(THEMECONTROL)) {
    for (const ThemeType theme : magic_enum::enum_values<ThemeType>()) {
      if (ini.HasEntry(THEMECONTROL, ThemeClass::Base_Name(theme))) {
        int scen = 1;
        uint32_t owners =
            kHouseFlagAllies | kHouseFlagSoviet | kHouseFlagOthers;

        const std::string value =
            ini.GetString(THEMECONTROL, ThemeClass::Base_Name(theme), "");
        base::Tokenizer tokens(value);
        std::string_view token = tokens.Next();
        if (!token.empty()) {
          scen = base::ParseIntegerOr<int>(token, 0);
        }

        token = tokens.Next();
        if (!token.empty()) {
          owners = Owner_From_Name(token);
        }

        ThemeClass::Set_Theme_Data(theme, scen, owners);
      }
    }
    return true;
  }
  return false;
}

/***********************************************************************************************
 * RulesClass::IQ -- Fetches the IQ control values from the INI database. *
 *                                                                                             *
 *    This will scan the database specified and retrieve the IQ control values
 *from it. These  * IQ control values are what gives the IQ rating meaning. It
 *fundimentally controls how    * the computer behaves. *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database to read the IQ controls from.
 **
 *                                                                                             *
 * OUTPUT:  bool; Was the IQ section found and processed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/11/1996 JLB : Created. *
 *=============================================================================================*/
bool RulesClass::IQ(engine::file::IniFile& ini) {
  static const char* const IQCONTROL = "IQ";
  if (ini.HasSection(IQCONTROL)) {
    MaxIQ = ini.GetInt(IQCONTROL, "MaxIQLevels", MaxIQ);
    IQSuperWeapons = ini.GetInt(IQCONTROL, "SuperWeapons", IQSuperWeapons);
    IQProduction = ini.GetInt(IQCONTROL, "Production", IQProduction);
    IQGuardArea = ini.GetInt(IQCONTROL, "GuardArea", IQGuardArea);
    IQRepairSell = ini.GetInt(IQCONTROL, "RepairSell", IQRepairSell);
    IQCrush = ini.GetInt(IQCONTROL, "AutoCrush", IQCrush);
    IQScatter = ini.GetInt(IQCONTROL, "Scatter", IQScatter);
    IQContentScan = ini.GetInt(IQCONTROL, "ContentScan", IQContentScan);
    IQAircraft = ini.GetInt(IQCONTROL, "Aircraft", IQAircraft);
    IQHarvester = ini.GetInt(IQCONTROL, "Harvester", IQHarvester);
    IQSellBack = ini.GetInt(IQCONTROL, "SellBack", IQSellBack);

    return true;
  }
  return false;
}

/***********************************************************************************************
 * RulesClass::Objects -- Fetch all the object characteristic values. *
 *                                                                                             *
 *    This will parse the specified INI database and fetch all the object
 *characteristic       * values specified therein. *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the ini database to scan. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/10/1996 JLB : Created. *
 *=============================================================================================*/
bool RulesClass::Objects(engine::file::IniFile& ini) {
  /*
  **	Fetch the game object values from the rules file.
  */
  for (int index = 0; index < TheTypeHeaps().warhead().Count(); index++) {
    TheTypeHeaps().warhead().ActiveAt(index)->Read_INI(ini);
  }

  for (int proj = 0; proj < TheTypeHeaps().bullet().Count(); proj++) {
    TheTypeHeaps().bullet().ActiveAt(proj)->Read_INI(ini);
  }

  for (int windex = 0; windex < TheTypeHeaps().weapon().Count(); windex++) {
    TheTypeHeaps().weapon().ActiveAt(windex)->Read_INI(ini);
  }

  for (int uindex = 0; uindex < TheTypeHeaps().unit().Count(); uindex++) {
    TheTypeHeaps().unit().ActiveAt(uindex)->Read_INI(ini);
  }

  for (int iindex = 0; iindex < TheTypeHeaps().infantry().Count(); iindex++) {
    TheTypeHeaps().infantry().ActiveAt(iindex)->Read_INI(ini);
  }

  for (int vindex = 0; vindex < TheTypeHeaps().vessel().Count(); vindex++) {
    TheTypeHeaps().vessel().ActiveAt(vindex)->Read_INI(ini);
  }

  for (int aindex = 0; aindex < TheTypeHeaps().aircraft().Count(); aindex++) {
    TheTypeHeaps().aircraft().ActiveAt(aindex)->Read_INI(ini);
  }

  for (int bindex = 0; bindex < TheTypeHeaps().building().Count(); bindex++) {
    TheTypeHeaps().building().ActiveAt(bindex)->Read_INI(ini);
  }

  /*
  **	Fetch the house attribute override values.
  */
  for (const HousesType house : magic_enum::enum_values<HousesType>()) {
    HouseTypeClass::As_Reference(house).Read_INI(ini);
  }

  /*
  **	Fetch the mission control values.
  */
  for (const MissionType mission : magic_enum::enum_values<MissionType>()) {
    MissionControlClass* miss = &mission_control_.at(mission);
    miss->Mission = mission;
    miss->Read_INI(ini);
  }

  return true;
}

/***********************************************************************************************
 * RulesClass::Difficulty -- Fetch the various difficulty group settings. *
 *                                                                                             *
 *    This routine is used to fetch the various group settings for the
 *difficulty levels.      *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database that has the difficulty
 *setting values.     *
 *                                                                                             *
 * OUTPUT:  bool; Was the difficulty section found and processed. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/10/1996 JLB : Created. *
 *=============================================================================================*/
bool RulesClass::Difficulty(engine::file::IniFile& ini) {
  Difficulty_Get(ini, Diff[static_cast<int>(DIFF_EASY)], "Easy");
  Difficulty_Get(ini, Diff[static_cast<int>(DIFF_NORMAL)], "Normal");
  Difficulty_Get(ini, Diff[static_cast<int>(DIFF_HARD)], "Difficult");
  return true;
}

}  // namespace ra
