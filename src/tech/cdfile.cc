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

/* $Header: /CounterStrike/CDFILE.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library *
 *                                                                                             *
 *                    File Name : CDFILE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : October 18, 1994 *
 *                                                                                             *
 *                  Last Update : September 22, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * CDFileClass::ClearSearchPaths -- Removes all record of a
 *search path.                  * CDFileClass::Open -- Opens the file object --
 *with path search.                           * CDFileClass::Open -- Opens the
 *file wherever it can be found.                             *
 *   CDFileClass::SetName -- Performs a multiple directory scan to set the
 *filename.          * CDFileClass::AddSearchPaths -- Sets a list of search
 *paths for file access.            * Is_Disk_Inserted -- Checks to see if a
 *disk is inserted in specified drive.               * harderr_handler --
 *Handles hard DOS errors.                                               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/cdfile.h"

#include <cerrno>
#include <filesystem>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "sdllib/file_access.h"
#include "tech/rawfile.h"

std::vector<std::string> CDFileClass::search_paths_;
std::string CDFileClass::raw_path_;
int CDFileClass::current_cd_drive_ = 0;
int CDFileClass::last_cd_drive_ = 0;

CDFileClass::CDFileClass(const std::string_view filename) {
  CDFileClass::SetName(filename);
}

extern int Get_CD_Index(int cd_drive, int timeout);

/***********************************************************************************************
 * CDFileClass::Open -- Opens the file object -- with path search. *
 *                                                                                             *
 *    This will open the file object, but since the file object could have been
 *constructed    * with a pathname, this routine will try to find the file
 *first. For files opened for      * writing, then use the existing filename
 *without performing a path search.                *
 *                                                                                             *
 * INPUT:   rights   -- The access rights to use when opening the file *
 *                                                                                             *
 * OUTPUT:  bool; Was the open successful? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
bool CDFileClass::Open(FileAccess rights) { return RawFileClass::Open(rights); }

/***********************************************************************************************
 * CDFC::RefreshSearchPaths -- Updates the search path when a CD changes or
 *is added        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 5/22/96 9:01AM ST : Created *
 *=============================================================================================*/
void CDFileClass::RefreshSearchPaths() {
  ClearSearchPaths();
  ProcessPathTokens(raw_path_);
}

int CDFileClass::AddSearchPaths(const std::string_view new_paths) {
  if (new_paths.empty()) {
    return 0;
  }

  // Append to persistent storage.
  // Check !empty() to avoid adding a leading semicolon.
  if (!raw_path_.empty()) {
    raw_path_ += ';';
  }
  raw_path_ += new_paths;

  // Process only the newly added paths to avoid redundant scanning.
  return ProcessPathTokens(new_paths);
}

int CDFileClass::ProcessPathTokens(std::string_view paths) {
  bool found_valid_path = false;

  for (const auto token_range : paths | std::views::split(';')) {
    // Materialize the view into a string for manipulation.
    std::string path(token_range.begin(), token_range.end());

    if (path.empty()) {
      continue;
    }

    // Ensure the path ends with a directory separator.
    // Use std::filesystem to handle platform-specific separators.
    if (!path.empty() &&
        path.back() != std::filesystem::path::preferred_separator &&
        path.back() != ':') {
      path += std::filesystem::path::preferred_separator;
    }

    // Handle Wildcard Resolution ("?:").
    // If a path starts with "?:", it is a placeholder for the CD-ROM drive.
    // We check if the current CD drive has the correct disc (timeout: 2*60
    // ticks).
    if (path.starts_with("?:")) {
      if (current_cd_drive_ && Get_CD_Index(current_cd_drive_, 120) >= 0) {
        // Map the internal drive index (0=A, 1=B...) to a char.
        path[0] = static_cast<char>(current_cd_drive_ + 'A');

        AddSearchPath(path);
        found_valid_path = true;
      }
      // If the wildcard logic was hit (even if no CD found), skip the default
      // add.
      continue;
    }

    AddSearchPath(path);
    found_valid_path = true;
  }

  return found_valid_path ? 0 : 1;
}

/***********************************************************************************************
 * CDFC::AddSearchPath -- Add a new path to the search path list *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    path *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 5/22/96 10:12AM ST : Created *
 *=============================================================================================*/
void CDFileClass::AddSearchPath(const std::string& path) {
  search_paths_.push_back(path);
}

/***********************************************************************************************
 * CDFC::SetCdDrive -- sets the current CD drive letter *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 5/22/96 9:39AM ST : Created *
 *=============================================================================================*/
void CDFileClass::SetCdDrive(int drive) {
  last_cd_drive_ = current_cd_drive_;
  current_cd_drive_ = drive;
}

/***********************************************************************************************
 * CDFileClass::ClearSearchPaths -- Removes all record of a search path. *
 *                                                                                             *
 *    Use this routine to clear out any previous path(s) set with
 *AddSearchPaths()          * function. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
void CDFileClass::ClearSearchPaths() { search_paths_.clear(); }

void CDFileClass::SetName(const std::string_view filename) {
  // Copied first, because filename may view this object's current name, which
  // the SetName calls below overwrite.
  const std::string name(filename);

  // Try to find the file in the current directory first.
  // This preserves the optimization of checking the local filesystem before
  // iterating through the CD/Network search paths.
  RawFileClass::SetName(name);

  // If the file system is disabled, no search paths exist, the name is empty
  // (a search path alone would name a directory), or the file was found
  // locally, keep the name as given.
  if (search_disabled_ || search_paths_.empty() || name.empty() ||
      RawFileClass::IsAvailable()) {
    return;
  }

  // Iterate through all registered search paths.
  for (const auto& base_path : search_paths_) {
    // AddSearchPath guarantees base_path ends with a path separator, so we can
    // safely concatenate directly.
    RawFileClass::SetName(base_path + name);
    if (RawFileClass::IsAvailable()) {
      return;
    }
  }

  // All path searching has failed. Just set the file name to the plain text
  // passed to this routine and be done with it.
  RawFileClass::SetName(name);
}

/***********************************************************************************************
 * CDFileClass::Open -- Opens the file wherever it can be found. *
 *                                                                                             *
 *    This routine is similar to the RawFileClass open except that if the file
 *is being        * opened only for READ access, it will search all specified
 *directories looking for the    * file. If after a complete search the file
 *still couldn't be found, then it is opened     * using the normal
 *RawFileClass system -- resulting in normal error procedures.       *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the override filename to supply for this file
 *object. It    * would be the base filename (sans any directory specification).
 **
 *                                                                                             *
 *          rights   -- The access rights to use when opening the file. *
 *                                                                                             *
 * OUTPUT:  bool; Was the file opened successfully? If so then the filename may
 *be different   * than requested. The location of the file can be determined by
 *examining the  * filename of this file object. The filename will contain the
 *complete         * pathname used to open the file. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
bool CDFileClass::Open(const std::string_view filename, FileAccess rights) {
  CDFileClass::Close();

  /*
  **	Verify that there is a filename associated with this file object. If
  *not, then this is a *	big error condition.
  */
  if (filename.empty()) {
    Error(ENOENT, false);
  }

  /*
  **	If writing is requested, then multiple drive searching is not performed.
  */
  if (search_disabled_ || rights == FileAccess::kWrite) {
    RawFileClass::SetName(filename);
    return RawFileClass::Open(rights);
  }

  /*
  **	Perform normal multiple drive searching for the filename and open
  **	using the normal procedure.
  */
  SetName(filename);
  return RawFileClass::Open(rights);
}
