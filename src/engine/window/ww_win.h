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

// File: The SDL event loop the games pump from wherever they wait: it
// presents the frames Display's redraw timer asks for and hands every other
// event to the game's handler. What is left of Barry W. Green's Westwood
// WINDOWS library (February 1995), whose text windows moved to
// engine/gfx/text_window.h.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_WW_WIN_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_WW_WIN_H_

union SDL_Event;

// Function type for the game's SDL event handler.
using EventHandler = void (*)(SDL_Event* event);

// Installs the game's event handler, called once for each event
// PumpEvents polls other than Display's redraw events, which the loop
// consumes itself. With none installed, those events are dropped.
void SetEventHandler(EventHandler handler);

// Function type for the game's per-pass pump hook.
using PumpHandler = void (*)();

// Installs the game's pump handler, called once per PumpEvents pass,
// before events are polled. Red Alert uses this to pump its sockets
// (Socket_Select); Tiberian Dawn installs none. With none installed,
// PumpEvents does nothing extra per pass.
void SetPumpHandler(PumpHandler handler);

// Drains SDL's event queue: runs the pump handler, then ends the frame on
// each of Display's redraw events and passes every other event to the event
// handler. Returns once the queue is empty; it never waits for an event.
//
// It is re-entrant. Ending the frame presents, and Display::Present() pumps
// this loop again so the game's wait-for-input loops keep answering, so a
// redraw event runs a nested loop that drains the rest of the queue.
void PumpEvents();

// Pushes an SDL_QUIT event, which the game's event handler receives on the
// next PumpEvents.
void SDL_Send_Quit();

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_WW_WIN_H_
