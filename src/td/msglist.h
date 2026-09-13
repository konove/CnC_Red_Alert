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

/***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : MSGLIST.H                                *
 *                                                                         *
 *                   Programmer : Bill R. Randolph                         *
 *                                                                         *
 *                   Start Date : 05/22/95                                 *
 *                                                                         *
 *                  Last Update : May 22, 1995 [BRR]                       *
 *                                                                         *
 * How the messages work:
 **
 * - MPlayerMessageList is a gadget list of all current messages
 **
 * - MPlayerMessageX & Y are the upper left corner of the 1st message *
 * - MPlayerMaxMessages is the max # of messages allowed, including
 ** the editable message; 0 = no limit.
 **
 * - EditLabel points to the textmessage gadget for the current editable
 ** field.  EditBuf points to the char buffer being edited.  EditInitPos
 ** & EditCurPos define buffer index positions.
 **
 * - EditSendAddress is the IPX Address to send the message to when RETURN
 ** is pressed.
 **
 *																									*
 * The UserData field in the TextLabelClass tells what the timeout for * each
 *message is (0 = none).
 ** When a message's timeout expires, it's deleted.  When a new message * is
 *added, the top message is deleted if MPlayerMaxMessages is exceeded.	*
 *                                                                         *
 * The Edit-able message is never deleted until ESC or RETURN is pressed.
 **
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_TD_MSGLIST_H_
#define CNC_RED_ALERT_TD_MSGLIST_H_

#include <cstdint>

#include "absl/base/attributes.h"
#include "sdllib/keyboard.h"
#include "td/defines.h"
#include "td/txtlabel.h"

/*
**	Class declaration
*/
class MessageListClass {
 public:
  /*
  **	Constructor/Destructor
  */
  MessageListClass();
  ~MessageListClass();
  MessageListClass(const MessageListClass&) = delete;
  MessageListClass& operator=(const MessageListClass&) = delete;
  MessageListClass(MessageListClass&&) = delete;
  MessageListClass& operator=(MessageListClass&&) = delete;

  /*
  **	Initialization
  */
  void Init(int x, int y, int max_msg, int maxchars, int height);
  TextLabelClass* Add_Message(char* txt ABSL_ATTRIBUTE_LIFETIME_BOUND,
                              int color, TextPrintType style, int timeout,
                              uint16_t magic_number, uint16_t crc);

  /*
  **	Message-editing routines
  */
  TextLabelClass* Add_Edit(int color, TextPrintType style,
                           char* to ABSL_ATTRIBUTE_LIFETIME_BOUND, int width);
  char* Get_Edit_Buf();

  /*
  **	Maintenance routines
  */
  int Manage();
  int Input(KeyNumType& input);
  void Draw();
  int Num_Messages();
  void Set_Width(int width);

 private:
  TextLabelClass* MessageList{nullptr};  // list of messages
  int MessageX{0};                       // x-coord of upper-left
  int MessageY{0};                       // y-coord of upper-left
  int MaxMessages{0};                    // max messages allowed
  int MaxChars{0};                       // max allowed chars per message
  int Height{0};                         // height in pixels
  TextLabelClass* EditLabel{nullptr};    // ptr to current edit label
  char* EditBuf{nullptr};                // ptr to current edit buffer
  int EditCurPos{0};                     // current edit position
  int EditInitPos{0};                    // initial edit position
  int Width = 0;                // Maximum width in pixels of editable string

  /*
  ** Static buffers provided for messages.  They must be long enough for
  ** both the message, and for the "To" prefix on edited messages, or
  ** the "From:" prefix on received messages.
  */
  static char MessageBuffers[MAX_NUM_MESSAGES][MAX_MESSAGE_LENGTH + 30];
  static char BufferAvail[MAX_NUM_MESSAGES];
};

#endif  // CNC_RED_ALERT_TD_MSGLIST_H_
