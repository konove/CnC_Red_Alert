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

/* $Header:   F:\projects\c&c\vcs\code\cargo.h_v   2.20   16 Oct 1995 16:45:14
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CARGO.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 23, 1994 *
 *                                                                                             *
 *                  Last Update : April 23, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_CARGO_H_
#define CNC_RED_ALERT_TD_CARGO_H_

#include "td/monoc.h"
#include "tech/noinit.h"

class FootClass;

/****************************************************************************
**	This class handles the basic cargo logic.
*/
class CargoClass {
 public:
  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  CargoClass() {
    Quantity = 0;
    CargoHold = nullptr;
  }
  CargoClass(const NoInitClass&) {}
  virtual ~CargoClass() { CargoHold = nullptr; }

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */

  virtual void Debug_Dump(MonoClass* mono) const;
  void AI() {}

  int How_Many() const { return Quantity; }
  bool Is_Something_Attached() const { return CargoHold != nullptr; }
  FootClass* Attached_Object() const;
  FootClass* Detach_Object();
  void Attach(FootClass* object);

  /*
  **	File I/O.
  */
  void Code_Pointers();
  void Decode_Pointers();

 private:
  /*
  **	This is the number of objects attached to this cargo hold. For
  *transporter *	objects, they might contain more than one object.
  */
  unsigned char Quantity;

  /*
  **	This is the target value of any attached object. A value of zero
  *indicates *	that no object is attached.
  */
  FootClass* CargoHold;
};

#endif  // CNC_RED_ALERT_TD_CARGO_H_
