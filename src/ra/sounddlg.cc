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

/* $Header: /CounterStrike/SOUNDDLG.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SOUNDDLG.CPP *
 *                                                                                             *
 *                   Programmer : Maria del Mar McCready-Legg, Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : Jan 8, 1995 *
 *                                                                                             *
 *                  Last Update : September 22, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * MusicListClass::Draw_Entry -- Draw the score line in a list box.
 ** SoundControlsClass::Process -- Handles all the options graphic interface. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/sounddlg.h"

#include <cstdio>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/numeric.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/control.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/jshell.h"
#include "ra/list.h"
#include "ra/session.h"
#include "ra/shapebtn.h"
#include "ra/slider.h"
#include "ra/textbtn.h"
#include "ra/theme.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/fixed.h"
#include "tech/mix_archive.h"

// The score list: each line is a track, with its theme kept alongside the
// text.
class MusicListClass : public ListClass {
 public:
  MusicListClass(int id, int x, int y, int w, int h)
      : ListClass(id, x, y, w, h, TPF_6PT_GRAD | TPF_NOSHADOW,
                  MixArchive::Retrieve("BTN-UP.SHP"),
                  MixArchive::Retrieve("BTN-DN.SHP")) {}
  ~MusicListClass() override = default;
  MusicListClass(const MusicListClass&) = delete;
  MusicListClass& operator=(const MusicListClass&) = delete;
  MusicListClass(MusicListClass&&) = delete;
  MusicListClass& operator=(MusicListClass&&) = delete;

  // Appends a line for `theme`, returning its index.
  int Add_Track(ThemeType theme, const char* text) {
    Themes.push_back(theme);
    return ListClass::Add_Item(text);
  }
  // The selected line's theme, or THEME_NONE when the list is empty.
  [[nodiscard]] ThemeType Current_Theme() const {
    return Count() > 0 ? Themes[base::ToSize(Current_Index())] : THEME_NONE;
  }
  void Remove_Item(int index) override {
    if (index >= 0 && index < Count()) {
      Themes.erase(Themes.begin() + index);
      ListClass::Remove_Item(index);
    }
  }

 protected:
  void Draw_Entry(int index, int x, int y, int width, bool selected) override;

 private:
  // One per item, parallel to List.
  std::vector<ThemeType> Themes;
};

/***********************************************************************************************
 * SoundControlsClass::Process -- Handles all the options graphic interface. *
 *                                                                                             *
 *    This routine is the main control for the visual representation of the
 *options            * screen. It handles the visual overlay and the player
 *input.                              *
 *                                                                                             *
 * INPUT:      none *
 *                                                                                             *
 * OUTPUT:     none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY:    12/31/1994 MML : Created. *
 *=============================================================================================*/
void SoundControlsClass::Process() {
  /*
  ** Adjust dialog controls for resolution
  */
  const int option_width = kOptionWidth * 2;
  const int option_height = kOptionHeight * 2;

  const int option_x = kOptionX * 2;
  const int option_y = kOptionY * 2;

  const int listbox_x = kListboxX * 2;
  const int listbox_y = kListboxY * 2;
  const int listbox_w = kListboxW * 2;
  const int listbox_h = (kListboxH * 2) + 2;

  const int button_width = kButtonWidth * 2;
  const int button_x = kButtonX * 2;
  const int button_y = kButtonY * 2;

  const int stop_x = kStopX * 2;
  const int stop_y = kStopY * 2;

  const int play_x = kPlayX * 2;
  const int play_y = kPlayY * 2;

  const int onoff_width = kOnoffWidth * 2;
  const int shuffle_x = kShuffleX * 2;
  const int shuffle_y = kShuffleY * 2;
  const int repeat_x = kRepeatX * 2;
  const int repeat_y = kRepeatY * 2;

  const int mslider_x = kMsliderX * 2;
  const int mslider_y = kMsliderY * 2;
  const int mslider_w = kMsliderW * 2;
  const int mslider_height = kMsliderHeight * 2;

  const int fxslider_x = kFxsliderX * 2;
  const int fxslider_y = kFxsliderY * 2;
  const int fxslider_w = kFxsliderW * 2;
  const int fxslider_height = kFxsliderHeight * 2;

  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();
  //	ThemeType theme;

  /*
  **	List box that holds the score text strings.
  */
  MusicListClass listbox(0, option_x + listbox_x, option_y + listbox_y,
                         listbox_w, listbox_h);

  /*
  **	Return to options menu button.
  */
  TextButtonClass returnto(kButtonOptions, TXT_OK, kTpfButton,
                           option_x + button_x, option_y + button_y,
                           button_width);
  //	TextButtonClass returnto(kButtonOptions, TXT_OPTIONS_MENU, kTpfButton,

  /*
  **	Stop playing button.
  */
  ShapeButtonClass stopbtn(kButtonStop, MixArchive::Retrieve("BTN-ST.SHP"),
                           option_x + stop_x, option_y + stop_y);

  /*
  **	Start playing button.
  */
  ShapeButtonClass playbtn(kButtonPlay, MixArchive::Retrieve("BTN-PL.SHP"),
                           option_x + play_x, option_y + play_y);

  /*
  **	Shuffle control.
  */
  TextButtonClass shufflebtn(kButtonShuffle, TXT_OFF, kTpfButton,
                             option_x + shuffle_x, option_y + shuffle_y,
                             onoff_width);
  //	TextButtonClass shufflebtn(kButtonShuffle, TXT_OFF, kTpfButton,
  // option_x+shuffle_x, option_y+shuffle_y, kOnoffWidth);

  /*
  **	Repeat control.
  */
  TextButtonClass repeatbtn(kButtonRepeat, TXT_OFF, kTpfButton,
                            option_x + repeat_x, option_y + repeat_y,
                            onoff_width);

  /*
  **	Music volume slider.
  */
  SliderClass music(kSliderMusic, option_x + mslider_x, option_y + mslider_y,
                    mslider_w, mslider_height, true);

  /*
  **	Sound volume slider.
  */
  SliderClass sound(kSliderSound, option_x + fxslider_x, option_y + fxslider_y,
                    fxslider_w, fxslider_height, true);

  /*
  **	Causes left mouse clicks inside the dialog area, but not on any
  **	particular button, to be ignored.
  */
  GadgetClass area(option_x, option_y, option_width, option_height,
                   GadgetClass::kLeftPress);

  /*
  **	Causes right clicks anywhere or left clicks outside of the dialog
  **	box area to be the same a clicking the return to game options button.
  */
  ControlClass ctrl(kButtonOptions, 0, 0, SeenBuff.Get_Width(),
                    SeenBuff.Get_Height(),
                    GadgetClass::kRightPress | GadgetClass::kLeftPress);

  /*
  **	The repeat and shuffle buttons are of the toggle type. They toggle
  **	between saying "on" and "off".
  */
  shufflebtn.IsToggleType = true;
  if (Options.IsScoreShuffle) {
    shufflebtn.Turn_On();
  } else {
    shufflebtn.Turn_Off();
  }
  shufflebtn.Set_Text(shufflebtn.IsOn ? TXT_ON : TXT_OFF);

  repeatbtn.IsToggleType = true;
  if (Options.IsScoreRepeat) {
    repeatbtn.Turn_On();
  } else {
    repeatbtn.Turn_Off();
  }
  repeatbtn.Set_Text(repeatbtn.IsOn ? TXT_ON : TXT_OFF);

  /*
  **	Set the initial values of the sliders.
  */
  music.Set_Maximum(255);
  music.Set_Thumb_Size(16);
  music.Set_Value(Options.ScoreVolume * 256);
  sound.Set_Maximum(255);
  sound.Set_Thumb_Size(16);
  sound.Set_Value(Options.Volume * 256);

  /*
  **	Set up the window.  Window x-coords are in bytes not pixels.
  */
  Set_Logic_Page(SeenBuff);

  /*
  **	Create Buttons.
  */
  GadgetClass* optionsbtn = &returnto;
  listbox.Add_Tail(*optionsbtn);
  stopbtn.Add_Tail(*optionsbtn);
  playbtn.Add_Tail(*optionsbtn);
  shufflebtn.Add_Tail(*optionsbtn);
  repeatbtn.Add_Tail(*optionsbtn);
  music.Add_Tail(*optionsbtn);
  sound.Add_Tail(*optionsbtn);
  area.Add_Tail(*optionsbtn);
  ctrl.Add_Tail(*optionsbtn);

  /*
  **	Add all the themes to the list box. The list box entries are constructed
  **	and then stored into allocated EMS memory blocks.
  */
  for (const ThemeType index : magic_enum::enum_values<ThemeType>()) {
    if (ThemeClass::Is_Allowed(index)) {
      char buffer[100];
      const int length = ThemeClass::Track_Length(index);
      const char* fullname = ThemeClass::Full_Name(index);

      absl::SNPrintF(buffer, sizeof(buffer), "Track %d\t%d:%02d\t%s",
                     listbox.Count() + 1, length / 60, length % 60, fullname);
      listbox.Add_Track(index, buffer);

      if (Theme.What_Is_Playing() == index) {
        listbox.Set_Selected_Index(listbox.Count() - 1);
      }
    }
  }
  static int _tabs[] = {55 * 2, 144, 180};
  listbox.Set_Tabs(_tabs);

  /*
  **	Main Processing Loop.
  */
  bool display = true;
  bool process = true;

  while (process) {
    /*
    **	Invoke game callback.
    */
    if (Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH) {
      Call_Back();
    } else {
      if (Main_Loop()) {
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

      /*
      **	Draw the background.
      */
      Dialog_Box(option_x, option_y, option_width, option_height);

      Draw_Caption(TXT_SOUND_CONTROLS, option_x, option_y, option_width);

      /*
      ** Draw the Music, Speech & Sound titles.
      */
      Fancy_Text_Print(TXT_MUSIC_VOLUME, option_x + mslider_x - 10,
                       option_y + mslider_y - 4, scheme, kTBlack,
                       kTpfText | TPF_RIGHT);
      Fancy_Text_Print(TXT_SOUND_VOLUME, option_x + fxslider_x - 10,
                       option_y + fxslider_y - 4, scheme, kTBlack,
                       kTpfText | TPF_RIGHT);

      Fancy_Text_Print(
          TXT_SHUFFLE, option_x + shuffle_x - 10 + (config::kIsEnglish ? 0 : 4),
          option_y + shuffle_y + 2, scheme, kTBlack, kTpfText | TPF_RIGHT);
      Fancy_Text_Print(TXT_REPEAT, option_x + repeat_x - 10,
                       option_y + repeat_y + 2, scheme, kTBlack,
                       kTpfText | TPF_RIGHT);

      optionsbtn->Draw_All();
      Show_Mouse();
      display = false;
    }

    /*
    **	Get user input.
    */
    const KeyNumType input = optionsbtn->Input();

    /*
    **	Process Input.
    */
    switch (static_cast<int>(input)) {
      case KN_ESC:
      case ButtonKey(kButtonOptions):
        process = false;
        break;

      /*
      **	Control music volume.
      */
      case ButtonKey(kSliderMusic):
        Options.Set_Score_Volume(fixed(music.Get_Value(), 256), true);
        if (Session.Type != GAME_NORMAL) {
          Options.MultiScoreVolume = Options.ScoreVolume;
        }
        break;

      /*
      **	Control sound volume.
      */
      case ButtonKey(kSliderSound):
        Options.Set_Sound_Volume(fixed(sound.Get_Value(), 256), true);
        break;

      case ButtonKey(kButtonListbox):
        break;

      /*
      **	Stop all themes from playing.
      */
      case ButtonKey(kButtonStop):
        Theme.Stop();
        Theme.Queue_Song(THEME_QUIET);
        //				Theme.Queue_Song(THEME_NONE);
        break;

      /*
      **	Start the currently selected theme to play.
      */
      case KN_SPACE:
      case ButtonKey(kButtonPlay):
        Theme.Queue_Song(listbox.Current_Theme());
        break;

      /*
      **	Toggle the shuffle button.
      */
      case ButtonKey(kButtonShuffle):
        shufflebtn.Set_Text(shufflebtn.IsOn ? TXT_ON : TXT_OFF);
        Options.Set_Shuffle(shufflebtn.IsOn);
        break;

      /*
      **	Toggle the repeat button.
      */
      case ButtonKey(kButtonRepeat):
        repeatbtn.Set_Text(repeatbtn.IsOn ? TXT_ON : TXT_OFF);
        Options.Set_Repeat(repeatbtn.IsOn);
        break;
      default:
        break;
    }
  }

  /*
  **	If the score volume was turned all the way down, then actually
  **	stop the scores from being played.
  */
  if (Options.ScoreVolume == 0) {
    Theme.Stop();
  }

  /*
  **	Free the items from the list box.
  */
}

/***********************************************************************************************
 * MusicListClass::Draw_Entry -- Draw the score line in a list box. *
 *                                                                                             *
 *    This routine will display the score line in a list box. It overrides the
 *list box        * handler for line drawing. *
 *                                                                                             *
 * INPUT:   index    -- The index within the list box that is being drawn. *
 *                                                                                             *
 *          x,y      -- The pixel coordinates of the upper left position of the
 *line.          *
 *                                                                                             *
 *          width    -- The width of the line that drawing is allowed to use. *
 *                                                                                             *
 *          selected-- Is the current line selected? *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/22/1995 JLB : Created. *
 *=============================================================================================*/
void MusicListClass::Draw_Entry(int index, int x, int y, int width,
                                bool selected) {
  RemapControlType* scheme = Get_Color_Scheme();

  if (base::Any(TextFlags & TPF_6PT_GRAD)) {
    TextPrintType flags = TextFlags;

    if (selected) {
      flags = flags | TPF_BRIGHT_COLOR;
      LogicPage->Fill_Rect(x, y, x + width - 1, y + LineHeight - 1,
                           Get_Color_Scheme()->Shadow);
    } else {
      if (!base::Any(flags & TPF_USE_GRAD_PAL)) {
        flags = flags | TPF_MEDIUM_COLOR;
      }
    }

    Conquer_Clip_Text_Print(Get_Item(index), x, y, scheme, kTBlack, flags,
                            width, Tabs);

  } else {
    Conquer_Clip_Text_Print(
        Get_Item(index), x, y,
        selected ? &ColorRemaps[PCOLOR_DIALOG_BLUE] : &ColorRemaps[PCOLOR_GREY],
        kTBlack, TextFlags, width, Tabs);
  }
}
