/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\ccfile.h_v   2.18   16 Oct 1995 16:45:28
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CCFILE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : October 17, 1994 *
 *                                                                                             *
 *                  Last Update : October 17, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_CCFILE_H_
#define CNC_RED_ALERT_TD_CCFILE_H_

#include <cstdint>
#include <cstdio>
#include <string_view>

#include "base/types.h"
#include "tech/cdfile.h"
#include "tech/file.h"

/*
**	This derived class for file access knows about mixfiles (packed files).
*It can handle opening *	a file that is embedded within a mixfile. This
*is true if the mixfile is cached or resides on *	disk. It is functionally
*similar to pakfiles, except much faster and less RAM intensive.
*/
class CCFileClass : public CDFileClass {
 public:
  explicit CCFileClass(std::string_view filename);
  CCFileClass();
  ~CCFileClass() override = default;
  CCFileClass(CCFileClass&&) = delete;
  CCFileClass& operator=(CCFileClass&&) = delete;

  // Delete should be overloaded here as well. Don't allow deletes of mixfiles.

  bool Open(std::string_view filename,
            FileAccess rights = FileAccess::kRead) override {
    SetName(filename);
    return Open(rights);
  }
  bool Open(FileAccess rights = FileAccess::kRead) override;
  bool IsAvailable() override;
  [[nodiscard]] bool IsOpen() const override;
  base::ssize Read(void* buffer, base::ssize size) override;
  base::ssize Write(const void* buffer, base::ssize size) override;
  base::ssize Seek(base::ssize offset,
                   SeekOrigin origin = SeekOrigin::kCurrent) override;
  base::ssize Size() override;
  void Close() override;

 private:
  /*
  **	This flag indicates that the file is part of a mixfile and the mixfile
  *resides on *	disk. The file handle for this file is a legitimate DOS handle,
  *although special *	handling is necessary that takes into account the
  *embedded nature of the file.
  */
  bool FromDisk;

  /*
  **	This indicates the file is actually part of a resident image of the
  *mixfile *	itself. In this case, the embedded file handle is invalid. All
  *file access actually *	gets routed through the cached version of the
  *file. This is a pointer to the start *	of the RAM image of the file.
  */
  const void* Pointer;

  /*
  **	This is the starting offset of the beginning of the file. This value is
  *only valid *	if the file is part of a mixfile that resides on disk. It serves
  *as the counterpart *	to the "Pointer" variable.
  */
  base::ssize Start;

  /*
  **	This is the current seek position of the file. It is duplicated here if
  *the file is *	part of a mixfile since the DOS seek position is not
  *accurate. This value will *	range from zero to the size of the file in
  *bytes.
  */
  base::ssize Position;

  /*
  **	This is the size of the file if it was embedded in a mixfile. The size
  *must be manually *	kept track of because the DOS file size is invalid.
  */
  base::ssize Length;

 public:
  // Force these to never be invoked.
  CCFileClass& operator=(const CCFileClass& c) = delete;
  CCFileClass(const CCFileClass&) = delete;
};

void WWDOS_Shutdown();

#endif  // CNC_RED_ALERT_TD_CCFILE_H_
