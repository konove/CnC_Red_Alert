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

/* $Header:   F:\projects\c&c\vcs\code\radio.h_v   2.15   16 Oct 1995 16:45:32
 * JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_RADIO_H_
#define CNC_RED_ALERT_TD_RADIO_H_

#include "td/defines.h"
#include "td/globals.h"
#include "td/mission.h"
#include "td/object.h"
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
  RadioMessageType LastMessage;

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
  static char const* Messages[RADIO_COUNT];

 public:
  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  RadioClass() {
    Radio = nullptr;
    LastMessage = RADIO_STATIC;
  }
  RadioClass(NoInitClass const& x) : MissionClass(x) {}
  ~RadioClass() override {}

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
#ifdef CHEAT_KEYS
  virtual void Debug_Dump(MonoClass* mono) const;
#endif
  bool Limbo() override;

  /*
  **	File I/O.
  */
  void Code_Pointers() override;
  void Decode_Pointers() override;
};

#endif  // CNC_RED_ALERT_TD_RADIO_H_
