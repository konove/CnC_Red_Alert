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
 * Functions: * CDFileClass::Clear_Search_Drives -- Removes all record of a
 *search path.                  * CDFileClass::Open -- Opens the file object --
 *with path search.                           * CDFileClass::Open -- Opens the
 *file wherever it can be found.                             *
 *   CDFileClass::Set_Name -- Performs a multiple directory scan to set the
 *filename.          * CDFileClass::Set_Search_Drives -- Sets a list of search
 *paths for file access.            * Is_Disk_Inserted -- Checks to see if a
 *disk is inserted in specified drive.               * harderr_handler --
 *Handles hard DOS errors.                                               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/cdfile.h"

#include <cerrno>
#include <filesystem>
#include <ranges>

#include "sdllib/file.h"

std::vector<std::string> CDFileClass::search_paths_;
std::string CDFileClass::raw_path_;
int CDFileClass::current_cd_drive_ = 0;
int CDFileClass::last_cd_drive_ = 0;

CDFileClass::CDFileClass(const char* filename) : is_disabled_(false) {
  CDFileClass::Set_Name(filename);
}

CDFileClass::CDFileClass() : is_disabled_(false) {}
extern int Get_CD_Index(int cd_drive, int timeout);

/***********************************************************************************************
 * Is_Disk_Inserted -- Checks to see if a disk is inserted in specified drive. *
 *                                                                                             *
 *    This routine will examine the drive specified to see if there is a disk
 *inserted. It     * can be used for floppy drives as well as for the CD-ROM. *
 *                                                                                             *
 * INPUT:   disk  -- The drive number to examine. 0=A, 1=B, etc. *
 *                                                                                             *
 * OUTPUT:  bool; Is a disk inserted into the specified drive? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/20/1995 JLB : Created. *
 *=============================================================================================*/
int cdecl Is_Disk_Inserted(int disk) {
  char scan[] = "?:\\*.*";

  scan[0] = static_cast<char>('A' + disk);

  // yeah this isn't going to work on non-windows...
  FindFileState state;
  bool ret = Find_First_File(scan, state);
  End_Find_File(state);
  return ret;
}

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
int CDFileClass::Open(FileAccess rights) {
  return BufferIOFileClass::Open(rights);
}

/***********************************************************************************************
 * CDFC::Refresh_Search_Drives -- Updates the search path when a CD changes or
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
void CDFileClass::Refresh_Search_Drives() {
  Clear_Search_Drives();
  Process_Path_Tokens(raw_path_);
}

int CDFileClass::Add_Search_Drives(const std::string_view new_paths) {
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
  return Process_Path_Tokens(new_paths);
}

int CDFileClass::Process_Path_Tokens(std::string_view paths) {
  bool found_valid_drive = false;

  for (auto token_range : paths | std::views::split(';')) {
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

        Add_Search_Drive(path);
        found_valid_drive = true;
      }
      // If the wildcard logic was hit (even if no CD found), skip the default
      // add.
      continue;
    }

    Add_Search_Drive(path);
    found_valid_drive = true;
  }

  return found_valid_drive ? 0 : 1;
}

/***********************************************************************************************
 * CDFC::Add_Search_Drive -- Add a new path to the search path list *
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
void CDFileClass::Add_Search_Drive(const std::string& path) {
  search_paths_.push_back(path);
}

/***********************************************************************************************
 * CDFC::Set_CD_Drive -- sets the current CD drive letter *
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
void CDFileClass::Set_CD_Drive(int drive) {
  last_cd_drive_ = current_cd_drive_;
  current_cd_drive_ = drive;
}

/***********************************************************************************************
 * CDFileClass::Clear_Search_Drives -- Removes all record of a search path. *
 *                                                                                             *
 *    Use this routine to clear out any previous path(s) set with
 *Set_Search_Drives()          * function. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
void CDFileClass::Clear_Search_Drives() { search_paths_.clear(); }

/***********************************************************************************************
 * CDFileClass::Set_Name -- Performs a multiple directory scan to set the
 *filename.            *
 *                                                                                             *
 *    This routine will scan all the directories specified in the path list and
 *if the file    * was found in one of the directories, it will set the filename
 *to a composite of the      * correct directory and the filename. It is used to
 *allow path searching when searching    * for files. Typical use is to support
 *CD-ROM drives. This routine examines the current    * directory first before
 *scanning through the path list. If after scanning the entire      * path list,
 *the file still could not be found, then the file object's name is set with *
 *    just the raw filename as passed to this routine. *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the filename to set as the name of this file
 *object.        *
 *                                                                                             *
 * OUTPUT:  Returns a pointer to the final and complete filename of this file
 *object. This     * may have a path attached to the file. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
const char* CDFileClass::Set_Name(const char* filename) {
  // Try to find the file in the current directory first.
  // This preserves the optimization of checking the local filesystem before
  // iterating through the CD/Network search paths.
  BufferIOFileClass::Set_Name(filename);

  // If the file system is disabled, no search paths exist, or the file
  // was found locally, return the current result immediately.
  if (is_disabled_ || search_paths_.empty() ||
      BufferIOFileClass::Do_Is_Available(AvailabilityCheck::kQuick)) {
    return File_Name();
  }

  // Iterate through all registered search paths.
  for (const auto& base_path : search_paths_) {
    // Construct the full path.
    // Note: Add_Search_Drive guarantees base_path ends with a path separator,
    // so we can safely concatenate directly.
    std::string full_path = base_path + filename;

    // Check availability on this specific drive/path.
    BufferIOFileClass::Set_Name(full_path.c_str());
    if (BufferIOFileClass::Do_Is_Available(AvailabilityCheck::kQuick)) {
      return File_Name();
    }
  }

  /*
  **	At this point, all path searching has failed. Just set the file name to
  *the *	plain text passed to this routine and be done with it.
  */
  BufferIOFileClass::Set_Name(filename);
  return File_Name();
}

/***********************************************************************************************
 * CDFileClass::Open -- Opens the file wherever it can be found. *
 *                                                                                             *
 *    This routine is similar to the RawFileClass open except that if the file
 *is being        * opened only for READ access, it will search all specified
 *directories looking for the    * file. If after a complete search the file
 *still couldn't be found, then it is opened     * using the normal
 *BufferIOFileClass system -- resulting in normal error procedures.       *
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
int CDFileClass::Open(const char* filename, FileAccess rights) {
  CDFileClass::Close();

  /*
  **	Verify that there is a filename associated with this file object. If
  *not, then this is a *	big error condition.
  */
  if (!filename) {
    Error(ENOENT, false);
  }

  /*
  **	If writing is requested, then multiple drive searching is not performed.
  */
  if (is_disabled_ || rights == FileAccess::kWrite) {
    BufferIOFileClass::Set_Name(filename);
    return BufferIOFileClass::Open(rights);
  }

  /*
  **	Perform normal multiple drive searching for the filename and open
  **	using the normal procedure.
  */
  Set_Name(filename);
  return BufferIOFileClass::Open(rights);
}
