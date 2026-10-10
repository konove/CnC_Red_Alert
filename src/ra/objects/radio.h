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

#ifndef CNC_RED_ALERT_RA_OBJECTS_RADIO_H_
#define CNC_RED_ALERT_RA_OBJECTS_RADIO_H_

#include <cstdint>
#include <optional>

#include "ra/core/defines.h"

namespace ra {

// What a radio can talk to: something that takes a message and answers it.
// TechnoClass is the one implementation in the game; a test supplies its own.
class RadioPeer {
 public:
  RadioPeer() = default;
  virtual ~RadioPeer();
  RadioPeer(const RadioPeer&) = delete;
  RadioPeer& operator=(const RadioPeer&) = delete;
  RadioPeer(RadioPeer&&) = delete;
  RadioPeer& operator=(RadioPeer&&) = delete;

  // Delivers `message` and returns the answer, RADIO_STATIC when the message
  // means nothing to the receiver. `from` is the sender, nullptr when there
  // is none. `param` carries a value in either direction for the messages
  // that have one.
  virtual RadioMessageType Receive(RadioPeer* from, RadioMessageType message,
                                   int32_t& param) = 0;
};

// The radio set of one object. It keeps the contact, the other party of an
// established conversation, and sends messages to it or to a named peer.
// Contact is made with RADIO_HELLO and ended with RADIO_OVER_OUT; while it
// lasts each side's radio points at the other, which is how one object
// "commands" another (a transport and its passenger, a refinery and its
// harvester).
//
// The radio does not know its owner. The owner passes itself to Transmit()
// as the sender and calls Receive() from its RadioPeer::Receive().
//
// Example:
//   radio.Transmit(*this, RADIO_HELLO, param, &transport);
//   if (radio.InContact()) { ... }
class RadioClass {
 public:
  // A throwaway lvalue for the messages that carry no parameter. Its value
  // means nothing, and every caller that leaves the parameter out shares it.
  static int32_t& DiscardedParam() {
    static int32_t discarded = 0;
    return discarded;
  }

  // Saved-game support for the message history. The owner saves the contact,
  // since it knows what kind of object that is, and restores it with
  // set_contact().
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(Old);
  }

  [[nodiscard]] bool InContact() const { return contact_ != nullptr; }

  // Drops the contact without telling it; Transmit() of RADIO_OVER_OUT is the
  // polite way.
  void Off() { contact_ = nullptr; }

  // The other party of the conversation, nullptr when there is none.
  [[nodiscard]] RadioPeer* contact() const { return contact_; }
  void set_contact(RadioPeer* contact) { contact_ = contact; }

  // Sends `message` from `sender`, the owner of this radio, to `to`, or to
  // the contact when `to` is nullptr, and returns the answer; RADIO_STATIC
  // when there is nobody to send it to. RADIO_HELLO first ends the current
  // conversation and makes `to` the contact if it answers RADIO_ROGER,
  // returning RADIO_NEGATIVE otherwise. RADIO_OVER_OUT to the contact ends
  // the conversation.
  RadioMessageType Transmit(RadioPeer& sender, RadioMessageType message,
                            int32_t& param, RadioPeer* to = nullptr);

  // Takes note of a message the owner received and answers the ones that are
  // about the conversation itself: RADIO_OVER_OUT from the contact ends it,
  // and RADIO_HELLO starts one with `from` unless this radio is talking to
  // somebody else. Returns nullopt for a message the owner has to answer.
  // An owner that is in no state to talk (a destroyed object) passes false
  // for `can_accept_contact`, and RADIO_HELLO is then left to it too.
  std::optional<RadioMessageType> Receive(RadioPeer* from,
                                          RadioMessageType message,
                                          bool can_accept_contact);

 private:
  // The last three different messages received, newest first.
  RadioMessageType Old[3]{RADIO_STATIC, RADIO_STATIC, RADIO_STATIC};

  // The other party of the conversation, nullptr when there is none. It is
  // a one-way reference: contact is only established when the other radio
  // points back at the owner of this one.
  RadioPeer* contact_ = nullptr;
};

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_OBJECTS_RADIO_H_
