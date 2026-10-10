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

/* $Header:   F:\projects\c&c\vcs\code\radio.cpv   2.19   16 Oct 1995 16:50:12
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : RADIO.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : June 25, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:
 *   RadioClass::Receive_Message -- Handles receipt of a radio message.
 *   RadioClass::Transmit_Message -- Transmit message from one object to
 *     another.
 *   RadioClass::Limbo -- When limboing a unit will always break radio
 *     contact.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/objects/radio.h"

#include <cstdint>
#include <optional>

#include "td/core/defines.h"

namespace td {

RadioPeer::~RadioPeer() = default;

RadioMessageType RadioClass::Transmit(RadioPeer& sender,
                                      RadioMessageType message, int32_t& param,
                                      RadioPeer* to) {
  if (to == nullptr) {
    to = contact_;
  }

  /*
  **	If there is no target for the radio message, then always return static.
  */
  if (to == nullptr) {
    return RADIO_STATIC;
  }

  /*
  **	Handle some special case processing that occurs when certain messages
  **	are transmitted.
  */
  if (to == contact_ && message == RADIO_OVER_OUT) {
    contact_ = nullptr;
  }

  /*
  **	If the message indicates that radio contact should be established,
  **	then try to do so, after ending the conversation this radio is in.
  **	If the other party agrees then contact is established.
  */
  if (message == RADIO_HELLO) {
    Transmit(sender, RADIO_OVER_OUT, DiscardedParam());
    if (to->Receive(&sender, message, param) == RADIO_ROGER) {
      contact_ = to;
      return RADIO_ROGER;
    }
    return RADIO_NEGATIVE;
  }

  return to->Receive(&sender, message, param);
}

std::optional<RadioMessageType> RadioClass::Receive(RadioPeer* from,
                                                    RadioMessageType message,
                                                    bool can_accept_contact) {
  /*
  **	Keep a record of the last message received by this radio.
  */
  LastMessage = message;

  /*
  **	When this message is received, it means that the other object
  **	has already turned its radio off. Turn this radio off as well.
  **	This only applies if the message is coming from the object that
  **	has an established conversation with this object.
  */
  if (from == contact_ && message == RADIO_OVER_OUT) {
    Off();
    return RADIO_ROGER;
  }

  /*
  **	The "hello" message is an attempt to establish contact. If this radio
  **	is already in an established conversation with another object, then
  **	return with "negative". If all is well, return with "roger".
  */
  if (message == RADIO_HELLO && can_accept_contact) {
    if (contact_ == from || contact_ == nullptr) {
      contact_ = from;
      return RADIO_ROGER;
    }
    return RADIO_NEGATIVE;
  }

  return std::nullopt;
}

}  // namespace td
