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

/* $Header: /CounterStrike/SPECIAL.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SPECIAL.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/27/95 *
 *                                                                                             *
 *                  Last Update : August 20, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Fetch_Difficulty -- Fetches the difficulty setting desired. *
 *   Fetch_Password -- Prompts for a password entry from client. *
 *   PWEditClass::Draw_Text -- Draws password style obscured text. *
 *   Special_Dialog -- Handles the special options dialog. * SpecialClass::Init
 *-- Initialize the special class of options.                            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/special.h"

#include <algorithm>
#include <cstring>

#include "port/safe_string.h"
#include "ra/checkbox.h"
#include "ra/conquer.h"
#include "ra/debug.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/edit.h"
#include "ra/event.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/mapedit.h"
#include "ra/queue.h"
#include "ra/rules.h"
#include "ra/session.h"
#include "ra/slider.h"
#include "ra/textbtn.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"

#ifdef WIN32
#define OPTION_WIDTH 236 * 2
#define OPTION_HEIGHT 162 * 2
#define OPTION_X ((640 - OPTION_WIDTH) / 2)
#define OPTION_Y (400 - OPTION_HEIGHT) / 2
#else
#define OPTION_WIDTH 236
#define OPTION_HEIGHT 162
#define OPTION_X ((320 - OPTION_WIDTH) / 2)
#define OPTION_Y (200 - OPTION_HEIGHT) / 2
#endif

/***********************************************************************************************
 * SpecialClass::Init -- Initialize the special class of options. *
 *                                                                                             *
 *    This initialization function is required (as opposed to using a
 *constructor) because     * the SpecialClass is declared as part of a union. A
 *union cannot have a member with a     * constructor. Other than this anomoly,
 *the function serves the same purpose as a          * normal constructor. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/20/1996 JLB : Created. *
 *=============================================================================================*/
void SpecialClass::Init() {
  IsShadowGrow = false;
  IsSpeedBuild = false;
  IsFromInstall = false;
  IsCaptureTheFlag = false;
  IsInert = false;
  IsThreePoint = false;
  IsTGrowth = true;
  IsTSpread = true;
}

/***********************************************************************************************
 * Special_Dialog -- Handles the special options dialog. *
 *                                                                                             *
 *    This dialog is used when setting the special game options. It does not
 *appear in the     * final version of the game. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/21/1995 JLB : Created. *
 *=============================================================================================*/
void Special_Dialog(bool simple) {
  SpecialClass oldspecial = Special;
  GadgetClass* buttons = nullptr;
  static struct {
    int Description;
    int Setting;
    CheckBoxClass* Button;
  } _options[] = {
      {TXT_THREE_POINT, 0, nullptr},
      {TXT_SPEED_BUILD, 0, nullptr},
  };

  TextButtonClass ok(200, TXT_OK, TPF_BUTTON, OPTION_X + 30,
                     OPTION_Y + OPTION_HEIGHT - 30);
  TextButtonClass cancel(201, TXT_CANCEL, TPF_BUTTON,
                         OPTION_X + OPTION_WIDTH - 120,
                         OPTION_Y + OPTION_HEIGHT - 30);
  buttons = &ok;
  cancel.Add(*buttons);

  for (int index = 0; index < sizeof(_options) / sizeof(_options[0]); index++) {
    _options[index].Button =
        new CheckBoxClass(100 + index, OPTION_X + 34,
                          OPTION_Y + 40 + index * 20);
    if (_options[index].Button) {
      _options[index].Button->Add(*buttons);

      bool value = false;
      switch (_options[index].Description) {
        case TXT_THREE_POINT:
          value = Special.IsThreePoint;
          break;

        case TXT_SPEED_BUILD:
          value = Special.IsSpeedBuild;
          break;
      }

      _options[index].Setting = value;
      if (value) {
        _options[index].Button->Turn_On();
      } else {
        _options[index].Button->Turn_Off();
      }
    }
  }

  Map.Override_Mouse_Shape(MOUSE_NORMAL);
  Set_Logic_Page(SeenBuff);
  bool display = true;
  bool process = true;
  while (process) {
    if (Session.Type == GAME_NORMAL) {
      Call_Back();
    } else {
      if (Main_Loop()) {
        process = false;
      }
    }

    if (display) {
      display = false;

      Hide_Mouse();
      Dialog_Box(OPTION_X, OPTION_Y, OPTION_WIDTH, OPTION_HEIGHT);
      Draw_Caption(TXT_SPECIAL_OPTIONS, OPTION_X, OPTION_Y, OPTION_WIDTH);

      for (int index = 0; index < sizeof(_options) / sizeof(_options[0]);
           index++) {
        Fancy_Text_Print(_options[index].Description,
                         _options[index].Button->X + 20,
                         _options[index].Button->Y,
                         GadgetClass::Get_Color_Scheme(), TBLACK,
                         TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }
      buttons->Draw_All();
      Show_Mouse();
    }

    KeyNumType input = buttons->Input();
    switch (input) {
      case KN_ESC:
      case 200 | KN_BUTTON:
        process = false;
        for (int index = 0; index < sizeof(_options) / sizeof(_options[0]);
             index++) {
          bool setting = _options[index].Setting;
          switch (_options[index].Description) {
            case TXT_THREE_POINT:
              oldspecial.IsThreePoint = setting;
              break;

            case TXT_SPEED_BUILD:
              oldspecial.IsSpeedBuild = setting;
              break;
          }
        }
        if (!simple) {
          OutList.Add(EventClass(oldspecial));
        } else {
          Special = oldspecial;
        }
        break;

      case 201 | KN_BUTTON:
        process = false;
        break;

      case KN_NONE:
        break;

      default:
        int index = (input & ~KN_BUTTON) - 100;
        if (static_cast<unsigned>(index) <
            sizeof(_options) / sizeof(_options[0])) {
          _options[index].Setting = _options[index].Button->IsOn;
        }
        break;
    }
  }

  if (!simple) {
    Map.Revert_Mouse_Shape();
    HidPage.Clear();
    Map.Flag_To_Redraw(true);
    Map.Render();
  }
}

/*
**	Derived from the edit class, this class allows entry of passwords style
*text *	as an edit box. This style is characterized by "*" being displayed for
*every *	real character entered.
*/
class PWEditClass : public EditClass {
 public:
  PWEditClass(int id, char* text, int max_len, TextPrintType flags, int x,
              int y, int w = -1, int h = -1)
      : EditClass(id, text, max_len, flags, x, y, w, h, ALPHANUMERIC) {}

 protected:
  void Draw_Text(const char* text) override;
};

/***********************************************************************************************
 * PWEditClass::Draw_Text -- Draws password style obscured text. *
 *                                                                                             *
 *    This routine is used by the password style edit box in order to display
 *the entered      * text. The text will be displayed as asterisks instead of
 *the actual characters the       * edit box may contain. This is necessary to
 *obscure the password entry from glancing      * eyes. *
 *                                                                                             *
 * INPUT:   text  -- Pointer to the text that is to be rendered. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/27/1995 JLB : Created. *
 *=============================================================================================*/
void PWEditClass::Draw_Text(const char* text) {
  char buffer[80];

  memset(buffer, '\0', sizeof(buffer));
  memset(buffer, '*', strlen(text));

  if (FontPtr == GradFont6Ptr) {
    TextPrintType flags;

    if (Has_Focus()) {
      flags = TPF_BRIGHT_COLOR;
    } else {
      flags = static_cast<TextPrintType>(0);
    }

    Conquer_Clip_Text_Print(buffer, X + 1, Y + 1, Color, TBLACK,
                            TextFlags | flags, Width - 2);

    if (Has_Focus() && strlen(buffer) < MaxLength) {
      Conquer_Clip_Text_Print("_", X + 1 + String_Pixel_Width(buffer), Y + 1,
                              Color, TBLACK, TextFlags | flags);
    }
  } else {
    Conquer_Clip_Text_Print(buffer, X + 1, Y + 1,
                            Has_Focus() ? &ColorRemaps[PCOLOR_DIALOG_BLUE]
                                        : &ColorRemaps[PCOLOR_GREY],
                            TBLACK, TextFlags, Width - 2);

    if (Has_Focus() && strlen(buffer) < MaxLength) {
      Conquer_Clip_Text_Print("_", X + 1 + String_Pixel_Width(buffer), Y + 1,
                              &ColorRemaps[PCOLOR_DIALOG_BLUE], TBLACK,
                              TextFlags);
    }
  }
}

/***********************************************************************************************
 * Fetch_Password -- Prompts for a password entry from client. *
 *                                                                                             *
 *    This routine will prompt for and return a password entry from the player.
 **
 *                                                                                             *
 * INPUT:   caption  -- The  caption to use for the top of the prompt dialog. *
 *                                                                                             *
 *          message  -- The body of the message to display in the dialog. *
 *                                                                                             *
 *          btext    -- The button text to use to finish the dialog box entry. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the password text entered. This pointer is
 *valid         * only until the next time that this routine is called. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/27/1995 JLB : Created. *
 *=============================================================================================*/
#define BUFFSIZE (511)
const char* Fetch_Password(int caption, int message, int btext) {
  char buffer[BUFFSIZE];
  bool process;      // loop while true
  KeyNumType input;  // user input
  TextButtonClass ok;

  if (btext == TXT_NONE) {
    btext = TXT_OK;
  }

  Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, TBLACK,
                   TPF_6PT_GRAD | TPF_NOSHADOW);

  /*
  **	Examine the optional button parameters. Fetch the width and starting
  **	characters for each.
  */
  int bwidth, bheight;  // button width and height

  /*
  **	Build the button list.
  */
  bheight = FontHeight + FontYSpacing + 4;
  bwidth = std::max(String_Pixel_Width(Text_String(btext)) + 16,
                    30u * 2);

  /*
  **	Determine the dimensions of the text to be used for the dialog box.
  **	These dimensions will control how the dialog box looks.
  */
  port::SafeCopy(buffer, Text_String(message));
  Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, TBLACK,
                   TPF_6PT_GRAD | TPF_NOSHADOW);
  int width;
  int height;
  Format_Window_String(buffer, 255, width, height);

  width = std::max(width, 100);
  width += 80;
  height += (60 + 25) * 2;

  int x = (SeenBuff.Get_Width() - width) / 2;
  int y = (SeenBuff.Get_Height() - height) / 2;

  /*
  **	Create the "ok" and password edit buttons.
  */
  TextButtonClass button1(1, btext, TPF_BUTTON, x + ((width - bwidth) >> 1),
                          y + height - (bheight + 10), bwidth);

  static char pbuffer[45];
  memset(pbuffer, '\0', sizeof(pbuffer));
  int editx = x + 52;
  int editwidth = (SeenBuff.Get_Width() / 2 - editx) * 2;
  PWEditClass button2(2, &pbuffer[0], sizeof(pbuffer),
                      TPF_6PT_GRAD | TPF_NOSHADOW, editx,
                      y + height - 70, editwidth, 20);

  TextButtonClass* buttonlist = nullptr;

  /*
  **	Add and initialize the buttons to the button list.
  */
  buttonlist = &button1;
  button2.Add(*buttonlist);

  /*
  **	Draw the background of the dialog.
  */
  Hide_Mouse();
  Set_Logic_Page(SeenBuff);
  Dialog_Box(x, y, width, height);
  Draw_Caption(caption, x, y, width);

  /*
  **	Draw the body of the message box.
  */
  Fancy_Text_Print(buffer, x + 40, y + 50,
                   GadgetClass::Get_Color_Scheme(), TBLACK,
                   TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

  /*
  **	Redraw the buttons.
  */
  if (buttonlist) {
    buttonlist->Draw_All();
  }
  Show_Mouse();

  /*
  **	Main Processing Loop.
  */
  process = true;
  bool first = true;
  while (process) {
    /*
    **	Invoke game callback.
    */
    Call_Back();

#ifdef WIN32
    /*
    ** Handle possible surface loss due to a focus switch
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      break;
    }
#endif  // WIN32

    /*
    **	Fetch and process input.
    */
    input = buttonlist->Input();
    if (first) {
      button2.Set_Focus();
      button2.Flag_To_Redraw();
      first = false;
    }
    switch (input) {
      case 1 | BUTTON_FLAG:
        process = false;
        break;

      case KN_ESC:
      case 2 | BUTTON_FLAG:
        process = false;
        break;

      case KN_RETURN:
        process = false;
        break;

      default:
        break;
    }
  }

  return pbuffer;
}

/***********************************************************************************************
 * Fetch_Difficulty -- Fetches the difficulty setting desired. *
 *                                                                                             *
 *    This will display a dialog box that requests the player to specify a
 *difficulty          * setting. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a difficulty setting of 0 for easiest and 4 for
 *hardest.              *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/13/1996 JLB : Created. *
 *=============================================================================================*/
int Fetch_Difficulty(bool amath) {
  const int w = 500;
  const int h = 160;
  const int x = 640 / 2 - w / 2;
  const int y = 400 / 2 - h / 2;
  const int bwidth = 60;

  /*
  **	Fill the description buffer with the description text. Break
  **	the text into appropriate spacing.
  */
  char buffer[512];
  port::SafeCopy(buffer, Text_String(TXT_DIFFICULTY));
  // If it's an aftermath mission, trim the sentence to get rid of the campaign
  // stuff.
  if (amath) {
    int index = 0;
    while (buffer[index] && buffer[index] != '.') {
      index++;
    }
    if (buffer[index] == '.') {
      buffer[index + 1] = 0;
    }
  }
  Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, TBLACK,
                   TPF_6PT_GRAD | TPF_NOSHADOW);
  int width;
  int height;
  Format_Window_String(buffer, w - 120, width, height);

  /*
  **	Create the OK button.
  */
  TextButtonClass okbutton(1, TXT_OK, TPF_BUTTON,
                           x + w - (bwidth + 40),
                           y + h - 36, bwidth);
  GadgetClass* buttonlist = &okbutton;

  /*
  **	Create the slider button.
  */
  SliderClass slider(2, x + 40, y + h - 58,
                     w - 80, 16, true);
  if (Rule.IsFineDifficulty) {
    slider.Set_Maximum(5);
    slider.Set_Value(2);
  } else {
    slider.Set_Maximum(3);
    slider.Set_Value(1);
  }
  slider.Add(*buttonlist);

  /*
  **	Main Processing Loop.
  */
  Set_Logic_Page(SeenBuff);
  bool redraw = true;
  bool process = true;
  while (process) {
    if (redraw) {
      redraw = false;

      /*
      **	Draw the background of the dialog.
      */
      Hide_Mouse();
      Dialog_Box(x, y, w, h);

      /*
      **	Draw the body of the message.
      */
      //			Fancy_Text_Print(buffer, x + 20*2, y +
      // 15*2, GadgetClass::Get_Color_Scheme(), TBLACK,
      // TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_NOSHADOW);
      Fancy_Text_Print(buffer, x + 40, y + 30,
                       GadgetClass::Get_Color_Scheme(), TBLACK,
                       TPF_6PT_GRAD | TPF_NOSHADOW);

      /*
      **	Display the descripton of the slider range.
      */
      Fancy_Text_Print(TXT_HARD, slider.X + slider.Width,
                       slider.Y - 18,
                       GadgetClass::Get_Color_Scheme(), TBLACK,
                       TPF_RIGHT | TPF_6PT_GRAD | TPF_DROPSHADOW);
      Fancy_Text_Print(TXT_EASY, slider.X, slider.Y - 18,
                       GadgetClass::Get_Color_Scheme(), TBLACK,
                       TPF_6PT_GRAD | TPF_DROPSHADOW);
      Fancy_Text_Print(TXT_NORMAL, slider.X + slider.Width / 2,
                       slider.Y - 18,
                       GadgetClass::Get_Color_Scheme(), TBLACK,
                       TPF_CENTER | TPF_6PT_GRAD | TPF_DROPSHADOW);

      /*
      **	Redraw the buttons.
      */
      if (buttonlist) {
        buttonlist->Draw_All();
      }
      Show_Mouse();
    }

    /*
    **	Invoke game callback.
    */
    Call_Back();

#ifdef WIN32
    /*
    ** Handle possible surface loss due to a focus switch
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      redraw = true;
      continue;
    }
#endif  // WIN32

    /*
    **	Fetch and process input.
    */
    KeyNumType input = buttonlist->Input();

    switch (input) {
      case KN_RETURN:
      case 1 | BUTTON_FLAG:
        process = false;
        break;

      default:
        break;
    }
  }

  return slider.Get_Value() * (Rule.IsFineDifficulty ? 1 : 2);
}
