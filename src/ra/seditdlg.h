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

#ifndef CNC_RED_ALERT_RA_SEDITDLG_H_
#define CNC_RED_ALERT_RA_SEDITDLG_H_

//	SEditDlg.h - "SimpleEditDlgClass": An ok/cancel type dialog with a
// single edit box. 	ajw 07/21/98

#include <string>

// The same idea as cancel_current_msgbox in ra/msgbox.h, but instead of
// closing the dialog it greys out every control on it: the dialog stays up
// with what the player typed still visible while the answer is on its way.
extern bool disable_current_msgbox;

class SimpleEditDlgClass {
 public:
  SimpleEditDlgClass(int iDialogWidth, const char* szTitle,
                     const char* szPrompt, int iEditCharsAccept,
                     const char* szPrompt2 = nullptr,
                     int iEditCharsAccept2 = 0);
  virtual ~SimpleEditDlgClass();

  const char* Show();  //	Shows dialog, returns text of button pressed.
                       //	Unless SetButtons() is used, value will be
                       // TXT_OK or TXT_CANCEL string values.

  void SetButtons(const char* szOk, const char* szCancel,
                  const char* szMiddle = nullptr);

  char szEdit[300];  //	iEditCharsAccept upper limit.
  char szEdit2[300];

 protected:
  int iDialogWidth;     //	X pixels width of entire dialog.
  std::string szTitle;  //	Title of dialog, or empty for no title.

  std::string szPrompt;  //	Text appearing to the left of edit box.
  int iEditCharsAccept;  //	Max length of string allowed in edit, includes
                         // null-terminator.

  std::string szPrompt2;  //	Empty when the dialog has only one edit box.
  int iEditCharsAccept2;

  const char* szOkButton;      //	Text of button that acts like an Ok button.
                               // Appears on left.
  const char* szCancelButton;  //	Text of button that acts like an Cancel
                               // button. Appears on right.
  const char* szMiddleButton;  //	Optional middle button text. Null = no
                               // middle button.
};

#endif  // CNC_RED_ALERT_RA_SEDITDLG_H_
