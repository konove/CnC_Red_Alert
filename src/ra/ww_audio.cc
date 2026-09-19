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

// Red Alert's sound effects and EVA speech: the tables that name the .AUD file
// behind every VocType and VoxType, sound effects placed on the tactical map,
// and the EVA speech queue with its two cached speech buffers. The sounds are
// played by the global AudioMixer `Audio` (tech/audio_mixer.h).
//
// Originally AUDIO.CPP by Joe L. Bostic, started September 10, 1993.

#include "ra/ww_audio.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <span>
#include <string>

#include "absl/log/log.h"
#include "absl/strings/match.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/numeric.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/config.h"
#include "ra/coord.h"
#include "ra/defines.h"
#include "ra/display_constants.h"
#include "ra/externs.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/house.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/mapedit.h"
#include "tech/audio_mixer.h"
#include "tech/fixed.h"
#include "tech/game_file.h"
#include "tech/mix_archive.h"

// Controls what special effects may occur on the sound effect.
enum class ContextType {
  // One recording, NAME.AUD, for everybody.
  IN_NOVAR,
  // A unit response recorded in four variations for each side's accent:
  // NAME.V00-.V03 for the Allies and NAME.R00-.R03 for the Soviets.
  // Sound_Effect() picks one from the house and the variation number.
  IN_VAR
};
using enum ContextType;

struct SoundEffect {
  // Root file name; Sound_Effect() adds the extension. "x" marks a VocType
  // slot that has no sound: there is no X.AUD, so it plays nothing.
  const char* Name;
  // Playback priority at full volume. Sound_Effect() scales it by the volume,
  // so a faint distant sound loses a busy mixer to a loud nearby one.
  int Priority;
  ContextType Where;  // In what game context does this sample exist.
};
static base::EnumArray<VocType, SoundEffect> SoundEffectName = {{

    // Civilian voices (technicians too).
    {"GIRLOKAY", 20, IN_NOVAR},  // VOC_GIRL_OKAY
    {"GIRLYEAH", 20, IN_NOVAR},  // VOC_GIRL_YEAH
    {"GUYOKAY1", 20, IN_NOVAR},  // VOC_GUY_OKAY
    {"GUYYEAH1", 20, IN_NOVAR},  // VOC_GUY_YEAH

    {"MINELAY1", 5, IN_VAR},  // VOC_MINELAY1

    // Infantry and vehicle responses.
    {"ACKNO", 20, IN_VAR},       // VOC_ACKNOWL      "acknowledged"
    {"AFFIRM1", 20, IN_VAR},     // VOC_AFFIRM       "affirmative"
    {"AWAIT1", 20, IN_VAR},      // VOC_AWAIT        "awaiting orders"
    {"EAFFIRM1", 20, IN_NOVAR},  // VOC_ENG_AFFIRM   Engineer: "affirmative"
    {"EENGIN1", 20, IN_NOVAR},   // VOC_ENG_ENG      Engineer: "engineering"
    {"NOPROB", 20, IN_VAR},      // VOC_NO_PROB      "not a problem"
    {"READY", 20, IN_VAR},       // VOC_READY        "ready and waiting"
    {"REPORT1", 20, IN_VAR},     // VOC_REPORT       "reporting"
    {"RITAWAY", 20, IN_VAR},     // VOC_RIGHT_AWAY   "right away sir"
    {"ROGER", 20, IN_VAR},       // VOC_ROGER        "roger"
    {"UGOTIT", 20, IN_VAR},      // VOC_UGOTIT       "you got it"
    {"VEHIC1", 20, IN_VAR},      // VOC_VEHIC        "vehicle reporting"
    {"YESSIR1", 20, IN_VAR},     // VOC_YESSIR       "yes sir"

    // Infantry deaths.
    {"DEDMAN1", 10, IN_NOVAR},   // VOC_SCREAM1      short infantry scream
    {"DEDMAN2", 10, IN_NOVAR},   // VOC_SCREAM3      short infantry scream
    {"DEDMAN3", 10, IN_NOVAR},   // VOC_SCREAM4      short infantry scream
    {"DEDMAN4", 10, IN_NOVAR},   // VOC_SCREAM5      short infantry scream
    {"DEDMAN5", 10, IN_NOVAR},   // VOC_SCREAM6      short infantry scream
    {"DEDMAN6", 10, IN_NOVAR},   // VOC_SCREAM7      short infantry scream
    {"DEDMAN7", 10, IN_NOVAR},   // VOC_SCREAM10     short infantry scream
    {"DEDMAN8", 10, IN_NOVAR},   // VOC_SCREAM11     short infantry scream
    {"DEDMAN10", 10, IN_NOVAR},  // VOC_YELL1        long infantry scream

    // Weapons, machinery and interface sounds.
    {"CHRONO2", 5, IN_NOVAR},    // VOC_CHRONO       Chronosphere sound.
    {"CANNON1", 1, IN_NOVAR},    // VOC_CANNON1      Cannon sound (medium).
    {"CANNON2", 1, IN_NOVAR},    // VOC_CANNON2      Cannon sound (short).
    {"IRONCUR9", 10, IN_NOVAR},  // VOC_IRON1        Iron Curtain.
    {"EMOVOUT1", 20, IN_NOVAR},  // VOC_ENG_MOVEOUT  Engineer: "movin' out"
    {"SONPULSE", 10, IN_NOVAR},  // VOC_SONAR        Sonar pulse.
    {"SANDBAG2", 5, IN_NOVAR},   // VOC_SANDBAG      Sand bag crunch.
    {"MINEBLO1", 5, IN_NOVAR},   // VOC_MINEBLOW     Weird mine explosion.
    {"CHUTE1", 1, IN_NOVAR},     // VOC_CHUTE1       Wind swoosh sound.
    {"DOGY1", 5, IN_NOVAR},      // VOC_DOG_BARK     Dog bark.
    {"DOGW5", 10, IN_NOVAR},     // VOC_DOG_WHINE    Dog whine.
    {"DOGG5P", 10, IN_NOVAR},    // VOC_DOG_GROWL2   Strong dog growl.
    {"FIREBL3", 1, IN_NOVAR},    // VOC_FIRE_LAUNCH  Fireball launch sound.
    {"FIRETRT1", 1, IN_NOVAR},   // VOC_FIRE_EXPLODE Fireball explode sound.
    {"GRENADE1", 1, IN_NOVAR},   // VOC_GRENADE_TOSS Grenade toss.
    {"GUN11", 1, IN_NOVAR},      // VOC_GUN_5        5 round burst (slow).
    {"GUN13", 1, IN_NOVAR},      // VOC_GUN_7        7 round burst (fast).
    {"EYESSIR1", 20, IN_NOVAR},  // VOC_ENG_YES      Engineer: "yes sir"
    {"GUN27", 1, IN_NOVAR},      // VOC_GUN_RIFLE    Rifle shot.
    {"HEAL2", 1, IN_NOVAR},      // VOC_HEAL         Healing effect.
    {"HYDROD1", 1, IN_NOVAR},    // VOC_DOOR         Hydraulic door.
    {"INVUL2", 1, IN_NOVAR},     // VOC_INVULNERABLE Invulnerability effect.
    {"KABOOM1", 1, IN_NOVAR},    // VOC_KABOOM1      Long explosion (muffled).
    {"KABOOM12", 1, IN_NOVAR},   // VOC_KABOOM12     Very long, muffled.
    {"KABOOM15", 1, IN_NOVAR},   // VOC_KABOOM15     Very long, muffled.
    {"SPLASH9", 5, IN_NOVAR},    // VOC_SPLASH       Water splash.
    {"KABOOM22", 1, IN_NOVAR},   // VOC_KABOOM22     Long explosion (sharp).
    {"AACANON3", 1, IN_NOVAR},   // VOC_AACANON3     AA cannon.
    {"TANDETH1", 10, IN_NOVAR},  // VOC_TANYA_DIE    Tanya: scream.
    {"MGUNINF1", 1, IN_NOVAR},   // VOC_GUN_5F       5 round burst (fast).
    {"MISSILE1", 1, IN_NOVAR},   // VOC_MISSILE_1    Missile, high tech effect.
    {"MISSILE6", 1, IN_NOVAR},   // VOC_MISSILE_2    Long missile launch.
    {"MISSILE7", 1, IN_NOVAR},   // VOC_MISSILE_3    Short missile launch.
    {"x", 1, IN_NOVAR},          // VOC_x6           Unused.
    {"PILLBOX1", 1, IN_NOVAR},   // VOC_GUN_5R       5 round burst (rattles).
    {"RABEEP1", 1, IN_NOVAR},    // VOC_BEEP         Generic beep sound.
    {"RAMENU1", 1, IN_NOVAR},    // VOC_CLICK        Generic click sound.
    {"SILENCER", 1, IN_NOVAR},   // VOC_SILENCER     Silencer.
    {"TANK5", 1, IN_NOVAR},      // VOC_CANNON6      Long muffled cannon shot.
    {"TANK6", 1, IN_NOVAR},      // VOC_CANNON7      Sharp mechanical cannon.
    {"TORPEDO1", 1, IN_NOVAR},   // VOC_TORPEDO      Torpedo launch.
    {"TURRET1", 1, IN_NOVAR},    // VOC_CANNON8      Sharp cannon fire.
    {"TSLACHG2", 10, IN_NOVAR},  // VOC_TESLA_POWER_UP  Hum charge up.
    {"TESLA1", 10, IN_NOVAR},    // VOC_TESLA_ZAP    Tesla zap effect.
    {"SQUISHY2", 10, IN_NOVAR},  // VOC_SQUISH       Squish effect.
    {"SCOLDY1", 10, IN_NOVAR},   // VOC_SCOLD        Scold bleep.
    {"RADARON2", 20, IN_NOVAR},  // VOC_RADAR_ON     Powering up electronics.
    {"RADARDN1", 10, IN_NOVAR},  // VOC_RADAR_OFF    B movie power down.
    {"PLACBLDG", 10, IN_NOVAR},  // VOC_PLACE_BUILDING_DOWN  Building slam.
    {"KABOOM30", 1, IN_NOVAR},   // VOC_KABOOM30     Short explosion (HE).
    {"KABOOM25", 10, IN_NOVAR},  // VOC_KABOOM25     Short growling explosion.
    {"x", 10, IN_NOVAR},         // VOC_x7           Unused.
    {"DOGW7", 10, IN_NOVAR},     // VOC_DOG_HURT     Dog whine (loud).
    {"DOGW3PX", 10, IN_NOVAR},   // VOC_DOG_YES      Dog 'yes sir'.
    {"CRMBLE2", 10, IN_NOVAR},   // VOC_CRUMBLE      Building crumble.
    {"CASHUP1", 10, IN_NOVAR},   // VOC_MONEY_UP     Rising money tick.
    {"CASHDN1", 10, IN_NOVAR},   // VOC_MONEY_DOWN   Falling money tick.
    {"BUILD5", 10, IN_NOVAR},    // VOC_CONSTRUCTION Building construction.
    {"BLEEP9", 10, IN_NOVAR},    // VOC_GAME_CLOSED  Long bleep.
    {"BLEEP6", 10, IN_NOVAR},    // VOC_INCOMING_MESSAGE  Soft happy warble.
    {"BLEEP5", 10, IN_NOVAR},    // VOC_SYS_ERROR    Sharp soft warble.
    {"BLEEP17", 10, IN_NOVAR},   // VOC_OPTIONS_CHANGED  Mid range warble.
    {"BLEEP13", 10, IN_NOVAR},   // VOC_GAME_FORMING Long warble.
    {"BLEEP12", 10, IN_NOVAR},   // VOC_PLAYER_LEFT  Chirp sequence.
    {"BLEEP11", 10, IN_NOVAR},   // VOC_PLAYER_JOINED  Reverse chirp sequence.
    {"H2OBOMB2", 10, IN_NOVAR},  // VOC_DEPTH_CHARGE Distant explosion sound.
    {"CASHTURN", 10, IN_NOVAR},  // VOC_CASHTURN     Airbrake.

    // Tanya.
    {"TUFFGUY1", 20, IN_NOVAR},  // VOC_TANYA_CHEW   "Chew on this"
    {"ROKROLL1", 20, IN_NOVAR},  // VOC_TANYA_ROCK   "Let's rock"
    {"LAUGH1", 20, IN_NOVAR},    // VOC_TANYA_LAUGH  "ha ha ha"
    {"CMON1", 20, IN_NOVAR},     // VOC_TANYA_SHAKE  "Shake it baby"
    {"BOMBIT1", 20, IN_NOVAR},   // VOC_TANYA_CHING  "Cha Ching"
    {"GOTIT1", 20, IN_NOVAR},    // VOC_TANYA_GOT    "That's all you got"
    {"KEEPEM1", 20, IN_NOVAR},   // VOC_TANYA_KISS   "Kiss it bye bye"
    {"ONIT1", 20, IN_NOVAR},     // VOC_TANYA_THERE  "I'm there"
    {"LEFTY1", 20, IN_NOVAR},    // VOC_TANYA_GIVE   "Give it to me"
    {"YEAH1", 20, IN_NOVAR},     // VOC_TANYA_YEA    "Yea?"
    {"YES1", 20, IN_NOVAR},      // VOC_TANYA_YES    "Yes sir?"
    {"YO1", 20, IN_NOVAR},       // VOC_TANYA_WHATS  "What's up."

    {"WALLKIL2", 5, IN_NOVAR},  // VOC_WALLKILL2    Crushing wall sound.
    {"x", 10, IN_NOVAR},        // VOC_x8           Unused.
    {"GUN5", 5, IN_NOVAR},      // VOC_TRIPLE_SHOT  Three quick shots.
    {"SUBSHOW1", 5, IN_NOVAR},  // VOC_SUBSHOW      Submarine surfacing.
    {"EINAH1", 20, IN_NOVAR},   // VOC_E_AH         Einstein: "ah"
    {"EINOK1", 20, IN_NOVAR},   // VOC_E_OK         Einstein: "ok"
    {"EINYES1", 20, IN_NOVAR},  // VOC_E_YES        Einstein: "yes"
    {"MINE1", 10, IN_NOVAR},    // VOC_TRIP_MINE    Mine explosion sound.

    // Spy and medic.
    {"SCOMND1", 20, IN_NOVAR},   // VOC_SPY_COMMANDER  Spy: "commander?"
    {"SYESSIR1", 20, IN_NOVAR},  // VOC_SPY_YESSIR   Spy: "yes sir"
    {"SINDEED1", 20, IN_NOVAR},  // VOC_SPY_INDEED   Spy: "indeed"
    {"SONWAY1", 20, IN_NOVAR},   // VOC_SPY_ONWAY    Spy: "on my way"
    {"SKING1", 20, IN_NOVAR},    // VOC_SPY_KING     Spy: "for king and country"
    {"MRESPON1", 20, IN_NOVAR},  // VOC_MED_REPORTING  Medic: "reporting"
    {"MYESSIR1", 20, IN_NOVAR},  // VOC_MED_YESSIR   Medic: "yes sir"
    {"MAFFIRM1", 20, IN_NOVAR},  // VOC_MED_AFFIRM   Medic: "affirmative"
    {"MMOVOUT1", 20, IN_NOVAR},  // VOC_MED_MOVEOUT  Medic: "movin' out"
    {"BEEPSLCT", 10, IN_NOVAR},  // VOC_BEEP_SELECT  Map selection beep.

    // Thief, and the giant ants.
    {"SYEAH1", 20, IN_NOVAR},    // VOC_THIEF_YEA    Thief: "yea?"
    {"ANTDIE", 20, IN_NOVAR},    // VOC_ANTDIE
    {"ANTBITE", 20, IN_NOVAR},   // VOC_ANTBITE
    {"SMOUT1", 20, IN_NOVAR},    // VOC_THIEF_MOVEOUT  Thief: "movin' out"
    {"SOKAY1", 20, IN_NOVAR},    // VOC_THIEF_OKAY   Thief: "ok"
    {"x", 20, IN_NOVAR},         // VOC_x11          Unused.
    {"SWHAT1", 20, IN_NOVAR},    // VOC_THIEF_WHAT   Thief: "what"
    {"SAFFIRM1", 20, IN_NOVAR},  // VOC_THIEF_AFFIRM Thief: "affirmative"

    // Voices added for the expansion packs (VG, 2/24/97): Stavros and the
    // commandos first, then the Aftermath units.
    {"STAVCMDR", 20, IN_NOVAR},  // VOC_STAVCMDR
    {"STAVCRSE", 20, IN_NOVAR},  // VOC_STAVCRSE
    {"STAVYES", 20, IN_NOVAR},   // VOC_STAVYES
    {"STAVMOV", 20, IN_NOVAR},   // VOC_STAVMOV
    {"BUZZY1", 20, IN_NOVAR},    // VOC_BUZZY1
    {"RAMBO1", 20, IN_NOVAR},    // VOC_RAMBO1
    {"RAMBO2", 20, IN_NOVAR},    // VOC_RAMBO2
    {"RAMBO3", 20, IN_NOVAR},    // VOC_RAMBO3
    {"MYES1", 20, IN_NOVAR},     // VOC_MECHYES1     Mechanic: "Yes sir!"
    {"MHOWDY1", 20, IN_NOVAR},   // VOC_MECHHOWDY1   Mechanic: "Howdy!"
    {"MRISE1", 20, IN_NOVAR},    // VOC_MECHRISE1    Mechanic: "Rise 'n shine!"
    {"MHUH1", 20, IN_NOVAR},     // VOC_MECHHUH1     Mechanic: "Huh?"
    {"MHEAR1", 20, IN_NOVAR},    // VOC_MECHHEAR1    Mechanic: "I Hear Ya!"
    {"MLAFF1", 20, IN_NOVAR},    // VOC_MECHLAFF1    Mechanic: guffaw
    {"MBOSS1", 20, IN_NOVAR},  // VOC_MECHBOSS1    Mechanic: "Sure Thing, Boss!"
    {"MYEEHAW1", 20, IN_NOVAR},  // VOC_MECHYEEHAW1  Mechanic: "Yee Haw!"
    {"MHOTDIG1", 20,
     IN_NOVAR},  // VOC_MECHHOTDIG1  Mechanic: "Hot Diggity Dog!"
    {"MWRENCH1", 20,
     IN_NOVAR},  // VOC_MECHWRENCH1  Mechanic: "I'll get my wrench."

    {"JBURN1", 20, IN_NOVAR},  // VOC_STBURN1   Shock Trooper: "Burn baby burn!"
    {"JCHRGE1", 20, IN_NOVAR},  // VOC_STCHRGE1  Shock Trooper: "Fully charged!"
    {"JCRISP1", 20, IN_NOVAR},  // VOC_STCRISP1  Shock Trooper: "Extra Crispy!"
    {"JDANCE1", 20, IN_NOVAR},  // VOC_STDANCE1  Shock Trooper: "Let's Dance!"
    {"JJUICE1", 20, IN_NOVAR},  // VOC_STJUICE1  Shock Trooper: "Got juice?"
    {"JJUMP1", 20, IN_NOVAR},   // VOC_STJUMP1   Shock Trooper: "Need a jump?"
    {"JLIGHT1", 20, IN_NOVAR},  // VOC_STLIGHT1  Shock Trooper: "Lights out!"
    {"JPOWER1", 20, IN_NOVAR},  // VOC_STPOWER1  Shock Trooper: "Power on!"
    {"JSHOCK1", 20, IN_NOVAR},  // VOC_STSHOCK1  Shock Trooper: "Shocking!"
    {"JYES1", 20, IN_NOVAR},    // VOC_STYES1    Shock Trooper: "Yesssss!"

    {"CHROTNK1", 20, IN_NOVAR},  // VOC_CHRONOTANK1  Chrono tank teleport.
    {"FIXIT1", 20, IN_NOVAR},    // VOC_MECH_FIXIT1  Mechanic fixes something.
    {"MADCHRG2", 20, IN_NOVAR},  // VOC_MAD_CHARGE   M.A.D. tank charges up.
    {"MADEXPLO", 20, IN_NOVAR},  // VOC_MAD_EXPLODE  M.A.D. tank explodes.
    {"SHKTROP1", 20, IN_NOVAR},  // VOC_SHOCK_TROOP1 Shock Trooper fires.
}};

VocType Voc_From_Name(const char* name) {
  if (name == nullptr) {
    return VOC_NONE;
  }

  for (const VocType voc : magic_enum::enum_values<VocType>()) {
    if (absl::EqualsIgnoreCase(name, SoundEffectName.at(voc).Name)) {
      return voc;
    }
  }

  return VOC_NONE;
}

const char* Voc_Name(VocType voc) {
  if (voc == VOC_NONE) {
    return "none";
  }
  return SoundEffectName.at(voc).Name;
}

void Sound_Effect(VocType voc, COORDINATE coord, int variation,
                  HousesType house) {
  CELL cell_pos = 0;

  if (Debug_Quiet || Options.Volume == 0 || voc == VOC_NONE || !SoundOn ||
      !Audio.is_open()) {
    return;
  }
  if (coord) {
    cell_pos = Coord_Cell(coord);
  }

  // A sound on screen, or with no location, plays at full volume, centred.
  // Off screen it fades linearly with the distance in cells, reaching silence
  // 192 cells away (1.5 map widths). Sub_Saturate() keeps a sliver of 1/256
  // even there.
  fixed volume(1);
  int pan_value = 0;
  if (coord && !Map.In_View(cell_pos)) {
    // Measured from the centre of the view: TacticalCoord is its upper-left
    // corner, which would make sounds below and right of the screen quieter
    // than those as far above and left.
    const COORDINATE view_center =
        Coord_Add(Map.TacticalCoord,
                  XY_Coord(static_cast<LEPTON>(Map.TacLeptonWidth / 2),
                           static_cast<LEPTON>(Map.TacLeptonHeight / 2)));
    const int distance = Distance(coord, view_center) / CELL_LEPTON_W;
    fixed dfixed = fixed(distance, 128 + 64);
    dfixed.Sub_Saturate(1);
    volume = fixed(1) - dfixed;

    // Pan by the column offset from the centre of the view, and only once the
    // sound lies left or right of the screen: 0x8000 per quarter map width,
    // clamped to the int16_t range. The mixer mixes in mono and ignores it.
    pan_value = Cell_X(cell_pos);
    pan_value -= Coord_XCell(Map.TacticalCoord) +
                 (Lepton_To_Cell(Map.TacLeptonWidth) / 2);
    if (std::abs(pan_value) > Lepton_To_Cell(Map.TacLeptonWidth / 2)) {
      pan_value *= 0x8000;
      pan_value /= MAP_CELL_W / 4;
      pan_value = Bound(pan_value, -0x7FFF, 0x7FFF);
    } else {
      pan_value = 0;
    }
  }

  Sound_Effect(voc, volume, variation, static_cast<int16_t>(pan_value), house);
}

int Sound_Effect(VocType voc, fixed volume, int variation, int16_t pan_value,
                 HousesType house) {
  // A VocType cast from scenario or INI data can be out of range; indexing
  // the table with it would fail the at() check.
  if (voc != VOC_NONE && !magic_enum::enum_contains(voc)) {
    DLOG(WARNING) << "Sound_Effect: invalid voc=" << static_cast<int>(voc)
                  << ", valid range is [0, "
                  << static_cast<int>(magic_enum::enum_count<VocType>()) - 1
                  << "]";
    return -1;
  }
  if (Debug_Quiet || Options.Volume == 0 || voc == VOC_NONE || !SoundOn ||
      !Audio.is_open()) {
    return -1;
  }

  // Alter the volume according to the game volume setting.
  volume = volume * Options.Volume;

  // Pick the file: NAME.AUD, or for a unit response the variation that fits
  // the house's accent and the kind of unit.
  const char* ext = ".AUD";
  if (SoundEffectName.at(voc).Where == IN_VAR) {
    // If no house is forced, use the one the player's house acts like.
    // Responses only come from units the player selects or orders, so there
    // is always a player house by then.
    if (house == HOUSE_NONE) {
      house = PlayerPtr->ActLike;
    }

    // Allied houses get the .V?? recordings, all others the Soviet .R?? ones.
    // Vehicles and aircraft pass a negative variation, -(ID + 1), and get
    // recordings 00 and 02; infantry pass ID + 1 and get 01 and 03. The parity
    // of the ID alternates between the two voices so a group of units does not
    // all answer in the same voice.
    if ((base::Bit<uint32_t>(house) & kHouseFlagAllies) != 0) {
      if (variation < 0) {
        if (std::abs(variation) % 2) {
          ext = ".V00";
        } else {
          ext = ".V02";
        }
      } else {
        if (variation % 2) {
          ext = ".V01";
        } else {
          ext = ".V03";
        }
      }
    } else {
      if (variation < 0) {
        if (std::abs(variation) % 2) {
          ext = ".R00";
        } else {
          ext = ".R02";
        }
      } else {
        if (variation % 2) {
          ext = ".R01";
        } else {
          ext = ".R03";
        }
      }
    }
  }
  const auto name = std::filesystem::path(SoundEffectName.at(voc).Name)
                        .replace_extension(ext)
                        .string();
  const auto ptr = MixArchive::RetrieveData(name);

  // The sample is played straight out of the mixfile cache, which keeps it
  // alive for as long as the mixer needs it. An empty span means the file is
  // in no loaded mixfile, as for the "x" placeholders.
  if (!ptr.empty()) {
    // Clamp to 255/256 so that volume * 256 fits the mixer's 0..255 range. A
    // quieter sound also plays at a lower priority.
    volume.Sub_Saturate(1);
    return Audio.Play(ptr, SoundEffectName.at(voc).Priority * volume,
                      volume * 256, pan_value);
  }
  return -1;
}

// The root file names of the EVA speech, one per VoxType. "none" marks a slot
// with no recording: there is no NONE.AUD, so speaking it says nothing.
static constexpr base::EnumArray<VoxType, const char*> Speech = {
    "MISNWON1",  // VOX_ACCOMPLISHED  mission accomplished
    "MISNLST1",  // VOX_FAIL  your mission has failed
    "PROGRES1",  // VOX_NO_FACTORY  unable to comply, building in progress
    "CONSCMP1",  // VOX_CONSTRUCTION  construction complete
    "UNITRDY1",  // VOX_UNIT_READY  unit ready
    "NEWOPT1",   // VOX_NEW_CONSTRUCT  new construction options
    "NODEPLY1",  // VOX_DEPLOY  cannot deploy here
    "STRCKIL1",  // VOX_STRUCTURE_DESTROYED  structure destroyed
    "NOPOWR1",   // VOX_INSUFFICIENT_POWER  insufficient power
    "NOFUNDS1",  // VOX_NO_CASH  insufficient funds
    "BCT1",      // VOX_CONTROL_EXIT  battle control terminated
    "REINFOR1",  // VOX_REINFORCEMENTS  reinforcements have arrived
    "CANCLD1",   // VOX_CANCELED  canceled
    "ABLDGIN1",  // VOX_BUILDING  building
    "LOPOWER1",  // VOX_LOW_POWER  low power
    "NOFUNDS1",  // VOX_NEED_MO_MONEY  insufficient funds
    "BASEATK1",  // VOX_BASE_UNDER_ATTACK  our base is under attack
    "NOBUILD1",  // VOX_UNABLE_TO_BUILD  unable to build more
    "PRIBLDG1",  // VOX_PRIMARY_SELECTED  primary building selected
    // VOX_MADTANK_DEPLOYED: M.A.D. Tank Deployed, English speech set only.
    config::kIsEnglish ? "TANK01" : "none",
    "none",      // VOX_none4
    "UNITLST1",  // VOX_UNIT_LOST  unit lost
    "SLCTTGT1",  // VOX_SELECT_TARGET  select target
    "ENMYAPP1",  // VOX_PREPARE  enemy approaching
    "SILOND1",   // VOX_NEED_MO_CAPACITY  silos needed
    "ONHOLD1",   // VOX_SUSPENDED  on hold
    "REPAIR1",   // VOX_REPAIRING  repairing
    "none",      // VOX_none5
    "none",      // VOX_none6
    "AUNITL1",   // VOX_AIRCRAFT_LOST  airborne unit lost
    "none",      // VOX_none7
    "AAPPRO1",   // VOX_ALLIED_FORCES_APPROACHING  allied forces approaching
    "AARRIVE1",  // VOX_ALLIED_APPROACHING  allied reinforcements have arrived
    "none",      // VOX_none8
    "none",      // VOX_none9
    "BLDGINF1",  // VOX_BUILDING_INFILTRATED  building infiltrated
    "CHROCHR1",  // VOX_CHRONO_CHARGING  chronosphere charging
    "CHRORDY1",  // VOX_CHRONO_READY  chronosphere ready
    "CHROYES1",  // VOX_CHRONO_TEST  chronosphere test successful
    "CMDCNTR1",  // VOX_HQ_UNDER_ATTACK  command center under attack
    "CNTLDED1",  // VOX_CENTER_DEACTIVATED  control center deactivated
    "CONVYAP1",  // VOX_CONVOY_APPROACHING  convoy approaching
    "CONVLST1",  // VOX_CONVOY_UNIT_LOST  convoy unit lost
    "XPLOPLC1",  // VOX_EXPLOSIVE_PLACED  explosive charge placed
    "CREDIT1",   // VOX_MONEY_STOLEN  credits stolen
    "NAVYLST1",  // VOX_SHIP_LOST  naval unit lost
    "SATLNCH1",  // VOX_SATALITE_LAUNCHED  satellite launched
    "PULSE1",    // VOX_SONAR_AVAILABLE  sonar pulse available
    "none",      // VOX_none10
    "SOVFAPP1",  // VOX_SOVIET_FORCES_APPROACHING  soviet forces approaching
    "SOVREIN1",  // VOX_SOVIET_REINFORCEMENTS  soviet reinforcements arrived
    "TRAIN1",    // VOX_TRAINING  training
    "AREADY1",   // VOX_ABOMB_READY
    "ALAUNCH1",  // VOX_ABOMB_LAUNCH
    "AARRIVN1",  // VOX_ALLIES_N
    "AARRIVS1",  // VOX_ALLIES_S
    "AARIVE1",   // VOX_ALLIES_E
    "AARRIVW1",  // VOX_ALLIES_W
    "1OBJMET1",  // VOX_OBJECTIVE1
    "2OBJMET1",  // VOX_OBJECTIVE2
    "3OBJMET1",  // VOX_OBJECTIVE3
    "IRONCHG1",  // VOX_IRON_CHARGING
    "IRONRDY1",  // VOX_IRON_READY
    "KOSYRES1",  // VOX_RESCUED
    "OBJNMET1",  // VOX_OBJECTIVE_NOT
    "FLAREN1",   // VOX_SIGNAL_N
    "FLARES1",   // VOX_SIGNAL_S
    "FLAREE1",   // VOX_SIGNAL_E
    "FLAREW1",   // VOX_SIGNAL_W
    "SPYPLN1",   // VOX_SPY_PLANE
    "TANYAF1",   // VOX_FREED
    "ARMORUP1",  // VOX_UPGRADE_ARMOR
    "FIREPO1",   // VOX_UPGRADE_FIREPOWER
    "UNITSPD1",  // VOX_UPGRADE_SPEED
    "MTIMEIN1",  // VOX_MISSION_TIMER
    "UNITFUL1",  // VOX_UNIT_FULL
    "UNITREP1",  // VOX_UNIT_REPAIRED
    "40MINR",    // VOX_TIME_40
    "30MINR",    // VOX_TIME_30
    "20MINR",    // VOX_TIME_20
    "10MINR",    // VOX_TIME_10
    "5MINR",     // VOX_TIME_5
    "4MINR",     // VOX_TIME_4
    "3MINR",     // VOX_TIME_3
    "2MINR",     // VOX_TIME_2
    "1MINR",     // VOX_TIME_1
    "TIMERNO1",  // VOX_TIME_STOP
    "UNITSLD1",  // VOX_UNIT_SOLD
    "TIMERGO1",  // VOX_TIMER_STARTED
    "TARGRES1",  // VOX_TARGET_RESCUED
    "TARGFRE1",  // VOX_TARGET_FREED
    "TANYAR1",   // VOX_TANYA_RESCUED
    "STRUSLD1",  // VOX_STRUCTURE_SOLD
    "SOVFORC1",  // VOX_SOVIET_FORCES_FALLEN
    "SOVEMP1",   // VOX_SOVIET_SELECTED
    "SOVEFAL1",  // VOX_SOVIET_EMPIRE_FALLEN
    "OPTERM1",   // VOX_OPERATION_TERMINATED
    "OBJRCH1",   // VOX_OBJECTIVE_REACHED
    "OBJNRCH1",  // VOX_OBJECTIVE_NOT_REACHED
    "OBJMET1",   // VOX_OBJECTIVE_MET
    "MERCR1",    // VOX_MERCENARY_RESCUED
    "MERCF1",    // VOX_MERCENARY_FREED
    "KOSYFRE1",  // VOX_KOSOYGEN_FREED
    "FLARE1",    // VOX_FLARE_DETECTED
    "COMNDOR1",  // VOX_COMMANDO_RESCUED
    "COMNDOF1",  // VOX_COMMANDO_FREED
    "BLDGPRG1",  // VOX_BUILDING_IN_PROGRESS
    "ATPREP1",   // VOX_ATOM_PREPPING
    "ASELECT1",  // VOX_ALLIED_SELECTED
    "APREP1",    // VOX_ABOMB_PREPPING
    "ATLNCH1",   // VOX_ATOM_LAUNCHED
    "AFALLEN1",  // VOX_ALLIED_FORCES_FALLEN
    "AAVAIL1",   // VOX_ABOMB_AVAILABLE
    "AARRIVE1",  // VOX_ALLIED_REINFORCEMENTS
    "SAVE1",     // VOX_SAVE1  mission saved
    "LOAD1"      // VOX_LOAD1  mission loaded
};

// The voice EVA is saying now, or VOX_NONE once Speak_AI() finds the speech
// buffer silent. Speak() drops a request for this voice so that the same
// announcement does not queue up behind itself.
static VoxType CurrentVoice = VOX_NONE;

const char* Speech_Name(VoxType speech) {
  if (speech == VOX_NONE) {
    return "none";
  }
  return Speech.at(speech);
}

void Speak(VoxType voice) {
  // Only one voice waits in the queue: a request made while another is
  // pending is dropped, not queued behind it.
  if (!Debug_Quiet && Options.Volume != 0 && Audio.is_open() &&
      voice != VOX_NONE && voice != SpeakQueue && voice != CurrentVoice &&
      SpeakQueue == VOX_NONE) {
    SpeakQueue = voice;
    // Start it now if EVA is silent, rather than a tick later.
    Speak_AI();
  }
}

void Speak_AI() {
  // The speech buffer EVA played last, and so the one to watch for the end of
  // the voice. The other buffer is the older one, reused for the next load.
  static int _index = 0;
  if (Debug_Quiet || !Audio.is_open()) {
    return;
  }

  if (!Audio.IsPlaying(base::At(SpeechBuffer, _index).data())) {
    CurrentVoice = VOX_NONE;
    if (SpeakQueue != VOX_NONE) {
      // Try to find a previously loaded copy of the EVA speech in one of the
      // speech buffers.
      std::span<const std::byte> speech;
      for (size_t index = 0; index < std::size(SpeechRecord); index++) {
        if (base::At(SpeechRecord, index) == SpeakQueue) {
          // _index tracks the buffer being played, so move it to the cached
          // one -- the poll at the top of this routine watches that buffer to
          // decide when the voice has finished.
          _index = static_cast<int>(index);
          speech = base::At(SpeechBuffer, index);
          break;
        }
      }

      // If a previous copy could not be located, then load the requested
      // voice into the oldest buffer available. A voice longer than the
      // buffer (kSpeechBufferSize) is cut short. SpeechRecord is only updated
      // on success, so a failed load leaves the old voice cached.
      if (speech.empty()) {
        _index = static_cast<int>((_index + 1) % std::ssize(SpeechRecord));

        const auto name = std::filesystem::path(Speech.at(SpeakQueue))
                              .replace_extension(".AUD")
                              .string();

        GameFile file(name);
        if (file.IsAvailable() && file.Read(base::At(SpeechBuffer, _index))) {
          speech = base::At(SpeechBuffer, _index);
          base::At(SpeechRecord, _index) = SpeakQueue;
        }
      }

      // Play the speech, whether it was cached or just loaded. At priority 254
      // it cuts off any sound effect (their priorities are at most 20) to get
      // a channel.
      if (!speech.empty()) {
        Audio.Play(speech, 254, Options.Volume * 256);
        CurrentVoice = SpeakQueue;
      }

      // Cleared even if the voice could not be loaded, so that a missing
      // file is not retried every tick.
      SpeakQueue = VOX_NONE;
    }
  }
}

void Stop_Speaking() {
  SpeakQueue = VOX_NONE;
  // Cleared here, not left for the next Speak_AI(), so that Speak() does not
  // drop the voice just stopped as one still being said.
  CurrentVoice = VOX_NONE;
  for (auto& index : SpeechBuffer) {
    Audio.Stop(index.data());
  }
}

bool Is_Speaking() {
  // Starts any queued voice first, so a caller waiting in a loop for EVA to
  // finish also keeps the queue moving.
  Speak_AI();
  return !Debug_Quiet && Audio.is_open() &&
         (SpeakQueue != VOX_NONE ||
          std::ranges::any_of(SpeechBuffer, [](const auto& buffer) {
            return Audio.IsPlaying(buffer.data());
          }));
}
