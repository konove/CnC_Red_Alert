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

/* $Header: /CounterStrike/INTRO.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : INTRO.H *
 *                                                                                             *
 *                   Programmer : Barry W. Green *
 *                                                                                             *
 *                   Start Date : May 8, 1995 *
 *                                                                                             *
 *                  Last Update : May 8, 1995  [BWG] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/globals.h"
#include "ra/init.h"
#include "ra/mplayer.h"
#include "ra/msgbox.h"
#include "ra/palette.h"
#include "ra/special.h"
#include "sdllib/gbuffer.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "winvq/vqa32/vqaplay.h"

/***********************************************************************************************
 * Choose_Side -- play the introduction movies, select house *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS: *
 *                                                                                             *
 * HISTORY: * 5/08/1995 BWG : Created. *
 *=============================================================================================*/
void Choose_Side()  //	ajw - In RA, all this did was play a movie.
                    // Denzil is using it in its original sense.
{
  Whom = HOUSE_GOOD;

  if (Special.IsFromInstall) {
    if (Using_DVD()) {
      Hide_Mouse();
      Load_Title_Page();
      GamePalette = CCPalette;
      HidPage.Blit(SeenPage);
      CCPalette.Set();
      Set_Logic_Page(SeenBuff);
      Show_Mouse();

      switch (WWMessageBox().Process(TXT_CHOOSE, TXT_ALLIES, TXT_SOVIET)) {
        case 0:
          CurrentCD = 0;
          break;

        case 1:
          CurrentCD = 1;
          break;
      }

      Hide_Mouse();
      BlackPalette.Set(kFadePaletteSlow);
      SeenPage.Clear();
    }

    Play_Movie(VQ_INTRO_MOVIE, THEME_NONE, false);
  }
}
