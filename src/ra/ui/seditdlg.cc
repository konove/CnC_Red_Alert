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

#include "ra/ui/seditdlg.h"

#include <algorithm>

#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/gadget.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/data/text.h"
#include "ra/data/text_ids.h"
#include "ra/startup/conquer.h"
#include "ra/startup/game_state.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"
#include "ra/ui/edit.h"
#include "ra/ui/textbtn.h"
#include "ra/wol/woledit.h"

namespace ra {

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
      iEditCharsAccept2(chars_accept2),
      szOkButton(TextString(TXT_OK)),
      szCancelButton(TextString(TXT_CANCEL)) {
  *szEdit = 0;
  *szEdit2 = 0;
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
  const int x_margin = 36;  // margin width/height
  const int y_margin = 20;  // margin width/height
  const int d_gap_y = 10;

  const int d_dialog_w = iDialogWidth;
  int d_dialog_h = !szPrompt2.empty() ? 58 + (2 * d_gap_y) + (2 * y_margin)
                                      : 38 + d_gap_y + (2 * y_margin);
  if (!szTitle.empty()) {
    d_dialog_h += 20 + (2 * d_gap_y);
  }
  const int d_dialog_x = ((640 - d_dialog_w) / 2);
  const int d_dialog_y = ((400 - d_dialog_h) / 2);
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // coord of x-center

  /*
          if( szTitle )
          {
                  d_title_w = StringPixelWidth( szTitle );
                  d_title_h = 20;
                  d_title_x = d_dialog_cx - d_title_w / 2;
                  d_title_y = d_dialog_y + d_gap_y;
          }
  */

  // The prompts print in kTextFont; see the Fancy_Text_Print() calls below.
  const FontStyle font = TextFontStyle(kTextFont, kTextFlags);
  const int d_prompt_w = StringPixelWidth(font, szPrompt.c_str());
  const int d_prompt_x = d_dialog_x + x_margin;
  const int d_prompt_y = !szTitle.empty() ? (d_dialog_y + (3 * d_gap_y) + 20)
                                          : (d_dialog_y + d_gap_y);

  const int d_edit_w = d_dialog_w - d_prompt_w - (2 * x_margin);
  const int d_edit_x = d_dialog_x + d_prompt_w + x_margin;
  const int d_edit_y = d_prompt_y;

  const int d_prompt2_w =
      !szPrompt2.empty() ? StringPixelWidth(font, szPrompt2.c_str()) : 0;
  const int d_prompt2_h = 20;
  const int d_prompt2_x = d_dialog_x + x_margin;
  const int d_prompt2_y = d_prompt_y + d_prompt2_h + d_gap_y;

  const int d_edit2_w = d_dialog_w - d_prompt2_w - (2 * x_margin);
  const int d_edit2_x = d_dialog_x + d_prompt2_w + x_margin;
  const int d_edit2_y = d_prompt2_y;

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
  constexpr int kButtonOk = 100;
  constexpr int kButtonCancel = 101;
  constexpr int kButtonMiddle = 102;
  constexpr int kButtonEdit = 103;
  constexpr int kButtonEdit2 = 104;

  /*
  **	Dialog variables
  */
  const char* szReturn = nullptr;

  /*
  **	Buttons
  */
  game::GadgetList commands;  // the button list

  TextButtonClass OkBtn(kButtonOk, szOkButton, kButtonFont, kButtonFlags,
                        d_ok_x, d_ok_y, d_ok_w);
  TextButtonClass CancelBtn(kButtonCancel, szCancelButton, kButtonFont,
                            kButtonFlags, d_cancel_x, d_cancel_y, d_cancel_w);
  TextButtonClass MiddleBtn(kButtonMiddle, szMiddleButton, kButtonFont,
                            kButtonFlags, d_mid_x, d_mid_y, d_mid_w);

  WOLEditClass EditBox(
      kButtonEdit, szEdit,
      std::min(static_cast<int>(sizeof(szEdit)), iEditCharsAccept),
      FontType::k6PointGradient, TPF_NOSHADOW, d_edit_x, d_edit_y, d_edit_w, -1,
      EditClass::kAlphanumeric);
  WOLEditClass EditBox2(
      kButtonEdit2, szEdit2,
      std::min(static_cast<int>(sizeof(szEdit2)), iEditCharsAccept2),
      FontType::k6PointGradient, TPF_NOSHADOW, d_edit2_x, d_edit2_y, d_edit2_w,
      -1, EditClass::kAlphanumeric);

  /*
  **	Initialize.
  */
  PixelView& view = TheScreen().visible_view();

  /*
  **	Create the button list.
  */
  commands.Add(OkBtn);
  commands.Add(CancelBtn);
  if (szMiddleButton) {
    commands.Add(MiddleBtn);
  }
  commands.Add(EditBox);
  if (!szPrompt2.empty()) {
    commands.Add(EditBox2);
  }
  EditBox.SetFocus();

  /*
  **	Main Processing Loop.
  */
  game::TheKeyboard().Clear();
  bool firsttime = true;
  bool display = true;
  bool process = true;
  while (process) {
    /*
    **	Invoke game callback.
    */
    ServiceRealTime();

    /*
    **	Refresh display if needed.
    */
    if (display) {
      /*
      **	Display the dialog box.
      */
      game::TheMouse().Hide();
      Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
      if (!szTitle.empty()) {
        Draw_Caption(view, szTitle.c_str(), d_dialog_x, d_dialog_y, d_dialog_w);
      }

      /*
      **	Redraw the buttons.
      */
      const TextPrinter prompt(
          view,
          {Get_Color_Scheme(), game::kTransparentBlack, kTextFont, kTextFlags});
      prompt.Print(szPrompt.c_str(), {d_prompt_x, d_prompt_y});
      if (!szPrompt2.empty()) {
        prompt.Print(szPrompt2.c_str(), {d_prompt2_x, d_prompt2_y});
      }
      commands.FlagToRedraw();
      game::TheMouse().Show();
      display = false;
    }

    /*
    **	Get user input.
    */
    engine::window::KeyNumber input = commands.Input(view).key;

    /*
    **	The first time through the processing loop, set the edit
    **	gadget to have the focus. The
    **	focus must be set here since the gadget list has changed
    **	and this change will cause any previous focus setting to be
    **	cleared by the input processing routine.
    */
    if (firsttime) {
      firsttime = false;
      EditBox.SetFocus();
      EditBox.FlagToRedraw();
    }

    //	My hack for triggering escape and return on key up instead of down...
    //	The problem that was occurring was that the calling dialog would act on
    // the key up, 	though this dialog handled the key down. ajw
    if (engine::window::KeyBuffer::IsDown(engine::window::KN_ESC)) {
      bEscapeDown = true;
    } else if (bEscapeDown) {
      input = engine::window::ButtonKey(kButtonCancel);
      bEscapeDown = false;
    }
    if (engine::window::KeyBuffer::IsDown(engine::window::KN_RETURN)) {
      bReturnDown = true;
    } else if (bReturnDown) {
      input = engine::window::ButtonKey(kButtonOk);
      bReturnDown = false;
    }

    //	I really hate to do this, but...      ajw
    if (TheGameState().cancel_msgbox()) {
      TheGameState().cancel_msgbox() = false;
      input = engine::window::ButtonKey(kButtonCancel);
    }

    if (TheGameState().disable_msgbox()) {
      TheGameState().disable_msgbox() = false;
      EditBox.Disable();
      //	These do not actually draw. I am actually clearing the "draw"
      // flag! 	Problem is Disable sets them to redraw, and I don't want to, and
      // there is no FlagToRedraw(false).
      EditBox.Gadget::Draw(view, true);
      if (!szPrompt2.empty()) {
        EditBox2.Disable();
        EditBox2.Gadget::Draw(view, true);
      }
      OkBtn.Disable();
      OkBtn.Gadget::Draw(view, true);
      CancelBtn.Disable();
      CancelBtn.Gadget::Draw(view, true);
      if (szMiddleButton) {
        MiddleBtn.Disable();
        MiddleBtn.Gadget::Draw(view, true);
      }
    }

    /*
    **	Process input.
    */
    switch (static_cast<int>(input)) {
        //		case ( KN_ESC ):
      case engine::window::ButtonKey(kButtonCancel):
        szReturn = szCancelButton;
        process = false;
        break;

        //		case KN_RETURN:
      case engine::window::ButtonKey(
          kButtonEdit):  //	(Return pressed while on edit.)
      case engine::window::ButtonKey(kButtonOk):
        szReturn = szOkButton;
        process = false;
        break;

      case engine::window::ButtonKey(kButtonMiddle):
        szReturn = szMiddleButton;
        process = false;
        break;

      default:
        break;
    }
  }

  return szReturn;
}

}  // namespace ra
