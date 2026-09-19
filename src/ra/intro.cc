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

// The first-launch introduction movie and, on the DVD, the side prompt in front
// of it.

#include "ra/intro.h"

#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/globals.h"
#include "ra/init.h"
#include "ra/movie.h"
#include "ra/mplayer.h"
#include "ra/msgbox.h"
#include "ra/palette.h"
#include "ra/text_ids.h"
#include "sdllib/gbuffer.h"
#include "sdllib/ww_mouse.h"

// ajw: in RA, all this did was play a movie. Denzil's DVD support uses it in
// its original sense again, because a DVD cannot tell the side by which disc is
// in the drive. (5/08/1995 BWG: created.)
void PlayFirstLaunchIntro() {
  // A CD install knows the side from the disc that is in the drive. The DVD
  // holds both campaigns, so the player has to be asked.
  if (Using_DVD()) {
    // Put the title page up as a backdrop for the dialog.
    Hide_Mouse();
    Load_Title_Page();
    GamePalette = CCPalette;
    HidPage.Blit(SeenBuff);
    CCPalette.Set();
    Set_Logic_Page(SeenBuff);
    Show_Mouse();

    // Process() returns the index of the button pressed. CurrentCD uses the
    // disc numbering, 0 for the Allied disc and 1 for the Soviet one.
    switch (WWMessageBox().Process(TXT_CHOOSE, TXT_ALLIES, TXT_SOVIET)) {
      case 0:
        CurrentCD = 0;
        break;

      case 1:
        CurrentCD = 1;
        break;
      default:
        break;
    }

    // Fade out and clear so that the movie starts from black. Mouse hides are
    // counted, so this one needs its own show: the caller balances only the
    // hide it makes itself.
    Hide_Mouse();
    BlackPalette.Set(kFadePaletteSlow);
    SeenBuff.Clear();
    Show_Mouse();
  }

  Play_Movie(VQ_INTRO_MOVIE, THEME_NONE, false);
}
