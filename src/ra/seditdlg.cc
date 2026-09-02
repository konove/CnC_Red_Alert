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

//	SEditDlg.cpp - "SimpleEditDlgClass": An ok/cancel type dialog with 1 or
// 2 edit boxes. 					Mostly a hack for what I
// need right now - not necessarily very flexible.
// Still - I can't believe there isn't a set of dialog classes in here already.
// ajw 07/21/98

#include "ra/seditdlg.h"

#include <algorithm>

#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/inline.h"
#include "ra/msgbox.h"
#include "ra/textbtn.h"
#include "ra/woledit.h"
#include "sdllib/font.h"
#include "sdllib/ww_mouse.h"

bool disable_current_msgbox = false;

//***********************************************************************************************
SimpleEditDlgClass::SimpleEditDlgClass(int dialog_width, const char* title,
                                       const char* prompt, int chars_accept,
                                       const char* prompt2 /* = NULL */,
                                       int chars_accept2 /* = 0 */)
    : iDialogWidth(dialog_width),
      szTitle(title != nullptr ? title : ""),
      szPrompt(prompt != nullptr ? prompt : ""),
      iEditCharsAccept(chars_accept),
      szPrompt2(prompt2 != nullptr ? prompt2 : ""),
      iEditCharsAccept2(chars_accept2) {
  *szEdit = 0;
  *szEdit2 = 0;

  szOkButton = Text_String(TXT_OK);
  szCancelButton = Text_String(TXT_CANCEL);
  szMiddleButton = nullptr;
}

//***********************************************************************************************
//***********************************************************************************************
//	Out of line so the vtable lands in this translation unit.
SimpleEditDlgClass::~SimpleEditDlgClass() = default;

//***********************************************************************************************
void SimpleEditDlgClass::SetButtons(const char* szOk, const char* szCancel,
                                    const char* szMiddle /*= NULL*/) {
  szOkButton = szOk;
  szCancelButton = szCancel;
  szMiddleButton = szMiddle;
}

//***********************************************************************************************
const char* SimpleEditDlgClass::Show() {
  //	Shows dialog, returns text of button pressed.
  //	Unless SetButtons() is used, value will be TXT_OK or TXT_CANCEL string
  // values.

  bool bEscapeDown = false;
  bool bReturnDown = false;

  /*
  **	Dialog & button dimensions
  */
  int x_margin = 36;  // margin width/height
  int y_margin = 20;  // margin width/height
  int d_gap_y = 10;

  int d_dialog_w = iDialogWidth;
  int d_dialog_h = !szPrompt2.empty() ? 58 + 2 * d_gap_y + 2 * y_margin
                                      : 38 + d_gap_y + 2 * y_margin;
  if (!szTitle.empty()) {
    d_dialog_h += 20 + 2 * d_gap_y;
  }
  int d_dialog_x = ((640 - d_dialog_w) / 2);
  int d_dialog_y = ((400 - d_dialog_h) / 2);
  int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // coord of x-center

  /*
          if( szTitle )
          {
                  d_title_w = String_Pixel_Width( szTitle );
                  d_title_h = 20;
                  d_title_x = d_dialog_cx - d_title_w / 2;
                  d_title_y = d_dialog_y + d_gap_y;
          }
  */

  int d_prompt_w = String_Pixel_Width(szPrompt.c_str());
  int d_prompt_x = d_dialog_x + x_margin;
  int d_prompt_y = !szTitle.empty() ? (d_dialog_y + 3 * d_gap_y + 20)
                                    : (d_dialog_y + d_gap_y);

  int d_edit_w = d_dialog_w - d_prompt_w - 2 * x_margin;
  int d_edit_x = d_dialog_x + d_prompt_w + x_margin;
  int d_edit_y = d_prompt_y;

  int d_prompt2_w =
      !szPrompt2.empty() ? String_Pixel_Width(szPrompt2.c_str()) : 0;
  int d_prompt2_h = 20;
  int d_prompt2_x = d_dialog_x + x_margin;
  int d_prompt2_y = d_prompt_y + d_prompt2_h + d_gap_y;

  int d_edit2_w = d_dialog_w - d_prompt2_w - 2 * x_margin;
  int d_edit2_x = d_dialog_x + d_prompt2_w + x_margin;
  int d_edit2_y = d_prompt2_y;

  const int d_ok_w = 80;
  const int d_ok_h = 18;
  const int d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - y_margin;
  const int d_cancel_w = 80;
  const int d_cancel_y = d_ok_y;

  //	The middle button, when there is one, sits between the other two, which
  //	move further apart to make room. MiddleBtn is built either way, so its
  //	geometry has to be defined either way.
  const bool bHasMiddle = szMiddleButton != nullptr;
  const int d_ok_x = d_dialog_cx - d_ok_w - (bHasMiddle ? 60 : 20);
  const int d_cancel_x = d_dialog_cx + (bHasMiddle ? 60 : 20);
  const int d_mid_w = 80;
  const int d_mid_x = d_dialog_cx - (d_mid_w / 2);
  const int d_mid_y = d_ok_y;

  /*
  **	Button enumerations
  */
  enum {
    BUTTON_OK = 100,
    BUTTON_CANCEL,
    BUTTON_MIDDLE,
    BUTTON_EDIT,
    BUTTON_EDIT2
  };

  /*
  **	Dialog variables
  */
  const char* szReturn = nullptr;

  /*
  **	Buttons
  */
  ControlClass* commands = nullptr;  // the button list

  TextButtonClass OkBtn(BUTTON_OK, szOkButton, kTpfButton, d_ok_x, d_ok_y,
                        d_ok_w);
  TextButtonClass CancelBtn(BUTTON_CANCEL, szCancelButton, kTpfButton,
                            d_cancel_x, d_cancel_y, d_cancel_w);
  TextButtonClass MiddleBtn(BUTTON_MIDDLE, szMiddleButton, kTpfButton, d_mid_x,
                            d_mid_y, d_mid_w);

  WOLEditClass EditBox(
      BUTTON_EDIT, szEdit,
      std::min(static_cast<int>(sizeof(szEdit)), iEditCharsAccept),
      TPF_6PT_GRAD | TPF_NOSHADOW, d_edit_x, d_edit_y, d_edit_w, -1,
      EditClass::kAlphanumeric);
  WOLEditClass EditBox2(
      BUTTON_EDIT2, szEdit2,
      std::min(static_cast<int>(sizeof(szEdit2)), iEditCharsAccept2),
      TPF_6PT_GRAD | TPF_NOSHADOW, d_edit2_x, d_edit2_y, d_edit2_w, -1,
      EditClass::kAlphanumeric);

  /*
  **	Initialize.
  */
  Set_Logic_Page(SeenBuff);

  /*
  **	Create the button list.
  */
  commands = &OkBtn;
  CancelBtn.Add_Tail(*commands);
  if (szMiddleButton) {
    MiddleBtn.Add_Tail(*commands);
  }
  EditBox.Add_Tail(*commands);
  if (!szPrompt2.empty()) {
    EditBox2.Add_Tail(*commands);
  }
  EditBox.Set_Focus();

  /*
  **	Main Processing Loop.
  */
  Keyboard->Clear();
  bool firsttime = true;
  bool display = true;
  bool process = true;
  while (process) {
    /*
    **	Invoke game callback.
    */
    Call_Back();

    /*
    **	Refresh display if needed.
    */
    if (display) {
      /*
      **	Display the dialog box.
      */
      Hide_Mouse();
      Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
      if (!szTitle.empty()) {
        Draw_Caption(szTitle.c_str(), d_dialog_x, d_dialog_y, d_dialog_w);
      }

      /*
      **	Redraw the buttons.
      */
      Fancy_Text_Print(szPrompt.c_str(), d_prompt_x, d_prompt_y,
                       GadgetClass::Get_Color_Scheme(), TBLACK, kTpfText);
      if (!szPrompt2.empty()) {
        Fancy_Text_Print(szPrompt2.c_str(), d_prompt2_x, d_prompt2_y,
                         GadgetClass::Get_Color_Scheme(), TBLACK, kTpfText);
      }
      commands->Flag_List_To_Redraw();
      Show_Mouse();
      display = false;
    }

    /*
    **	Get user input.
    */
    KeyNumType input = commands->Input();

    /*
    **	The first time through the processing loop, set the edit
    **	gadget to have the focus. The
    **	focus must be set here since the gadget list has changed
    **	and this change will cause any previous focus setting to be
    **	cleared by the input processing routine.
    */
    if (firsttime) {
      firsttime = false;
      EditBox.Set_Focus();
      EditBox.Flag_To_Redraw();
    }

    //	My hack for triggering escape and return on key up instead of down...
    //	The problem that was occurring was that the calling dialog would act on
    // the key up, 	though this dialog handled the key down. ajw
    if (Keyboard->Down(KN_ESC)) {
      bEscapeDown = true;
    } else if (bEscapeDown) {
      input = ButtonKey(BUTTON_CANCEL);
      bEscapeDown = false;
    }
    if (Keyboard->Down(KN_RETURN)) {
      bReturnDown = true;
    } else if (bReturnDown) {
      input = ButtonKey(BUTTON_OK);
      bReturnDown = false;
    }

    //	I really hate to do this, but...      ajw
    if (cancel_current_msgbox) {
      cancel_current_msgbox = false;
      input = ButtonKey(BUTTON_CANCEL);
    }

    if (disable_current_msgbox) {
      disable_current_msgbox = false;
      EditBox.Disable();
      //	These do not actually draw. I am actually clearing the "draw"
      // flag! 	Problem is Disable sets them to redraw, and I don't want to, and
      // there is no Flag_To_Redraw( false ).
      EditBox.GadgetClass::Draw_Me(true);
      if (!szPrompt2.empty()) {
        EditBox2.Disable();
        EditBox2.GadgetClass::Draw_Me(true);
      }
      OkBtn.Disable();
      OkBtn.GadgetClass::Draw_Me(true);
      CancelBtn.Disable();
      CancelBtn.GadgetClass::Draw_Me(true);
      if (szMiddleButton) {
        MiddleBtn.Disable();
        MiddleBtn.GadgetClass::Draw_Me(true);
      }
    }

    /*
    **	Process input.
    */
    switch (input) {
        //		case ( KN_ESC ):
      case ButtonKey(BUTTON_CANCEL):
        szReturn = szCancelButton;
        process = false;
        break;

        //		case KN_RETURN:
      case ButtonKey(BUTTON_EDIT):  //	(Return pressed while on edit.)
      case ButtonKey(BUTTON_OK):
        szReturn = szOkButton;
        process = false;
        break;

      case ButtonKey(BUTTON_MIDDLE):
        szReturn = szMiddleButton;
        process = false;
        break;

      default:
        break;
    }
  }

  return szReturn;
}
