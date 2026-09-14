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

#include <string>
#include <string_view>

#include "sdllib/file_access.h"
#include "tech/disk_file.h"
#include "tech/search_paths.h"

CDFileClass::CDFileClass(const std::string_view filename) {
  CDFileClass::SetName(filename);
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
bool CDFileClass::Open(FileAccess rights) { return DiskFile::Open(rights); }

void CDFileClass::SetName(const std::string_view filename) {
  if (search_disabled_) {
    DiskFile::SetName(filename);
    return;
  }
  // Resolved into a string before SetName runs, since filename may view this
  // object's current name.
  const std::string resolved =
      SearchPaths::Resolve(filename).value_or(std::string(filename));
  DiskFile::SetName(resolved);
}

/***********************************************************************************************
 * CDFileClass::Open -- Opens the file wherever it can be found. *
 *                                                                                             *
 *    This routine is similar to the DiskFile open except that if the file
 *is being        * opened only for READ access, it will search all specified
 *directories looking for the    * file. If after a complete search the file
 *still couldn't be found, then it is opened     * using the normal
 *DiskFile system -- resulting in normal error procedures.       *
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
  **	If writing is requested, then multiple drive searching is not performed.
  */
  if (search_disabled_ || rights == FileAccess::kWrite) {
    DiskFile::SetName(filename);
    return DiskFile::Open(rights);
  }

  /*
  **	Perform normal multiple drive searching for the filename and open
  **	using the normal procedure.
  */
  SetName(filename);
  return DiskFile::Open(rights);
}
