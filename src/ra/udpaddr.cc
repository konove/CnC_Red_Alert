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

#include <cstdio>
#include <string>

#include "ra/_wsproto.h"
#include "ra/ccfile.h"
#include "ra/ccini.h"
#include "ra/colrlist.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/init.h"
#include "ra/jshell.h"
#include "ra/palette.h"
#include "ra/textbtn.h"
#include "ra/wsproto.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"

bool Get_Broadcast_Addresses() {
  int d_dialog_w = 640;                     // dialog width
  int d_dialog_h = 320;                     // dialog height
  int d_dialog_x = (640 - d_dialog_w) / 2;  // dialog x-coord
  int d_dialog_y = (400 - d_dialog_h) / 2;  // centered y-coord
  int d_dialog_cx = d_dialog_x + d_dialog_w / 2;        // center x-coord

  int d_margin2 = 14;  // small margin

  int d_ip_address_list_w = 600;
  int d_ip_address_list_h = (20 * 6 + 3) * 2;  // 6 rows high
  int d_ip_address_list_x = d_dialog_cx - d_ip_address_list_w / 2;
  int d_ip_address_list_y = d_margin2 + d_dialog_y;

  int d_ok_w = 80;
  int d_ok_h = 18;
  int d_ok_x = d_dialog_cx + d_dialog_w / 4;
  int d_ok_y = d_dialog_y + d_dialog_h - 40;

#if (GERMAN | FRENCH)
  int d_cancel_w = 100;
#else
  int d_cancel_w = 80;
#endif
  int d_cancel_h = 18;
  int d_cancel_x = d_dialog_cx - d_dialog_w / 4;
  int d_cancel_y = d_dialog_y + d_dialog_h - 40;

  //------------------------------------------------------------------------
  //	Button Enumerations
  //------------------------------------------------------------------------
  enum {
    BUTTON_IPLIST = 100,
    BUTTON_OK,
    BUTTON_CANCEL,
  };

  //------------------------------------------------------------------------
  //	Redraw values: in order from "top" to "bottom" layer of the dialog
  //------------------------------------------------------------------------
  typedef enum {
    REDRAW_NONE = 0,
    REDRAW_PARMS,
    REDRAW_BUTTONS,
    REDRAW_BACKGROUND,
    REDRAW_ALL = REDRAW_BACKGROUND
  } RedrawType;

  //------------------------------------------------------------------------
  //	Dialog variables
  //------------------------------------------------------------------------
  RedrawType display = REDRAW_ALL;  // redraw level
  bool process = true;              // process while true
  KeyNumType input;

  int width;
  int height;

  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();

  Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, TBLACK,
                   TPF_6PT_GRAD | TPF_NOSHADOW);
  // Format_Window_String rewrites the buffer in place, so the title cannot
  // be a string literal.
  char title[] = "IP Addresses";
  Format_Window_String(title, SeenBuff.Get_Height(), width, height);

  GadgetClass* commands = nullptr;  // button list
  ColorListClass ip_address_list(
      BUTTON_IPLIST, d_ip_address_list_x, d_ip_address_list_y,
      d_ip_address_list_w, d_ip_address_list_h, kTpfText,
      MFCD::Retrieve("BTN-UP.SHP"), MFCD::Retrieve("BTN-DN.SHP"));

  TextButtonClass okbtn(BUTTON_OK, TXT_OK, kTpfButton, d_ok_x, d_ok_y, d_ok_w,
                        d_ok_h);
  TextButtonClass cancelbtn(BUTTON_CANCEL, TXT_CANCEL, kTpfButton, d_cancel_x,
                            d_cancel_y, d_cancel_w, d_cancel_h);

  ip_address_list.Set_Selected_Style(ColorListClass::SELECT_NORMAL);

  Fancy_Text_Print("", 0, 0, scheme, TBLACK, TPF_CENTER | kTpfText);

  Load_Title_Page(true);
  CCPalette.Set();  // GamePalette.Set();

  /*
  ** Add all the ip addresses from the ini file to the list box.
  */
  CCINIClass ip_ini;
  int res = 0;

  CCFileClass fc("IP.INI");
  if (ip_ini.Load(fc, false)) {
    int entry = 0;
    char entry_name[16];
    do {
      entry++;
      char* temp = new char[128];
      sprintf(entry_name, "%d", entry);
      res = ip_ini.Get_String("IP_ADDRESSES", entry_name, "", temp, 128);
      if (res) {
        ip_address_list.Add_Item(temp);
        char debug[128];
        sprintf(debug, "RA95 - Adding address %s\n", temp);
        WWDebugString(debug);
      }
    } while (res);
  }
  ip_address_list.Flag_To_Redraw();

  //------------------------------------------------------------------------
  //	Processing loop
  //------------------------------------------------------------------------
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }
    //.....................................................................
    //	Refresh display if needed
    //.....................................................................
    if (display) {
      Hide_Mouse();

      //..................................................................
      //	Redraw backgound & dialog box
      //..................................................................
      if (display >= REDRAW_BACKGROUND) {
        Load_Title_Page(true);
        CCPalette.Set();  // GamePalette.Set();
        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

        //...............................................................
        //	Dialog & Field labels
        //...............................................................
        Fancy_Text_Print("IP Addresses", d_dialog_cx - width / 2,
                         d_dialog_y + 50, scheme, TBLACK, kTpfText);

        //...............................................................
        //	Rebuild the button list
        //...............................................................
        okbtn.Zap();
        cancelbtn.Zap();
        ip_address_list.Zap();

        commands = &okbtn;
        cancelbtn.Add_Tail(*commands);
        ip_address_list.Add_Tail(*commands);
      }

      //..................................................................
      //	Redraw buttons
      //..................................................................
      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All();
      }

      Show_Mouse();
      display = REDRAW_NONE;
    }

    //.....................................................................
    //	Get user input
    //.....................................................................
    input = commands->Input();

    //.....................................................................
    //	Process input
    //.....................................................................
    switch (input) {
      //..................................................................
      // ESC / CANCEL: send a SIGN_OFF
      // - If we're part of a game, stay in this dialog; otherwise, exit
      //..................................................................
      case KN_ESC:
      case ButtonKey(BUTTON_CANCEL):
        return false;

      case ButtonKey(BUTTON_OK):
        process = false;
        break;
    }
  }

  //------------------------------------------------------------------------
  //	Restore screen
  //------------------------------------------------------------------------
  Hide_Mouse();
  Load_Title_Page(true);
  CCPalette.Set();  // GamePalette.Set();
  Show_Mouse();

  for (int i = 0; i < ip_address_list.Count(); i++) {
    std::string addr = ip_address_list.Get_Item(i);
    size_t hash_pos = addr.find('#');
    if (hash_pos != std::string::npos) {
      addr.resize(hash_pos);
    }
    PacketTransport->Set_Broadcast_Address(addr.data());
  }

  return true;
}
