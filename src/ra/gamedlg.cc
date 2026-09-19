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

/* $Header: /CounterStrike/GAMEDLG.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : GAMEDLG.CPP *
 *                                                                                             *
 *                   Programmer : Maria del Mar McCready Legg, Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : Jan 8, 1995 *
 *                                                                                             *
 *                  Last Update : Jan 18, 1995   [MML] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * OptionsClass::Process -- Handles all the options graphic
 *interface.                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/gamedlg.h"

#include "base/array.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/event.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/mapedit.h"
#include "ra/msgbox.h"
#include "ra/options.h"
#include "ra/queue.h"
#include "ra/sounddlg.h"
#include "ra/text_ids.h"
#include "ra/textbtn.h"
#include "ra/visudlg.h"
#include "ra/wol_main.h"
#include "ra/wolapiob.h"
#include "ra/wolstrng.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "session.h"
#include "slider.h"
#include "tech/ww_audio.h"

/***********************************************************************************************
 * OptionsClass::Process -- Handles all the options graphic interface. *
 *                                                                                             *
 *    This routine is the main control for the visual representation of the
 *options            * screen. It handles the visual overlay and the player
 *input.                              *
 *                                                                                             *
 * INPUT:   none * OUTPUT:  none * WARNINGS:   none * HISTORY: * 12/31/1994 MML
 *: Created.                                                                 *
 *=============================================================================================*/
void GameControlsClass::Process() {
  /*
  **	Dialog & button dimensions
  */
  const int d_dialog_w = 464;                                // dialog width
  int d_dialog_h = 282;                                      // dialog height
  const int d_dialog_x =
      (SeenBuff.Get_Width() - d_dialog_w) / 2;  // dialog x-coord
  int d_dialog_y =
      (SeenBuff.Get_Height() - d_dialog_h) / 2;   // centered y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // center x-coord
  const int d_top_margin = 50;

  const int d_txt6_h = 12 + 1;  // ht of 6-pt text
  const int d_margin1 = 10;     // large margin

  const int d_speed_w = d_dialog_w - 68;
  const int d_speed_h = 12;
  const int d_speed_x = d_dialog_x + 34;
  // The German captions sit a little higher.
  constexpr int kGermanOffsetY = config::kIsGerman ? 4 : 0;
  const int d_speed_y =
      d_dialog_y + d_top_margin + d_margin1 + d_txt6_h - kGermanOffsetY;

  const int d_scroll_w = d_dialog_w - 68;
  const int d_scroll_h = 12;
  const int d_scroll_x = d_dialog_x + 34;
  const int d_scroll_y = d_speed_y + d_speed_h + d_txt6_h + (d_margin1 * 2) +
                         d_txt6_h - kGermanOffsetY;

  const int d_visual_w = d_dialog_w - 80;
  const int d_visual_h = 18;
  const int d_visual_x = d_dialog_x + 40;
  const int d_visual_y = d_scroll_y + d_scroll_h + d_txt6_h + (d_margin1 * 2);

  const int d_sound_w = d_dialog_w - 80;
  const int d_sound_h = 18;
  const int d_sound_x = d_dialog_x + 40;
  const int d_sound_y = d_visual_y + d_visual_h + d_margin1;

  const int d_ok_w = 40;
  const int d_ok_h = 18;
  const int d_ok_x = d_dialog_cx - (d_ok_w / 2);
  int d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - d_margin1 - 8;

  const int d_wol_x = d_sound_x;
  const int d_wol_y = d_sound_y + d_sound_h + d_margin1;
  const int d_wol_w = d_sound_w;
  const int d_wol_h = d_sound_h;

  //	The Westwood Online options button only appears while connected.
  const bool bShowWolapi =
      config::kWolapiEnabled && pWolapi != nullptr && !pWolapi->bConnectionDown;
  if (bShowWolapi) {
    //	Enlarge dialog and shift ok button down.
    d_dialog_h += d_wol_h + d_margin1;
    d_dialog_y =
        ((SeenBuff.Get_Height() - d_dialog_h) / 2);  // centered y-coord
    // d_ok_y += d_wol_h + d_margin1;
    d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - d_margin1 - 8;
  }

  /*
  **	Button Enumerations
  */
  //	kButtonWolapi is always in the list, and so always takes a slot in
  //	buttons[] below, even when bShowWolapi keeps it off the screen. One
  //	spare pointer costs less than two spellings of the enum.
  constexpr int kButtonSpeed = 100;
  constexpr int kButtonScrollrate = 101;
  constexpr int kButtonVisual = 102;
  constexpr int kButtonSound = 103;
  constexpr int kButtonWolapi = 104;
  constexpr int kButtonOk = 105;
  constexpr int kButtonCount = 106;
  constexpr int kButtonFirst = kButtonSpeed;

  /*
  **	Dialog variables
  */

  int gamespeed = static_cast<int>(Options.GameSpeed);
  int scrollrate = Options.ScrollRate;
  int selection = 0;
  bool pressed = false;
  int curbutton = 0;
  TextButtonClass* buttons[kButtonCount - kButtonFirst] = {};

  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();

  /*
  **	Buttons
  */

  SliderClass gspeed_btn(kButtonSpeed, d_speed_x, d_speed_y, d_speed_w,
                         d_speed_h, true);
  SliderClass scrate_btn(kButtonScrollrate, d_scroll_x, d_scroll_y, d_scroll_w,
                         d_scroll_h, true);
  TextButtonClass visual_btn(kButtonVisual, TXT_VISUAL_CONTROLS, kTpfButton,
                             d_visual_x, d_visual_y, d_visual_w, d_visual_h);
  TextButtonClass sound_btn(kButtonSound, TXT_SOUND_CONTROLS, kTpfButton,
                            d_sound_x, d_sound_y, d_sound_w, d_sound_h);
  TextButtonClass okbtn(kButtonOk, TXT_OPTIONS_MENU, kTpfButton, d_ok_x,
                        d_ok_y);
  okbtn.X = (SeenBuff.Get_Width() - okbtn.Width) / 2;

  TextButtonClass wol_btn(kButtonWolapi, TXT_WOL_OPTTITLE, kTpfButton, d_wol_x,
                          d_wol_y, d_wol_w, d_wol_h);

  /*
  **	Various Inits.
  */
  Set_Logic_Page(SeenBuff);

  /*
  **	Build button list
  */
  GadgetClass* commands = &okbtn;  // button list
  gspeed_btn.Add_Tail(*commands);
  scrate_btn.Add_Tail(*commands);
  visual_btn.Add_Tail(*commands);
  sound_btn.Add_Tail(*commands);
  if (bShowWolapi) {
    wol_btn.Add_Tail(*commands);
  }
  /*
  **	Init button states
  **	For sliders, the thumb ranges from 0 - (maxval-1), so to convert the
  **	thumb value to a real-world value:
  **		val = (std::max - slider.Get_Value()) - 1;
  **	and,
  **		slider.Set_Value(-(val + 1 - std::max));
  */
  gspeed_btn.Set_Maximum(OptionsClass::kMaxSpeedSetting);  // varies from 0 - 7
  gspeed_btn.Set_Thumb_Size(1);
  gspeed_btn.Set_Value(OptionsClass::kMaxSpeedSetting - 1 - gamespeed);

  scrate_btn.Set_Maximum(OptionsClass::kMaxScrollSetting);  // varies from 0 - 7
  scrate_btn.Set_Thumb_Size(1);
  scrate_btn.Set_Value(OptionsClass::kMaxScrollSetting - 1 - scrollrate);

  /*
  **	Fill array of button ptrs.
  */
  buttons[0] = nullptr;
  buttons[1] = nullptr;
  buttons[2] = &visual_btn;
  buttons[3] = &sound_btn;
  buttons[4] = &wol_btn;
  buttons[5] = &okbtn;
  /*
  **	Processing loop.
  */
  bool process = true;
  bool display = true;
  bool refresh = true;
  while (process) {
    /*
    **	Invoke game callback.
    */
    if (Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH) {
      ServiceRealTime();
    } else {
      if (RunFrame()) {
        process = false;
      }
    }

    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = true;
    }
    /*
    **	Refresh display if needed.
    */
    if (display) {
      Hide_Mouse();

      Map.Flag_To_Redraw(true);
      Map.Render();

      Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
      Draw_Caption(TXT_GAME_CONTROLS, d_dialog_x, d_dialog_y, d_dialog_w);
      Show_Mouse();
      display = false;
      refresh = true;
    }

    if (refresh) {
      Hide_Mouse();

      /*
      **	Label the game speed slider
      */
      TextPrintType style = kTpfText;
      if (curbutton == kButtonSpeed - kButtonFirst) {
        style = style | TPF_BRIGHT_COLOR;
      }
      Fancy_Text_Print(TXT_SPEED, d_speed_x, d_speed_y - d_txt6_h, scheme,
                       kTBlack, style);

      Fancy_Text_Print(TXT_SLOWER, d_speed_x, d_speed_y + d_speed_h + 2, scheme,
                       kTBlack, kTpfText);
      Fancy_Text_Print(TXT_FASTER, d_speed_x + d_speed_w,
                       d_speed_y + d_speed_h + 2, scheme, kTBlack,
                       kTpfText | TPF_RIGHT);

      /*
      **	Label the scroll rate slider
      */
      style = kTpfText;
      if (curbutton == kButtonScrollrate - kButtonFirst) {
        style = style | TPF_BRIGHT_COLOR;
      }
      Fancy_Text_Print(TXT_SCROLLRATE, d_scroll_x, d_scroll_y - d_txt6_h,
                       scheme, kTBlack, style);

      Fancy_Text_Print(TXT_SLOWER, d_scroll_x, d_scroll_y + d_scroll_h + 2,
                       scheme, kTBlack, kTpfText);
      Fancy_Text_Print(TXT_FASTER, d_scroll_x + d_scroll_w,
                       d_scroll_y + d_scroll_h + 2, scheme, kTBlack,
                       kTpfText | TPF_RIGHT);

      commands->Draw_All();

      Show_Mouse();
      refresh = false;
    }

    /*
    **	Get user input.
    */
    const KeyNumType input = commands->Input();

    /*
    **	Process input.
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonSpeed):
        curbutton = kButtonSpeed - kButtonFirst;
        refresh = true;
        break;

      case ButtonKey(kButtonScrollrate):
        curbutton = kButtonScrollrate - kButtonFirst;
        refresh = true;
        break;

      case ButtonKey(kButtonVisual):
        selection = kButtonVisual;
        pressed = true;
        break;

      case ButtonKey(kButtonSound):
        selection = kButtonSound;
        pressed = true;
        break;

      case ButtonKey(kButtonOk):
        selection = kButtonOk;
        pressed = true;
        break;

      case ButtonKey(kButtonWolapi):
        selection = kButtonWolapi;
        pressed = true;
        break;

      case KN_ESC:
        process = false;
        break;

      case KN_LEFT:
        if (curbutton == kButtonSpeed - kButtonFirst) {
          gspeed_btn.Bump(true);
        } else if (curbutton == kButtonScrollrate - kButtonFirst) {
          scrate_btn.Bump(true);
        }
        break;

      case KN_RIGHT:
        if (curbutton == kButtonSpeed - kButtonFirst) {
          gspeed_btn.Bump(false);
        } else if (curbutton == kButtonScrollrate - kButtonFirst) {
          scrate_btn.Bump(false);
        }
        break;

      case KN_UP:
        if (base::At(buttons, curbutton)) {
          base::At(buttons, curbutton)->Turn_Off();
          base::At(buttons, curbutton)->Flag_To_Redraw();
        }

        curbutton--;
        if (!bShowWolapi && curbutton == kButtonWolapi - kButtonFirst) {
          curbutton--;  //	Skip over missing button.
        }
        if (curbutton < 0) {
          curbutton = kButtonCount - kButtonFirst - 1;
        }

        if (base::At(buttons, curbutton)) {
          base::At(buttons, curbutton)->Turn_On();
          base::At(buttons, curbutton)->Flag_To_Redraw();
        }
        refresh = true;
        break;

      case KN_DOWN:
        if (base::At(buttons, curbutton)) {
          base::At(buttons, curbutton)->Turn_Off();
          base::At(buttons, curbutton)->Flag_To_Redraw();
        }

        curbutton++;
        if (!bShowWolapi && curbutton == kButtonWolapi - kButtonFirst) {
          curbutton++;  //	Skip over missing button.
        }
        if (curbutton > kButtonCount - kButtonFirst - 1) {
          curbutton = 0;
        }

        if (base::At(buttons, curbutton)) {
          base::At(buttons, curbutton)->Turn_On();
          base::At(buttons, curbutton)->Flag_To_Redraw();
        }
        refresh = true;
        break;

      case KN_RETURN:
        selection = curbutton + kButtonFirst;
        pressed = true;
        break;

      default:
        break;
    }

    /*
    **	Perform some action. Either to exit the dialog or bring up another.
    */
    if (pressed) {
      /*
      **	Record the new options slider settings.
      ** The GameSpeed data member MUST NOT BE SET HERE!  It will cause
      *multiplayer
      ** games to go out of sync.  It's set by virtue of the event being
      *executed.
      */
      if (gamespeed !=
          OptionsClass::kMaxSpeedSetting - 1 - gspeed_btn.Get_Value()) {
        gamespeed = OptionsClass::kMaxSpeedSetting - 1 - gspeed_btn.Get_Value();
        OutList.Add(EventClass(EventClass::GAMESPEED, gamespeed));
      }

      if (scrollrate !=
          OptionsClass::kMaxScrollSetting - 1 - scrate_btn.Get_Value()) {
        scrollrate =
            OptionsClass::kMaxScrollSetting - 1 - scrate_btn.Get_Value();
        Options.ScrollRate = scrollrate;
      }
      process = false;

      /*
      ** Save the settings in such a way that the GameSpeed is only set during
      ** the save process; restore it when we're done, so multiplayer games
      *don't
      ** go out of sync.
      */
      if (Session.Type == GAME_NORMAL) {
        Options.GameSpeed = static_cast<unsigned int>(gamespeed);
        Options.Save_Settings();  // save new value
      } else {
        const auto old = Options.GameSpeed;  // save orig value
        Options.GameSpeed = static_cast<unsigned int>(gamespeed);
        Options.Save_Settings();  // save new value
        Options.GameSpeed = old;  // restore old value
      }

      /*
      **	Possibly launch into another dialog if so directed.
      */
      switch (selection) {
        case kButtonVisual:
          VisualControlsClass::Process();
          process = true;
          display = true;
          refresh = true;
          break;

        case kButtonSound:
          if (!Audio.is_open()) {
            WWMessageBox().Process(Text_String(TXT_NO_SOUND_CARD));
            process = true;
            display = true;
            refresh = true;
          } else {
            SoundControlsClass::Process();
            process = true;
            display = true;
            refresh = true;
          }
          break;

        case kButtonWolapi:
          if (WOL_Options_Dialog(pWolapi, true)) {
            //	The game ended while in this dialog.
            process = false;
          } else {
            process = true;
            display = true;
            refresh = true;
          }
          break;

        case kButtonOk:
        default:
          break;
      }

      pressed = false;
    }
  }
}
