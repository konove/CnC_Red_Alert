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

// File: Installer registry flags and CD/DVD availability checks.

#include "ra/installation.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <span>
#include <string>

#include "absl/strings/str_format.h"
#include "base/numeric.h"
#include "port/platform.h"
#include "port/safe_string.h"
#include "port/win32/win32_registry.h"
#include "ra/config.h"
#include "ra/externs.h"
#include "ra/globals.h"
#include "ra/inline.h"
#include "ra/interpal.h"
#include "ra/jshell.h"
#include "ra/mission_id.h"
#include "ra/mplayer.h"
#include "ra/msgbox.h"
#include "ra/palette.h"
#include "ra/text_ids.h"
#include "ra/theme.h"
#include "sdllib/drawbuff.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/playcd.h"
#include "sdllib/ww_mouse.h"
#include "tech/game_file.h"
#include "tech/mix_archive.h"
#include "tech/rgb.h"
#include "tech/search_paths.h"

const char* Game_Registry_Key() {
  if constexpr (config::kBuildLanguage == config::BuildLanguage::French) {
    return "SOFTWARE\\Westwood\\Alerte Rouge version Windows 95";
  } else if constexpr (config::kBuildLanguage ==
                       config::BuildLanguage::German) {
    return "SOFTWARE\\Westwood\\Alarmstufe Rot Windows 95 Edition";
  } else {
    return "SOFTWARE\\Westwood\\Red Alert Windows 95 Edition";
  }
}

bool ReadInstallerFlag(const char* value_name) {
  return port::ReadRegistryDword(HKEY_LOCAL_MACHINE, Game_Registry_Key(),
                                 value_name)
             .value_or(0) != 0;
}

bool Is_Counterstrike_Installed() {
  if constexpr (port::kIsWindows) {
    static const bool installed = ReadInstallerFlag("CStrikeInstalled");
    return installed;
  } else {
    return true;
  }
}

bool Is_Aftermath_Installed() {
  if constexpr (port::kIsWindows) {
    static const bool installed = ReadInstallerFlag("AftermathInstalled");
    return installed;
  } else {
    return true;
  }
}

// There are no removable discs to check: the data is installed on disk, so
// this reports the DVD unconditionally. The original scanned the drive's
// volume label against kCdNames.
int Get_CD_Index(int /*cd_drive*/, int /*timeout*/) {
  return 5;  // we uh, magically have the DVD
}

// Disc identifiers, matching the order of kCdNames below. kCdSoviet and
// kCdAllied are unreferenced by name but fix the numbering the later values
// depend on, and are the values Get_CD_Index() returns for those discs.
namespace {
constexpr int kCdLocal = -2;
constexpr int kCdAny = -1;
[[maybe_unused]] constexpr int kCdSoviet = 0;
[[maybe_unused]] constexpr int kCdAllied = 1;
constexpr int kCdCounterstrike = 2;
constexpr int kCdAftermath = 3;
constexpr int kCdCsOrAm = 4;
constexpr int kCdDvd = 5;

// Index of the DVD's name in kCdNames. The table has no entry for the
// kCdCsOrAm request, so the names stop lining up with the disc ids there.
constexpr int kDvdName = 4;
}  // namespace

bool Force_CD_Available(int cd_desired)  // ajw
{
  static int _last = -1;
  static std::span<const std::byte> font;
  // Disc names as printed on the localized releases, in the language this
  // build was compiled for.
  static constexpr std::array<const char*, 5> kCdNames = [] {
    if constexpr (config::kBuildLanguage == config::BuildLanguage::French) {
      return std::array{
          "ALERTE ROUGE CD1",   "ALERTE ROUGE CD2", "CD Missions Taiga",
          "CD Missions M.A.D.", "ALERTE ROUGE DVD",
      };
    } else if constexpr (config::kBuildLanguage ==
                         config::BuildLanguage::German) {
      return std::array{
          "ALARMSTUFE ROT CD1",       "ALARMSTUFE ROT CD2",
          "CD Gegenangriff einlegen", "CD TRANS einlegen",
          "ALARMSTUFE ROT DVD",
      };
    } else {
      return std::array{
          "RED ALERT DISK 1", "RED ALERT DISK 2", "CounterStrike CD",
          "Aftermath CD",     "RED ALERT DVD",
      };
    }
  }();

  int new_cd_drive = 0;

  // If the required CD is set to -2 then it means that the file is present
  // on the local hard drive and we shouldn't have to worry about it.
  if (cd_desired == kCdLocal) {
    return true;
  }

  // Find out if the CD in the current drive is the one we are looking for
  const int current_drive = SearchPaths::current_cd_drive();
  int cd_current = Get_CD_Index(current_drive, 1 * 60);

  if (Using_DVD()) {
    // The DVD release carries every disc's content, so any disc request is
    // satisfied by it.
    cd_desired = kCdDvd;
  }

  if (cd_current >= 0) {
    // If the current cd is CS or AM then change request to whatever
    // is present.
    if ((cd_desired == kCdCsOrAm) &&
        (cd_current == kCdCounterstrike || cd_current == kCdAftermath)) {
      cd_desired = cd_current;
    }

    // If the current CD is requested or any CD will work
    if (cd_desired == cd_current || cd_desired == kCdAny) {
      // The required CD is still in the CD drive we used last time, so the
      // content is already reachable.
      return true;
    }
  }

  // Flag that we will have to restart the theme
  Theme.Stop();

  // Check the last drive
  if (!new_cd_drive) {
    // Check the last CD drive we used if it's different from the current one
    const int last_drive = SearchPaths::last_cd_drive();

    // Make sure the last drive is valid and it isn't the current drive
    // Skipped when it is the current drive, which the search above already
    // covered.
    if (last_drive && last_drive != SearchPaths::current_cd_drive()) {
      // Find out if there is a C&C cd in the last drive and if so is it the one
      // we are looking for
      // Give it a nice big timeout so the CD changer has time to swap the discs
      cd_current = Get_CD_Index(last_drive, 10 * 60);

      if (cd_current >= 0) {
        // If the cd is CS or AM then change request to whatever
        // is present.
        if ((cd_desired == kCdCsOrAm) &&
            (cd_current == kCdCounterstrike || cd_current == kCdAftermath)) {
          cd_desired = cd_current;
        }

        // If the cd is present or any cd will work
        if (cd_desired == cd_current || cd_desired == kCdAny) {
          // The required CD is in the CD drive we used last time
          new_cd_drive = last_drive;
        }
      }
    }
  }

  // Lordy.  No sign of that blimming CD anywhere. Search all the CD drives
  // then if we still can't find it prompt the user to insert it.
  if (!new_cd_drive) {
    // Small timeout for the first pass through the drives
    int drive_search_timeout = 2 * 60;

    for (;;) {
      char buffer[128];
      // Search all present CD drives for the required disc.
      for (int i = 0; i < CDList.Get_Number_Of_Drives(); i++) {
        const int cd_drive = CDList.Get_Next_CD_Drive();
        cd_current = Get_CD_Index(cd_drive, drive_search_timeout);

        if (cd_current >= 0) {
          // We found a C&C cd - lets see if it was the one we were looking for
          // Require CS or AM
          // If the cd is CS or AM then change request to whatever
          // is present.
          if ((cd_desired == kCdCsOrAm) &&
              (cd_current == kCdCounterstrike || cd_current == kCdAftermath)) {
            cd_desired = cd_current;
          }

          if (cd_desired == cd_current || cd_desired == kCdAny) {
            // Woohoo! The disk was in a different cd drive. Refresh the search
            // path list and return.
            new_cd_drive = cd_drive;
            break;
          }
        }
      }

      // A new disc has become available so break
      if (new_cd_drive) {
        break;
      }

      // Increase the timeout for subsequent drive searches.
      drive_search_timeout = 5 * 60;

      // Prompt to insert the CD into the drive.
      // V.Grippi
      if (cd_desired == kCdCsOrAm) {
        cd_desired = kCdAftermath;
      }

      // The wording is fixed by the language this build was compiled for; only
      // the disc name varies.
      const auto insert_prompt = [&buffer](const char* disc_name) {
        if constexpr (config::kBuildLanguage == config::BuildLanguage::French) {
          absl::SNPrintF(buffer, sizeof(buffer), "Insèrez le %s", disc_name);
        } else if constexpr (config::kBuildLanguage ==
                             config::BuildLanguage::German) {
          absl::SNPrintF(buffer, sizeof(buffer), "Bitte %s", disc_name);
        } else {
          absl::SNPrintF(buffer, sizeof(buffer), "Please insert the %s",
                         disc_name);
        }
      };

      if (cd_desired == kCdDvd) {
        insert_prompt(kCdNames.at(kDvdName));
      } else if (cd_desired == kCdCounterstrike || cd_desired == kCdAftermath) {
        insert_prompt(kCdNames.at(base::ToSize(cd_desired)));
      } else {
        // These prompts come from the localized string table, so verify the
        // translation still takes a %d followed by a %s before using it.
        const int text =
            cd_desired == kCdAny ? TXT_CD_DIALOG_1 : TXT_CD_DIALOG_2;  // 0 or 1
        const auto format =
            absl::ParsedFormat<'d', 's'>::New(Text_String(text));
        if (format != nullptr) {
          port::SafeCopy(buffer,
                         absl::StrFormat(*format, cd_desired + 1,
                                         kCdNames.at(base::ToSize(cd_desired)))
                             .c_str());
        }
      }

      GraphicViewPortClass* old_page = Set_Logic_Page(SeenBuff);
      Theme.Stop();
      int hidden = Get_Mouse_State();
      font = FontPtr;

      // Only set the palette if necessary.
      if (PaletteClass::CurrentPalette.at(1).Red_Component() +
              PaletteClass::CurrentPalette.at(1).Blue_Component() +
              PaletteClass::CurrentPalette.at(1).Green_Component() ==
          0) {
        GamePalette.Set();
      }

      Keyboard->Clear();

      while (Get_Mouse_State()) {
        Show_Mouse();
      }

      if (WWMessageBox().Process(buffer, TXT_OK, TXT_CANCEL, TXT_NONE, true) ==
          1) {
        Set_Logic_Page(old_page);
        while (hidden--) {
          Hide_Mouse();
        }
        return false;
      }

      while (hidden--) {
        Hide_Mouse();
      }
      Set_Font(font);
      Set_Logic_Page(old_page);
    }
  }

  CurrentCD = cd_current;

  SearchPaths::SetCdDrive(new_cd_drive);
  SearchPaths::Refresh();

  // If it broke out of the query for CD-ROM loop, then this means that the
  // CD-ROM has been inserted.
  // kCdCsOrAm is a request, not a disc that exists; narrow it to Aftermath
  // now that a real disc has been found, so the cache check below compares
  // like with like.
  if (cd_desired == 4) {
    cd_desired--;
  }

  // Re-register the secondary mix files from the disc that was just found,
  // but only when the disc actually changed.
  //
  // The cd_desired != kCdDvd condition is ajw's: on the DVD build this ran
  // before Init_Secondary_Mixfiles() and corrupted the mixfile system. Skipping
  // it there is safe, because the DVD is the only disc that can ever be
  // requested when Using_DVD(), and cd_desired can never be kCdDvd otherwise.
  if (cd_desired > -1 && _last != cd_desired && cd_desired != 5) {
    _last = cd_desired;

    Theme.Stop();

    delete MoviesMix;
    delete GeneralMix;
    delete ScoreMix;
    delete MainMix;

    MainMix = MixArchive::Register("MAIN.MIX", &FastKey);
    assert(MainMix != nullptr);
    if (GameFile("MOVIES1.MIX").IsAvailable()) {
      MoviesMix = MixArchive::Register("MOVIES1.MIX", &FastKey);
    } else {
      MoviesMix = MixArchive::Register("MOVIES2.MIX", &FastKey);
    }
    assert(MoviesMix != nullptr);
    GeneralMix = MixArchive::Register("GENERAL.MIX", &FastKey);
    ScoreMix = MixArchive::Register("SCORES.MIX", &FastKey);
    ThemeClass::Scan();
  }

  return true;
}

bool Force_Scenario_Available(const char* name) {
  // Calls Force_CD_Available based on type of scenario. szName is assumed to
  // be an official scenario here.
  if (IsMissionCounterstrike(name)) {
    return Force_CD_Available(4);
  }
  if (IsMissionAftermath(name)) {
    return Force_CD_Available(3);
  }
  return true;
}
