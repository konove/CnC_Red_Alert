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

/* $Header: /CounterStrike/RAWFILE.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library *
 *                                                                                             *
 *                    File Name : RAWFILE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : August 8, 1994 *
 *                                                                                             *
 *                  Last Update : October 18, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * RawFileClass::File_Name -- Returns with the filename associate
 *with the file object.      * RawFileClass::RawFileClass -- Default constructor
 *for a file object.                      * RawFileClass::~RawFileClass --
 *Default deconstructor for a file object.                   *
 *   RawFileClass::Is_Open -- Checks to see if the file is open or not. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef RAWFILE_Hx
#define RAWFILE_Hx

#include <climits>
#include <cstdio>
#include <string>

#include "tech/wwfile.h"

#ifndef WWERROR
#define WWERROR -1
#endif

/*
**	This is the definition of the raw file class. It is derived from the
*abstract base FileClass *	and handles the interface to the low level DOS
*routines. This is the first class in the *	chain of derived file
* classes that actually performs a useful function. With this class, *
*	I/O is possible. More sophisticated features, such as packed files,
* CD-ROM support, *	file caching, and XMS/EMS memory support, are
* handled by derived classes.
**
**	Of particular importance is the need to override the error routine if
*more sophisticated *	error handling is required. This is more than
*likely if greater functionality is derived *	from this base class.
*/
class RawFileClass : public FileClass {
 public:
  /*
  **	This is a record of the access rights used to open the file. These
  *rights are *	used if the file object is duplicated.
  */
  int Rights;

  RawFileClass(const char* filename);
  RawFileClass();
  RawFileClass(const RawFileClass& f);
  RawFileClass& operator=(const RawFileClass& f);
  RawFileClass(RawFileClass&&) = delete;
  RawFileClass& operator=(RawFileClass&&) = delete;
  ~RawFileClass() override;

  const char* File_Name() const override;
  const char* Set_Name(const char* filename) override;
  int Create() override;
  int Delete() override;
  int Is_Open() const override;
  int Open(const char* filename, int rights = READ) override;
  int Open(int rights = READ) override;
  long Read(void* buffer, long size) override;
  long Seek(long pos, int dir = SEEK_CUR) override;
  long Size() override;
  long Write(const void* buffer, long size) override;
  void Close() override;
  unsigned long Get_Date_Time() override;
  bool Set_Date_Time(unsigned long datetime) override;
  void Error(int error, int canretry = false,
             const char* filename = nullptr) override;

  void Bias(int start, int length = -1);

  void* Get_File_Handle() { return Handle; }

  /*
  **	These bias values enable a sub-portion of a file to appear as if it
  **	were the whole file. This comes in very handy for multi-part files such
  *as *	mixfiles.
  */
  int BiasStart;
  int BiasLength;

 protected:
  int Do_Is_Available(AvailabilityCheck mode) override;

  /*
  **	This function returns the largest size a low level DOS read or write may
  **	perform. Larger file transfers are performed in chunks of this size or
  *less.
  */
  long Transfer_Block_Size() { return static_cast<long>((UINT_MAX)) - 16L; }

  long Raw_Seek(long pos, int dir = SEEK_CUR);

 private:
  /*
  **	This is the low level DOS handle. A -1 indicates an empty condition.
  */
  void* Handle;

  /*
  **	This holds the filename string. Using std::string provides automatic
  **	memory management (RAII).
  */
  std::string Filename_;

  //
  // file date and time are in the following formats:
  //
  //      date   bits 0-4   day (0-31)
  //             bits 5-8   month (1-12)
  //             bits 9-15  year (0-119 representing 1980-2099)
  //
  //      time   bits 0-4   second/2 (0-29)
  //             bits 5-10  minutes (0-59)
  //             bits 11-15 hours (0-23)
  //
  unsigned short Date;
  unsigned short Time;
};

/***********************************************************************************************
 * RawFileClass::File_Name -- Returns with the filename associate with the file
 *object.        *
 *                                                                                             *
 *    Use this routine to determine what filename is associated with this file
 *object. If no   * filename has yet been assigned, then this routing will
 *return NULL.                      *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the file name associated with this file
 *object or NULL   * if one doesn't exist. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
inline const char* RawFileClass::File_Name() const {
  return Filename_.empty() ? nullptr : Filename_.c_str();
}

/***********************************************************************************************
 * RawFileClass::RawFileClass -- Default constructor for a file object. *
 *                                                                                             *
 *    This constructs a null file object. A null file object has no file handle
 *or filename    * associated with it. In order to use a file object created in
 *this fashion it must be     * assigned a name and then opened. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
inline RawFileClass::RawFileClass()
    : Rights(READ),
      BiasStart(0),
      BiasLength(-1),
      Handle(nullptr),
      Date(0),
      Time(0) {}

/***********************************************************************************************
 * RawFileClass::~RawFileClass -- Default deconstructor for a file object. *
 *                                                                                             *
 *    This constructs a null file object. A null file object has no file handle
 *or filename    * associated with it. In order to use a file object created in
 *this fashion it must be     * assigned a name and then opened. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
inline RawFileClass::~RawFileClass() {
  Close();
  // Filename_ (std::string) automatically cleans up via RAII
}

/***********************************************************************************************
 * RawFileClass::Is_Open -- Checks to see if the file is open or not. *
 *                                                                                             *
 *    Use this routine to determine if the file is open. It returns true if it
 *is.             *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Is the file open? *
 *                                                                                             *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
inline int RawFileClass::Is_Open() const { return Handle != nullptr; }

#endif  // RAWFILE_Hx
