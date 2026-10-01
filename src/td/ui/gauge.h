// File: GaugeClass and TriColorGaugeClass, game::GaugeClass and
// game::TriColorGaugeClass drawn Tiberian Dawn's way.

#ifndef CNC_RED_ALERT_TD_UI_GAUGE_H_
#define CNC_RED_ALERT_TD_UI_GAUGE_H_

#include "game/ui/gauge.h"
#include "td/ui/ui_skin.h"

namespace td {

using GaugeClass = game::GaugeClass<UiSkin>;
using TriColorGaugeClass = game::TriColorGaugeClass<UiSkin>;

}  // namespace td

#endif  // CNC_RED_ALERT_TD_UI_GAUGE_H_
