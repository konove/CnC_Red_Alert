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
#ifndef CNC_RED_ALERT_RA_CHAT_H_
#define CNC_RED_ALERT_RA_CHAT_H_

// File: Inter-player chat: composing, sending and receiving messages.

#include "sdllib/keyboard.h"

// Processes inter-player message input. F1 through F8 open an editable message
// addressed to one player or to everyone, and RETURN sends what has been typed.
//
// The two session types put the text on the wire differently -- a serial
// packet or an IPX global packet -- but both build the same message from the
// same edit buffer.
void Message_Input(KeyNumType& input);

// Services the IPX connection and dispatches any global packet that arrived:
// sign-offs, player chat, or a game-setup packet.
void IPX_Call_Back();

// Hook for the "SECRET UNITS ON" chat trigger, which every machine recognizes
// at the same point. Does nothing: see the definition.
void Enable_Secret_Units();

#endif  // CNC_RED_ALERT_RA_CHAT_H_
