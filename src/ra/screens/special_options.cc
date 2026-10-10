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

#include "ra/screens/special_options.h"

#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/core/special_flags.h"
#include "ra/data/text_ids.h"
#include "ra/debug/debug.h"
#include "ra/editor/mapedit.h"
#include "ra/map/world.h"
#include "ra/net/event.h"
#include "ra/net/network.h"
#include "ra/net/session.h"
#include "ra/startup/conquer.h"
#include "ra/ui/checkbox.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"
#include "ra/ui/textbtn.h"

namespace ra {

using enum engine::window::KeyNumber;

void RunSpecialOptionsDialog() {
  // The dialog, centred on the screen.
  constexpr int kDialogWidth = 472;
  constexpr int kDialogHeight = 324;
  constexpr int kDialogX = (Screen::kWidth - kDialogWidth) / 2;
  constexpr int kDialogY = (Screen::kHeight - kDialogHeight) / 2;

  // OK sits in the bottom left corner and Cancel in the bottom right.
  constexpr int kOkButtonX = kDialogX + 30;
  constexpr int kCancelButtonX = kDialogX + kDialogWidth - 120;
  constexpr int kButtonY = kDialogY + kDialogHeight - 30;

  // The check box, with its label to the right.
  constexpr int kCheckBoxX = kDialogX + 34;
  constexpr int kCheckBoxY = kDialogY + 40;
  constexpr int kLabelOffsetX = 20;  // From the box's left edge.

  // Gadget ids; Input() reports a clicked button as ButtonKey(id).
  constexpr int kSpeedBuildBoxId = 101;
  constexpr int kOkButtonId = 200;
  constexpr int kCancelButtonId = 201;

  // A copy the dialog edits. The installed flags change only when the event OK
  // queues is executed.
  SpecialFlags edited_flags = TheSpecialFlags();

  TextButtonClass ok_button(kOkButtonId, TXT_OK, kButtonFont, kButtonFlags,
                            kOkButtonX, kButtonY);
  TextButtonClass cancel_button(kCancelButtonId, TXT_CANCEL, kButtonFont,
                                kButtonFlags, kCancelButtonX, kButtonY);
  game::GadgetList gadgets;
  gadgets.Add(ok_button);
  gadgets.AddAfter(cancel_button, ok_button);

  // Speed build is the one flag the dialog offers. The box toggles itself
  // when clicked, so the flag is only read back from it on OK.
  CheckBoxClass speed_build_box(kSpeedBuildBoxId, kCheckBoxX, kCheckBoxY);
  gadgets.AddAfter(speed_build_box, ok_button);
  if (edited_flags.speed_build) {
    speed_build_box.Turn_On();
  }

  TheMap().Override_Mouse_Shape(MOUSE_NORMAL);
  PixelView& view = TheScreen().visible_view();
  bool redraw = true;
  bool process = true;
  while (process) {
    // A multiplayer game cannot pause for a dialog: the other players' frames
    // keep coming, so the game runs on behind it. RunFrame() returns true when
    // that game is over, which closes the dialog without applying anything.
    if (TheSession().Type == GAME_NORMAL) {
      ServiceRealTime();
    } else {
      if (RunFrame()) {
        process = false;
      }
    }

    if (redraw) {
      redraw = false;

      game::TheMouse().Hide();
      Dialog_Box(kDialogX, kDialogY, kDialogWidth, kDialogHeight);
      Draw_Caption(view, TXT_SPECIAL_OPTIONS, kDialogX, kDialogY, kDialogWidth);

      TextPrinter(view,
                  {Get_Color_Scheme(), game::kTransparentBlack,
                   FontType::k6PointGradient, TPF_USE_GRAD_PAL | TPF_NOSHADOW})
          .Print(TXT_SPEED_BUILD, {kCheckBoxX + kLabelOffsetX, kCheckBoxY});
      gadgets.DrawAll(view);
      game::TheMouse().Show();
    }

    switch (static_cast<int>(gadgets.Input(view).key)) {
      case engine::window::ButtonKey(kOkButtonId):
        process = false;
        edited_flags.speed_build = speed_build_box.IsOn;
        // Queued as an event rather than assigned, so that every machine
        // changes its flags on the same frame.
        TheNetwork().out_list().Add(EventClass(edited_flags));
        break;

      case KN_ESC:
      case engine::window::ButtonKey(kCancelButtonId):
        process = false;
        break;

      // No input, the check box (which has already toggled itself) or a key.
      default:
        break;
    }
  }

  TheMap().Revert_Mouse_Shape();
  TheScreen().hidden_view().Clear();
  TheMap().Flag_To_Redraw(true);
  TheMap().Render();
}

}  // namespace ra
