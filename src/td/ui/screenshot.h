// File: Screenshot_Key(), the screenshot on K the gadget list offers every
// key to.

#ifndef CNC_RED_ALERT_TD_UI_SCREENSHOT_H_
#define CNC_RED_ALERT_TD_UI_SCREENSHOT_H_

#include "engine/window/keyboard.h"

namespace td {

// For game::Gadget::SetKeyHook(): on K, in a build with the scenario
// editor, saves the visible page as the first free scrshtNN.pcx.
void Screenshot_Key(engine::window::KeyNumber key);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_UI_SCREENSHOT_H_
