#include <cstddef>
#include <span>
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

/* $Header:   F:\projects\c&c\vcs\code\menus.cpv   2.17   16 Oct 1995 16:50:48
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MENUS.CPP *
 *                                                                                             *
 *                   Programmer : Phil W. Gorrow *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : May 17, 1995 [BRR] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Main_Menu -- Menu processing *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

/*****************************
**	Function prototypes
******************************/

#include "td/menus.h"

#include <algorithm>
#include <cctype>
#include <cstdint>

#include "base/array.h"
#include "base/numeric.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "sdllib/wwstd.h"
#include "td/config.h"
#include "td/conquer.h"
#include "td/control.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/expand.h"
#include "td/externs.h"
#include "td/globals.h"
#include "td/goptions.h"
#include "td/init.h"
#include "td/jshell.h"
#include "td/mapedit.h"
#include "td/textbtn.h"

static bool Coordinates_In_Region(int x, int y, int inx1, int iny1, int inx2,
                                  int iny2);
static int Select_To_Entry(int select, uint32_t bitfield, int index);
static void Flash_Line(const char* text, int xpix, int ypix, int nfgc,
                       int hfgc, int bgc);

int UnknownKey;

static int MenuUpdate = 1;
static int MenuSkip;

/*=========================================================================*/
/*	SELECT_TO_ENTRY:
 */
/*																									*/
/*		This routine converts a selection to the correct string entry.
 * It	   */
/*	does this by search through a long bitfield starting at position index
 */
/*	until it finds the correct conversion to entries.
 */
/*																									*/
/*	INPUTS:	int selection from menu, long the bit field to search, int
 */
/*				the starting index within the bit field.
 */
/*	RETURNS:	int the index into the table of entries
 */
/*=========================================================================*/
static int Select_To_Entry(int select, uint32_t bitfield, int index) {

  if (bitfield == 0xFFFFFFFFL) { /* if all bits are set	*/
    return select;               /*		then it as is		*/
  }

  // Bits past the field read as clear, as they did when the probe was a
  // 64-bit shift masked to 32 bits.
  const auto is_set = [bitfield](const int bit) {
    return bit < 32 && (bitfield & base::Bit<uint32_t>(bit)) != 0;
  };

  int placement = 0;                 /* current pos zero		*/
  while (select) {                   /* while still ones		*/
    if (is_set(placement + index)) { /* if this flagged then	*/
      select--;                      /* decrement counter		*/
    }
    placement++; /* and we moved a place	*/
  }
  while (!is_set(placement + index)) {
    placement++;
  }

  return placement; /* return the position	*/
}

/*=========================================================================*/
/*	FLASH_LINE:
 */
/*																									*/
/*		This routine will flash the line at the desired location for the
 */
/*	menu routine. It is way cool awesome!
 */
/*																									*/
/*	INPUTS:	char *text, int x position on line, int y position, char
 */
/*				normal foreground color, char hilight foreground
 * color, char 	*/
/*				background color
 */
/*	RETURNS:	none
 */
/*=========================================================================*/
static void Flash_Line(const char* text, int xpix, int ypix, int nfgc,
                       int hfgc, int bgc) {
  for (int loop = 0; loop < 3; loop++) {
    Hide_Mouse();
    Fancy_Text_Print(text, xpix, ypix, hfgc, bgc, TPF_8POINT | TPF_DROPSHADOW);
    Delay(2);
    Fancy_Text_Print(text, xpix, ypix, nfgc, bgc, TPF_8POINT | TPF_DROPSHADOW);
    Show_Mouse();
    Delay(2);
  }
}

/*=========================================================================*/
/*	COORDINATES_IN_REGION:
 */
/*																									*/
/*		Test to see if a given pair of coordinates are within the given
 */
/*	rectangular region.
 */
/*																									*/
/*	INPUTS:	int x to be tested, int y to be tested, int left x pos,
 */
/*				int top y pos, int right x pos, int bottom y pos
 */
/*	RETURNS:	none
 */
/*=========================================================================*/
static bool Coordinates_In_Region(int x, int y, int inx1, int iny1, int inx2,
                                  int iny2) {
  return ((x >= inx1) && (x <= inx2) && (y >= iny1) && (y <= iny2));
}

#ifdef NEVER
/*=========================================================================*/
/*	FIND_MENU_ITEMS:
 */
/*																									*/
/*		This routine finds the real total items in a menu when certain
 * items	*/
/*	may be disabled by bit fields and the like. This is done by looping
 */
/*	through the fields, starting at the position passed in index and
 */
/*	counting the number of bits that are set.
 */
/*																									*/
/*	INPUTS:	int the maximum number of items possible on the menu, long
 */
/*				the bit field of enabled and disabled items,
 * char the index		*/
/*				point to start at within the list.
 */
/*	RETURNS:	int the total number of items in the menu
 */
/*=========================================================================*/
int Find_Menu_Items(int maxitems, unsigned long field, char index) {
  int loop, ctr;

  if (field == 0xFFFFFFFFL) { /* if all bits are set	*/
    return (maxitems);        /* then maxitems set		*/
  }

  for (loop = ctr = 0; loop < maxitems; loop++) { /* loop through items	*/
    if (field & (1L << (loop + index))) {         /* if the bit is set		*/
      ctr++;                                      /*		count the item		*/
    }
  }
  return (ctr);
}
#endif

/*=========================================================================*/
/*	SETUP_EOB_MONITOR_MENU:
 */
/*																									*/
/*		This routine sets up the eye of the beholder monitor menu.
 */
/*																									*/
/*	INPUTS:	int the menu we are using, char *[] the array of text which
 */
/*				makes up the menu commands, long the info field,
 * int the			*/
/*				index into the field, int the number of lines to
 * skip.			*/
/*	RETURNS:	none
 */
/*=========================================================================*/
void Setup_Menu(const MenuConfig& menu, std::span<const char* const> labels,
                const uint32_t visible_items, const int bit_offset,
                const int line_spacing) {
  const int menu_x = (static_cast<int>(WinX) + menu.x) * 8;
  const int menu_y = static_cast<int>(WinY) + menu.y;

  const int selected_entry =
      Select_To_Entry(menu.selected, visible_items, bit_offset);
  const int item_count = menu.item_count;

  Fancy_Text_Print(0, 0, 0, kTBlack, kTBlack, TPF_8POINT | TPF_DROPSHADOW);
  Hide_Mouse();
  for (int i = 0; i < item_count; i++) {
    const int text_index = Select_To_Entry(i, visible_items, bit_offset);
    const int draw_y = menu_y + (i * FontHeight) + (i * line_spacing);
    Fancy_Text_Print(labels[base::ToSize(text_index)], menu_x, draw_y,
                     text_index == selected_entry && MenuUpdate
                         ? menu.highlight_color
                         : menu.normal_color,
                     kTBlack, TPF_8POINT | TPF_DROPSHADOW);
  }
  MenuSkip = line_spacing;
  Show_Mouse();
  Keyboard::Clear();
}

/*=========================================================================*/
/*	CHECK_MENU:
 */
/*																									*/
/*																									*/
/*																									*/
/*	INPUTS:
 */
/*	RETURNS:
 */
/*=========================================================================*/
int Check_Menu(MenuConfig& menu, std::span<const char* const> text,
               uint32_t field, int index) {
  int drawy = 0;
  int item = 0;
  int idx = 0;

  const int maxitem = menu.item_count - 1;            /* find max items */
  int newitem = item = menu.selected % (maxitem + 1); /* find selected */
  int select = -1;                                    /* no selection made		*/
  const int menuskip = FontHeight + MenuSkip;         /* calc new font height	*/
  const int halfskip = MenuSkip / 2;                  /* adjustment for menus	*/

  const int menuy = static_cast<int>(WinY) + menu.y; /* get the absolute */
  const int menux = (static_cast<int>(WinX) + menu.x) * 8; /* coords of menu */
  const int normcol = menu.normal_color;
  const int litcol = menu.highlight_color;

  /*
  **	Fetch a pending keystroke from the buffer if there is a keystroke
  **	present. If no keystroke is pending then simple mouse tracking will
  **	be done.
  */
  int key = 0;
  UnknownKey = 0;
  if (Keyboard::Check()) {
    key = (Keyboard::Get() & 0x18FF); /* mask off all but release bit	*/
  }

  /*
  **	if we are using the mouse and it is installed, then find the mouse
  **	coordinates of the menu and if we are not somewhere on the menu get
  **	the heck outta here. If we are somewhere on the menu, then figure
  **	out the new selected item, and continue forward.
  */
  /* get menu coords from the menu */
  const int mx1 = (static_cast<int>(WinX) * 8) + (menu.x * FontWidth);
  const int my1 = static_cast<int>(WinY) + menu.y - halfskip;
  const int mx2 = mx1 + (menu.item_width * FontWidth) -
                  1; /*		structure as		*/
  const int my2 = my1 + (menu.item_count * menuskip) -
                  1; /*		necessary			*/

  const int tempy = Get_Mouse_Y();
  if (Coordinates_In_Region(Get_Mouse_X(), tempy, mx1, my1, mx2, my2) &&
      MenuUpdate) {
    newitem = (tempy - my1) / menuskip;
  }

  switch (key) {
    case KN_UP:            /* if the key moves up	*/
      newitem--;           /* 	new item up one	*/
      if (newitem < 0) {   /* if invalid new item	*/
        newitem = maxitem; /* put at list bottom	*/
      }
      break;
    case KN_DOWN:              /* if key moves down		*/
      newitem++;               /*		new item down one	*/
      if (newitem > maxitem) { /* if new item past 		*/
        newitem = 0;           /*		list end, clear	*/
      }
      break;
    case KN_HOME:  /* if top of list key 	*/
    case KN_PGUP:  /*		is selected then	*/
      newitem = 0; /*		new item = top		*/
      break;
    case KN_END:         /* if bottom of list is	*/
    case KN_PGDN:        /*		selected then		*/
      newitem = maxitem; /*		new item = bottom	*/
      break;

    /*
    **	Handle mouse button press. Set selection and then fall into the
    **	normal menu item select logic.
    */
    case KN_RMOUSE:
    case KN_LMOUSE:
      if (Coordinates_In_Region(ActiveKeyboard->MouseQX,
                                ActiveKeyboard->MouseQY, mx1, my1, mx2, my2)) {
        newitem = (ActiveKeyboard->MouseQY - my1) / menuskip;
      } else {
        UnknownKey = key;  //	Pass the unprocessed button click back.
        break;
      }
      [[fallthrough]];

    /*
    **	Normal menu item select logic. Will flash line and exit with menu
    **	selection number.
    */
    case KN_RETURN: /* if a selection is 	*/
    case KN_SPACE:  /*		made with key		*/
    case KN_CENTER:
      select = newitem; /*		flag it made.		*/
      break;

    case 0:
      break;

    /*
    **	When no key was pressed or an unknown key was pressed, set the
    **	global record of the key and exit normally.
    **	EXCEPTION:	If the key matches the first letter of any of the
    **					menu entries, then presume it as a
    *selection of *					that entry.
    */
    default:
      for (int menu_item = 0; menu_item < menu.item_count; menu_item++) {
        if (toupper(*text[base::ToSize(
                Select_To_Entry(menu_item, field, index))]) ==
            toupper(Keyboard::To_ASCII(static_cast<KeyNumType>(key % 256)))) {
          newitem = select = menu_item;
          break;
        }
      }
      UnknownKey = key;
      break;
  }

  if (newitem != item) {
    Hide_Mouse();
    idx = Select_To_Entry(item, field, index);
    drawy = menuy + (item * menuskip);
    Fancy_Text_Print(text[base::ToSize(idx)], menux, drawy, normcol, kTBlack,
                     TPF_8POINT | TPF_DROPSHADOW);
    idx = Select_To_Entry(newitem, field, index);
    drawy = menuy + (newitem * menuskip);
    Fancy_Text_Print(text[base::ToSize(idx)], menux, drawy, litcol, kTBlack,
                     TPF_8POINT | TPF_DROPSHADOW);
    Show_Mouse(); /* resurrect the mouse	*/
  }

  if (select != -1) {
    idx = Select_To_Entry(select, field, index);
    Hide_Mouse(); /* get rid of the mouse	*/
    drawy = menuy + (newitem * menuskip);
    Flash_Line(text[base::ToSize(idx)], menux, drawy, normcol, litcol, kTBlack);
    Show_Mouse();
    select = idx;
  }

  menu.selected = newitem; /* update menu select	*/

  return select;
}

/***************************************************************************
 * Do_Menu -- Generic menu processor.                                      *
 *                                                                         *
 *    This helper function displays a menu of specified entries and waits  *
 *    for the player to make a selection. If a selection is made, then     *
 *    a whole number (starting at 0) is returned matching the entry        *
 *    selected. If ESC is pressed, then -1 is returned.                    *
 *                                                                         *
 * INPUT:   strings  -- A pointer to an array of pointers to text strings. *
 *                      Each entry in the list will be a menu entry that   *
 *                      can be selected.                                   *
 *                                                                         *
 *          blue     -- Should the special blue color be used to display   *
 *                      the menu?                                          *
 *                                                                         *
 * OUTPUT:  Returns with the cardinal number of the selected menu entry.   *
 *          If ESC was pressed, then -1 is returned.                       *
 *                                                                         *
 * WARNINGS:   none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/16/1994 JLB : Created.                                             *
 *=========================================================================*/
int Do_Menu(std::span<const char* const> strings, bool blue) {
  if (strings.empty()) {
    return (-1);
  }
  Set_Logic_Page(SeenBuff);
  Keyboard::Clear();

  /*
  **	Determine the number of entries in this string.
  */
  const auto terminator = std::ranges::find(strings, nullptr);
  strings = strings.first(static_cast<size_t>(terminator - strings.begin()));
  const int count = static_cast<int>(strings.size());
  menu_config.item_count = count;

  /*
  **	Determine the width of the menu by finding the length of the
  **	longest menu entry.
  */
  Fancy_Text_Print(TXT_NONE, 0, 0, 0, 0, TPF_8POINT | TPF_DROPSHADOW);
  int length = 0;  // The width of the menu (in pixels).
  for (const char* text : strings) {
    length = std::max(length, String_Pixel_Width(text));
  }
  length += 7;
  menu_config.item_width = length / 8;

  /*
  **	Adjust the window values to match the size of the
  **	specified menu.
  */
  base::At(base::At(WindowList, static_cast<int>(WINDOW_MENU)), kWindowWidth) =
      menu_config.item_width + 2;
  base::At(base::At(WindowList, static_cast<int>(WINDOW_MENU)), kWindowX) =
      19 - (length / 16);
  base::At(base::At(WindowList, static_cast<int>(WINDOW_MENU)), kWindowY) =
      174 - (menu_config.item_count * (FontHeight + FontYSpacing));
  base::At(base::At(WindowList, static_cast<int>(WINDOW_MENU)), kWindowHeight) =
      (menu_config.item_count * FontHeight) + 5 /*11*/;

  /*
  **	Display the menu.
  */
  Change_Window(static_cast<int>(WINDOW_MENU));
  Show_Mouse();
  Window_Box(WINDOW_MENU, blue ? BOXSTYLE_BLUE_UP : BOXSTYLE_RAISED);
  Setup_Menu(menu_config, strings, 0xFFFFL, 0, 0);

  Keyboard::Clear();
  int selection = -1;  // Selection from user.
  UnknownKey = 0;
  while (selection == -1) {
    Call_Back();
    selection = Check_Menu(menu_config, strings, 0xFFL, 0);
    // The KN_ESC/KN_LMOUSE/KN_RMOUSE tests were unreachable: any of them
    // already satisfies the != 0 in front of them, so the loop has always
    // exited on the first unrecognized key of any kind.
    if (UnknownKey != 0) {
      break;
    }
  }
  Keyboard::Clear();
  Hide_Mouse();

  HidPage.Blit(SeenBuff);
  Change_Window(static_cast<int>(WINDOW_MAIN));
  Map.Flag_To_Redraw(true);
  return selection;
}

/***************************************************************************
 * Main_Menu -- Menu processing                                            *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		index of item selected, -1 if time out
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   05/17/1995 BRR : Created.                                             *
 *=========================================================================*/
int Main_Menu(int timeout) {
  constexpr int kDialogW = 304;
  constexpr int kDialogH = 272;
  constexpr int kDialogX = 170;
  constexpr int kDialogY = 0;
  constexpr int kStartW = 250;
  constexpr int kStartH = 18;
  constexpr int kStartX = 196;
  [[maybe_unused]] constexpr int kStartY = 70;
#ifdef BONUS_MISSIONS
  constexpr int kBonusW = 250;
  constexpr int kBonusH = 18;
  constexpr int kBonusX = 196;
#endif  // BONUS_MISSIONS
  constexpr int kInternetW = 250;
  constexpr int kInternetH = 18;
  constexpr int kInternetX = 196;
  [[maybe_unused]] constexpr int kInternetY = 72;
  constexpr int kLoadW = 250;
  constexpr int kLoadH = 18;
  constexpr int kLoadX = 196;
  [[maybe_unused]] constexpr int kLoadY = 106;
  constexpr int kMultiW = 250;
  constexpr int kMultiH = 18;
  constexpr int kMultiX = 196;
  [[maybe_unused]] constexpr int kMultiY = 142;
  constexpr int kIntroW = 250;
  constexpr int kIntroH = 18;
  constexpr int kIntroX = 196;
  [[maybe_unused]] constexpr int kIntroY = 178;
#if (defined(GERMAN) || defined(FRENCH))
  constexpr int kExitW = 166;
#else
  constexpr int kExitW = 126;
#endif
  constexpr int kExitH = 18;
#if (defined(GERMAN) || defined(FRENCH))
  constexpr int kExitX = 236;
#else
  constexpr int kExitX = 256;
#endif
  [[maybe_unused]] constexpr int kExitY = 222;

#ifdef NEWMENU
  int starty = 50;
#endif

#ifdef NEWMENU
  constexpr int kButtonExpand = 100;
  constexpr int kButtonStart = kButtonExpand + 1;
#ifdef BONUS_MISSIONS
  constexpr int kButtonBonus = kButtonStart + 1;
  constexpr int kButtonInternet = kButtonBonus + 1;
#else
  constexpr int kButtonInternet = kButtonStart + 1;
#endif  // BONUS_MISSIONS
  constexpr int kButtonLoad = kButtonInternet + 1;
#else
  constexpr int kButtonStart = 100;
  constexpr int kButtonLoad = kButtonStart + 1;
#endif
  constexpr int kButtonMulti = kButtonLoad + 1;
  constexpr int kButtonIntro = kButtonMulti + 1;
  constexpr int kButtonExit = kButtonIntro + 1;

#ifdef NEWMENU
  const bool expansions = Expansion_Present();
#endif
  KeyNumType input = KN_NONE;  // input from user
  int retval = 0;    // return value
  int curbutton = 0;
#ifdef NEWMENU
#ifdef BONUS_MISSIONS
  TextButtonClass* buttons[8];
#else
  TextButtonClass* buttons[7];
#endif  // BONUS_MISSIONS
#else
  TextButtonClass* buttons[5];
#endif
  int64_t starttime = 0;

  ControlClass* commands = nullptr;  // the button list

#ifdef NEWMENU
#ifdef BONUS_MISSIONS
  int ystep = 26;
#else
  int ystep = 30;
#endif  // BONUS_MISSIONS

  if (expansions) {
    ystep -= 4;
  }
  TextButtonClass expandbtn(
      kButtonExpand, TXT_NEW_MISSIONS,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kStartX,
      starty, kStartW, kStartH);
  if (expansions) {
    starty += ystep;
  }

  TextButtonClass startbtn(
      kButtonStart, TXT_START_NEW_GAME,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kStartX,
      starty, kStartW, kStartH);
  starty += ystep;

#ifdef BONUS_MISSIONS
  TextButtonClass bonusbtn(
      kButtonBonus, TXT_BONUS_MISSIONS,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kBonusX,
      starty, kBonusW, kBonusH);
  starty += ystep;
#endif  // BONUS_MISSIONS

  TextButtonClass internetbutton(
      kButtonInternet, TXT_INTERNET,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kInternetX,
      starty, kInternetW, kInternetH);
  starty += ystep;

  TextButtonClass loadbtn(
      kButtonLoad, TXT_LOAD_MISSION,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kLoadX,
      starty, kLoadW, kLoadH);
  starty += ystep;
#else

  TextButtonClass startbtn(
      kButtonStart, TXT_START_NEW_GAME,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kStartX,
      kStartY, kStartW, kStartH);

  TextButtonClass loadbtn(
      kButtonLoad, TXT_LOAD_MISSION,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kLoadX,
      kLoadY, kLoadW, kLoadH);

#endif

#ifdef DEMO
  TextButtonClass multibtn(
      kButtonMulti, TXT_ORDER_INFO,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMultiX,
      kMultiY, kMultiW, kMultiH);
#else

#ifdef NEWMENU
  TextButtonClass multibtn(
      kButtonMulti, TXT_MULTIPLAYER_GAME,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMultiX,
      starty, kMultiW, kMultiH);
  starty += ystep;

  // TextButtonClass internetbutton(BUTTON_INTERNET, TXT_INTERNET,
  //	TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
  //	D_INTERNET_X, starty, D_INTERNET_W, D_INTERNET_H);
  // starty += ystep;
#else
  TextButtonClass multibtn(
      kButtonMulti, TXT_MULTIPLAYER_GAME,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMultiX,
      kMultiY, kMultiW, kMultiH);
#endif
#endif

#ifdef NEWMENU
#ifdef DEMO
  TextButtonClass introbtn(
      kButtonIntro, TXT_JUST_INTRO,
#else   // DEMO
  TextButtonClass introbtn(
      kButtonIntro, TXT_INTRO,
#endif  // DEMO
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kIntroX,
      starty, kIntroW, kIntroH);
  starty += ystep;

  TextButtonClass exitbtn(
      kButtonExit, TXT_EXIT_GAME,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
#if (defined(GERMAN) || defined(FRENCH))
      // D_EXIT_X, starty);
      kExitX, starty, kExitW, kExitH);
#else
      kExitX, starty, kExitW, kExitH);
#endif

#else

#ifdef DEMO
  TextButtonClass introbtn(
      kButtonIntro, TXT_JUST_INTRO,
#else   // DEMO
  TextButtonClass introbtn(
      kButtonIntro, TXT_INTRO,
#endif  // DEMO
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kIntroX,
      kIntroY, kIntroW, kIntroH);

  TextButtonClass exitbtn(
      kButtonExit, TXT_EXIT_GAME,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
#if (defined(GERMAN) || defined(FRENCH))
      // D_EXIT_X, D_EXIT_Y);
      kExitX, kExitY, kExitW, kExitH);
#else
      kExitX, kExitY, kExitW, kExitH);
#endif
#endif

  /*
  **	Initialize
  */
  Set_Logic_Page(SeenBuff);
  Keyboard::Clear();
  starttime = TickCount.Time();

  /*
  **	Create the list
  */
  commands = &startbtn;
#ifdef NEWMENU
  if (expansions) {
    expandbtn.Add_Tail(*commands);
  }
#endif
#ifdef BONUS_MISSIONS
  bonusbtn.Add_Tail(*commands);
#endif  // BONUS_MISSIONS

#ifndef DEMO
  internetbutton.Add_Tail(*commands);
#endif  // DEMO
  loadbtn.Add_Tail(*commands);
  multibtn.Add_Tail(*commands);
  introbtn.Add_Tail(*commands);
  exitbtn.Add_Tail(*commands);

  /*
  **	Fill array of button ptrs
  */
#ifdef NEWMENU
  if (expansions) {
    curbutton = 0;
  } else {
    curbutton = 1;
  }
  int butt = 0;

  base::At(buttons, butt++) = &expandbtn;
  base::At(buttons, butt++) = &startbtn;
#ifdef BONUS_MISSIONS
  base::At(buttons, butt++) = &bonusbtn;
#endif  // BONUS_MISSIONS
  base::At(buttons, butt++) = &internetbutton;
  base::At(buttons, butt++) = &loadbtn;
  base::At(buttons, butt++) = &multibtn;
  base::At(buttons, butt++) = &introbtn;
  base::At(buttons, butt++) = &exitbtn;
#else
  curbutton = 0;
  buttons[0] = &startbtn;
  buttons[1] = &loadbtn;
  buttons[2] = &multibtn;
  buttons[3] = &introbtn;
  buttons[4] = &exitbtn;
#endif
  base::At(buttons, curbutton)->Turn_On();

  Keyboard::Clear();

  Fancy_Text_Print(TXT_NONE, 0, 0, kCcGreen, kTBlack,
                   TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
  while (Get_Mouse_State() > 0) {
    Show_Mouse();
  }

  /*
  **	Main Processing Loop.
  */
  bool display = true;
  bool process = true;
  while (process) {
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
    **	If timeout expires, bail
    */
    if (timeout && TickCount.Time() - starttime > timeout) {
      retval = -1;
      process = false;
    }

    /*
    **	Invoke game callback.
    */
    Call_Back();

    /*
    **	Refresh display if needed.
    */
    if (display) {
      /*
      **	Load the background picture.
      */
      Load_Title_Page(true);

      /*
      **	Display the title and text overlay for the menu.
      */
      Set_Logic_Page(HidPage);
      Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
      Draw_Caption(TXT_NONE, kDialogX, kDialogY, kDialogW);
      if constexpr (config::kVirginCheatKeysEnabled) {
#ifdef DEMO
        Version_Number();
        Fancy_Text_Print("Demo%s", kDialogX + kDialogW - 10,
                         kDialogY + kDialogH - 20, kGrey, kTBlack,
                         TPF_6POINT | TPF_FULLSHADOW | TPF_RIGHT, VersionText);
#else
        Fancy_Text_Print("V.%d%s", kDialogX + kDialogW - 10,
                         kDialogY + kDialogH - 20, kGrey, kTBlack,
                         TPF_6POINT | TPF_FULLSHADOW | TPF_RIGHT,
                         Version_Number(), VersionText, FOREIGN_VERSION_NUMBER);
#endif
        //			Fancy_Text_Print("V.%d%s%02d",
        // D_DIALOG_X+D_DIALOG_W-5,
        // D_DIALOG_Y+D_DIALOG_H-10, GREY, TBLACK,
        // TPF_6POINT|TPF_FULLSHADOW|TPF_RIGHT, Version_Number(), VersionText,
        // FOREIGN_VERSION_NUMBER);
      } else {
#ifdef DEMO
        Version_Number();
        Fancy_Text_Print("Demo%s", kDialogX + kDialogW - 10,
                         kDialogY + kDialogH - 20, kGrey, kTBlack,
                         TPF_6POINT | TPF_FULLSHADOW | TPF_RIGHT, VersionText);
#else
        Fancy_Text_Print("V.%d%s", kDialogX + kDialogW - 10,
                         kDialogY + kDialogH - 20, kGrey, kTBlack,
                         TPF_6POINT | TPF_FULLSHADOW | TPF_RIGHT,
                         Version_Number(), VersionText);
#endif
      }

      /*
      **	Copy the menu to the visible page.
      */
      Hide_Mouse();
      HidPage.Blit(SeenBuff);
      Show_Mouse();

      Set_Logic_Page(SeenBuff);
      startbtn.Draw_All();
      display = false;
    }

    /*
    **	Get and process player input.
    */
    input = commands->Input();
    switch (static_cast<int>(input)) {
#ifdef NEWMENU
      case ButtonKey(kButtonExpand):
      case ButtonKey(kButtonInternet):
#else
#define kButtonExpand kButtonStart
#endif
      case ButtonKey(kButtonStart):
#ifdef BONUS_MISSIONS
      case ButtonKey(kButtonBonus):
#endif  // BONUS_MISSIONS
      case ButtonKey(kButtonLoad):
      case ButtonKey(kButtonMulti):
      case ButtonKey(kButtonIntro):
      case ButtonKey(kButtonExit):
        retval = (input & 0x7FFF) - kButtonExpand;
#ifdef DEMO
        // The demo shifts every button after Start up by one.
        if (input != ButtonKey(kButtonStart)) {
          retval += 1;
        }
#endif  // DEMO
        process = false;
        break;

      case KN_UP:
        base::At(buttons, curbutton)->Turn_Off();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        curbutton--;
#ifdef NEWMENU
        if (expansions) {
          if (curbutton < 0) {
            curbutton = 6;
          }
        } else {
          if (curbutton < 1) {
            curbutton = 6;
          }
        }
#else
        if (curbutton < 0) {
          curbutton = 4;
        }
#endif
        base::At(buttons, curbutton)->Turn_On();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        break;

      case KN_DOWN:
        base::At(buttons, curbutton)->Turn_Off();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        curbutton++;
#ifdef NEWMENU
        if (curbutton > 6) {
          if (expansions) {
            curbutton = 0;
          } else {
            curbutton = 1;
          }
        }
#else
        if (curbutton > 4) {
          curbutton = 0;
        }
#endif
        base::At(buttons, curbutton)->Turn_On();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        break;

      case KN_RETURN:
        base::At(buttons, curbutton)->IsPressed = true;
        base::At(buttons, curbutton)->Draw_Me(true);
        retval = curbutton;
        process = false;
        break;

      default:
        break;
    }
  }
  return retval;
}
