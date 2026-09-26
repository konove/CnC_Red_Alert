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

/* $Header:   F:\projects\c&c\vcs\code\gscreen.cpv   2.17   16 Oct 1995 16:51:34
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : GSCREEN.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 12/15/94 *
 *                                                                                             *
 *                  Last Update : January 19, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * GScreenClass::GScreenClass -- Default constructor for
 *GScreenClass.                       * GScreenClass::One_Time -- Handles one
 *time class setups.                                  * GScreenClass::Init --
 *Init's the entire display hierarchy by calling all Init routines.   *
 *   GScreenClass::Init_Clear -- Sets the map to a known state. *
 *   GScreenClass::Init_Theater -- Performs theater-specific initializations. *
 *   GScreenClass::Init_IO -- Initializes the Button list ('Buttons'). *
 *   GScreenClass::Flag_To_Redraw -- Flags the display to be redrawn. *
 *   GScreenClass::Blit_Display -- Redraw the display from the hidpage to the
 *seenpage.        * GScreenClass::Render -- General drawing dispatcher an
 *display update function.            * GScreenClass::Input -- Fetches input and
 *processes gadgets.                               * GScreenClass::Add_A_Button
 *-- Add a gadget to the game input system.                      *
 *   GScreenClass::Remove_A_Button -- Removes a gadget from the game input
 *system.             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/gscreen.h"

#include <cstdio>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "engine/gfx/pcx_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "engine/window/ww_mouse.h"
#include "td/config.h"
#include "td/debug_state.h"
#include "td/defines.h"
#include "td/display_constants.h"
#include "td/gadget.h"
#include "td/inline.h"
#include "td/input.h"
#include "td/mapedit.h"
#include "td/msglist.h"
#include "td/palette.h"
#include "td/screen.h"
#include "td/session.h"
#include "td/world.h"

GadgetClass* GScreenClass::Buttons = nullptr;

PixelBuffer* GScreenClass::ShadowPage = nullptr;

/***********************************************************************************************
 * GScreenClass::GScreenClass -- Default constructor for GScreenClass. *
 *                                                                                             *
 *    This constructor merely sets the display system, so that it will redraw
 *the first time   * the render function is called. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/15/1994 JLB : Created. *
 *=============================================================================================*/
GScreenClass::GScreenClass() = default;

/***********************************************************************************************
 * GScreenClass::One_Time -- Handles one time class setups. *
 *                                                                                             *
 * This routine (and all those that overload it) must perform truly one-time
 *initialization.   * Such init's would normally be done in the constructor, but
 *other aspects of the game may    * not have been initialized at the time the
 *constructors are called (such as the file system, * the display, or other
 *WWLIB subsystems), so many initializations should be deferred to the  *
 * One_Time init's. *
 *                                                                                             *
 * Any variables set in this routine should be declared as static, so they won't
 *be modified   * by the load/save process.  Non-static variables will be
 *over-written by a loaded game.      *
 *                                                                                             *
 * This function allocates the shadow buffer that is used for quick screen
 *updates. If         * there were any data files to load, they would be loaded
 *at this time as well.               *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Call this routine only ONCE at the beginning of the game. *
 *                                                                                             *
 * HISTORY: * 12/15/1994 JLB : Created. *
 *=============================================================================================*/
void GScreenClass::One_Time() {
  /*
  **	Allocate the screen shadow page. This page is used to reduce access to
  *the *	actual screen memory. It contains a duplicate of what the
  *SEENPAGE is.
  */
  Buttons = nullptr;
  ShadowPage = new PixelBuffer(320, 200);
  if (ShadowPage) {
    ShadowPage->view().Clear();
    TheScreen().hidden_page().view().Clear();
  }
}

/***********************************************************************************************
 * GScreenClass::Init -- Init's the entire display hierarchy by calling all Init
 *routines.     *
 *                                                                                             *
 * This routine shouldn't be overloaded.  It's the main map initialization
 *routine, and will   * perform a complete map initialization, from mixfiles to
 *clearing the buffers.  Calling this * routine results in calling every
 *initialization routine in the entire map hierarchy.        *
 *                                                                                             *
 * INPUT: * theater      theater to initialize to *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 12/28/1994 BR : Created. *
 *=============================================================================================*/
void GScreenClass::Init(TheaterType theater) {
  Init_Clear();
  Init_IO();
  Init_Theater(theater);
}

/***********************************************************************************************
 * GScreenClass::Init_Clear -- Sets the map to a known state. *
 *                                                                                             *
 * This routine (and those that overload it) clears any buffers and variables to
 *a known       * state.  It assumes that all buffers are allocated & valid. The
 *map should be displayable   * after calling this function, and should draw
 *basically an empty display.                    *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 12/28/1994 BR : Created. *
 *=============================================================================================*/
void GScreenClass::Init_Clear() {
  /*
  ** Clear the ShadowPage & HidPage to force a complete shadow blit.
  */
  if (ShadowPage) {
    ShadowPage->view().Clear();
    TheScreen().hidden_page().view().Clear();
  }

  IsScreenToRedraw = true;
}

/***********************************************************************************************
 * GScreenClass::Init_Theater -- Performs theater-specific initializations. *
 *                                                                                             *
 * This routine (and those that overload it) performs any theater-specific
 *initializations     * needed.  This will include setting the palette, setting
 *up remap tables, etc.  This routine * only needs to be called when the theater
 *has changed.                                       *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 12/28/1994 BR : Created. *
 *=============================================================================================*/
void GScreenClass::Init_Theater(TheaterType /*unused*/) {}

/***********************************************************************************************
 * GScreenClass::Init_IO -- Initializes the Button list ('Buttons'). *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 12/28/1994 BR : Created. *
 *=============================================================================================*/
void GScreenClass::Init_IO() {
  /*
  ** Reset the button list.  This means that any other elements of the map that
  *need
  ** buttons must attach them after this routine is called!
  */
  Buttons = nullptr;
}

/***********************************************************************************************
 * GScreenClass::Flag_To_Redraw -- Flags the display to be redrawn. *
 *                                                                                             *
 *    This function is used to flag the display system whether any rendering is
 *needed. The    * parameter tells the system either to redraw EVERYTHING, or
 *just that something somewhere * has changed and the individual Draw_It
 *functions must be called. When a sub system       * determines that it needs
 *to render something local to itself, it would call this routine * with a false
 *parameter. If the entire screen gets trashed or needs to be rebuilt, then   *
 *    this routine will be called with a true parameter. *
 *                                                                                             *
 * INPUT:   complete -- bool; Should the ENTIRE screen be redrawn? *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   This doesn't actually draw the screen, it merely sets flags so
 *that when the    * Render() function is called, the appropriate drawing steps
 *will be performed.   *
 *                                                                                             *
 * HISTORY: * 12/15/1994 JLB : Created. *
 *=============================================================================================*/
void GScreenClass::Flag_To_Redraw(bool complete) {
  IsToUpdate = true;
  if (complete) {
    IsScreenToRedraw = true;
  }
}

/***********************************************************************************************
 * GScreenClass::Input -- Fetches input and processes gadgets. *
 *                                                                                             *
 *    This routine will fetch the keyboard/mouse input and dispatch this through
 *the gadget    * system. *
 *                                                                                             *
 * INPUT:   key      -- Reference to the key code (for future examination). *
 *                                                                                             *
 *          x,y      -- Reference to mouse coordinates (for future examination).
 **
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/19/1995 JLB : Created. *
 *=============================================================================================*/
void GScreenClass::Input(engine::window::KeyNumber& key, int& x, int& y) {
  key = TheKeyboard().Peek();

  x = Get_Mouse_X();
  y = Get_Mouse_Y();

  if (Buttons) {
    /*
    ** If any buttons need redrawing, they will do so in the Input routine, and
    ** they should draw themselves to the HidPage.  So, flag ourselves for a
    *Blit
    ** to show the newly drawn buttons.
    */
    if (Buttons->Is_List_To_Redraw()) {
      Flag_To_Redraw(false);
    }

    key = Buttons->Input(TheScreen().hidden_view()).key;

  } else {
    if (key) {
      key = TheKeyboard().Read();
    }
  }
  AI(key, x, y);
}

/***********************************************************************************************
 * GScreenClass::Add_A_Button -- Add a gadget to the game input system. *
 *                                                                                             *
 *    This will add a gadget to the game input system. The gadget will be
 *processed in         * subsiquent calls to the GScreenClass::Input() function.
 **
 *                                                                                             *
 * INPUT:   gadget   -- Reference to the gadget that will be added to the input
 *system.        *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/19/1995 JLB : Created. *
 *=============================================================================================*/
void GScreenClass::Add_A_Button(GadgetClass& gadget) {
  /*------------------------------------------------------------------------
  If this gadget is already in the list, remove it before adding it in:
  - If 1st gadget in list, use Remove_A_Button to remove it, to reset the
    value of 'Buttons' appropriately
  - Otherwise, just call the Remove function for that gadget to remove it
    from any list it may be in
  ------------------------------------------------------------------------*/
  if (Buttons == &gadget) {
    Remove_A_Button(gadget);
  } else {
    gadget.Remove();
  }

  /*------------------------------------------------------------------------
  Now add the gadget to our list:
  - If there are not buttons, start the list with this one
  - Otherwise, add it to the tail of the existing list
  ------------------------------------------------------------------------*/
  if (Buttons) {
    gadget.Add_Tail(*Buttons);
  } else {
    Buttons = &gadget;
  }
}

/***********************************************************************************************
 * GScreenClass::Remove_A_Button -- Removes a gadget from the game input system.
 **
 *                                                                                             *
 * INPUT:   gadget   -- Reference to the gadget that will be removed from the
 *input system.    *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   'gadget' MUST be already a part of 'Buttons', or the new value of
 *'Buttons'     * will be invalid! *
 *                                                                                             *
 * HISTORY: * 01/19/1995 JLB : Created. *
 *=============================================================================================*/
void GScreenClass::Remove_A_Button(GadgetClass& gadget) {
  Buttons = gadget.Remove();
}

/***********************************************************************************************
 * GScreenClass::Render -- General drawing dispatcher an display update
 *function.              *
 *                                                                                             *
 *    This routine should be called in the main game loop (once every game
 *frame). It will     * call the Draw_It() function if necessary. All rendering
 *is performed to the hidden page. After
 *rendering has been performed, the HIDPAGE is          * copied to the visible
 *page.                                                              *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   This actually updates the graphic display. As a result it can
 *take quite a      * while to perform. *
 *                                                                                             *
 * HISTORY: * 12/15/1994 JLB : Created. *
 *=============================================================================================*/
void GScreenClass::Render() {
  // if (Buttons && Buttons->Is_List_To_Redraw()) {
  //	IsToRedraw = true;
  // }

  if (IsToUpdate || IsScreenToRedraw) {
    // TheMouse()->Erase_Mouse(&HidPage, true);
    PixelView& view = TheScreen().hidden_view();

    // if (IsToRedraw) {
    //	Hide_Mouse();
    //	SeenBuff.CopyToBuffer(0, 0, 320, 200, ShadowPage);
    //	Show_Mouse();
    // }
    Draw_It(view, IsScreenToRedraw);

    if (Buttons) {
      Buttons->Draw_All(view, false);
    }

    if constexpr (config::kScenarioEditorEnabled) {
      /*
      ** Draw the Editor's buttons
      */
      if (TheDebugState().map_editor_active() && Buttons) {
        Buttons->Draw_All(view);
      }
    }
    /*
    ** Draw the multiplayer message system to the Hidpage at this point.
    ** This way, they'll Blit along with the rest of the map.
    */
    if (TheSession().messages().Num_Messages() > 0) {
      TheSession().messages().Set_Width(
          Lepton_To_Cell(TheMap().TacLeptonWidth) * ICON_PIXEL_W);
    }
    TheSession().messages().Draw(view);

    Blit_Display();
    IsToUpdate = false;
    IsScreenToRedraw = false;
  }
}

#define MAX_SCREENS_SAVED (30 * 15)  // Enough for 30 seconds @ 15 fps

static PixelBuffer* ScreenList[MAX_SCREENS_SAVED];
static int CurrentScreen = 0;
bool ScreenRecording = false;

static void Add_Current_Screen() {
  if (ScreenRecording) {
    base::At(ScreenList, CurrentScreen) = new PixelBuffer;
    base::At(ScreenList, CurrentScreen)
        ->Init(TheScreen().visible_view().width(),
               TheScreen().visible_view().height(), {}, 0);
    TheScreen().visible_view().BlitTo(
        base::At(ScreenList, CurrentScreen)->view());

    CurrentScreen++;

    if (CurrentScreen == MAX_SCREENS_SAVED) {
      char filename[20];
      for (int i = 0; i < MAX_SCREENS_SAVED; i++) {
        absl::SNPrintF(filename, sizeof(filename), "SCRN%04d.PCX", i);
        Write_PCX_File(filename, base::At(ScreenList, i)->view(),
                       CurrentPalette);
        delete base::At(ScreenList, i);
      }

      CurrentScreen = 0;
      ScreenRecording = false;
    }
  }
}

extern bool CanVblankSync;

/***********************************************************************************************
 * GScreenClass::Blit_Display -- Redraw the display from the hidpage to the
 *seenpage.          *
 *                                                                                             *
 *    This routine is used to copy the correct display from the HIDPAGE * to the
 *SEENPAGE. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/14/1994 JLB : Created. * 05/01/1994 JLB : Converted to member
 *function.                                            *
 *=============================================================================================*/
void GScreenClass::Blit_Display() {
  TheMouse()->Draw_Mouse(&TheScreen().hidden_view());
  TheScreen().hidden_view().BlitTo(TheScreen().visible_view(), 0, 0, 0, 0,
                                   TheScreen().hidden_view().width(),
                                   TheScreen().hidden_view().height());
  if (config::kCheatKeysEnabled) {
    Add_Current_Screen();
  }
  TheMouse()->Erase_Mouse(&TheScreen().hidden_view(), false);
}
