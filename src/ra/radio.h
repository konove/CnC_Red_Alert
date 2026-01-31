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

/* $Header: /CounterStrike/RADIO.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : RADIO.H *
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

#ifndef RADIO_H
#define RADIO_H

#include "ra/defines.h"
#include "ra/globals.h"
#include "ra/mission.h"
#include "ra/monoc.h"
#include "ra/object.h"
#include "tech/noinit.h"

/****************************************************************************
**	Radio contact is controlled by this class. It handles the mundane chore
**	of keeping the radio contact alive as well as broadcasting messages
**	to the receiving radio. Radio contact is primarily used when one object
**	is in "command" of another.
*/
class RadioClass : public MissionClass {
 private:
  /*
  **	This is a record of the last message received by this receiver.
  */
  RadioMessageType Old[3];

  /*
  **	This is the object that radio communication has been established
  **	with. Although is is only a one-way reference, it is required that
  **	the receiving radio is also tuned to the object that contains this
  **	radio set.
  */
  RadioClass* Radio;

  /*
  **	This is a text representation of all the possible radio messages. This
  **	text is used for monochrome debug printing.
  */
  static const char* Messages[RADIO_COUNT];

 public:
  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  RadioClass(RTTIType rtti, int id) : MissionClass(rtti, id), Radio(nullptr) {}
  RadioClass(const NoInitClass& x) : MissionClass(x) {}
  ~RadioClass() override { Radio = nullptr; }

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  bool In_Radio_Contact() const { return Radio != nullptr; }
  void Radio_Off() { Radio = nullptr; }
  TechnoClass* Contact_With_Whom() const { return (TechnoClass*)Radio; }

  // Inherited from base class(es).
  RadioMessageType Receive_Message(RadioClass* from, RadioMessageType message,
                                   long& param) override;
  virtual RadioMessageType Transmit_Message(RadioMessageType message,
                                            long& param = LParam,
                                            RadioClass* to = nullptr);
  virtual RadioMessageType Transmit_Message(RadioMessageType message,
                                            RadioClass* to);
  void Debug_Dump(MonoClass* mono) const override;
  bool Limbo() override;

  /*
  **	File I/O.
  */
  void Code_Pointers() override;
  void Decode_Pointers() override;
};

#endif
