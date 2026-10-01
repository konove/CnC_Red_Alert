// File: GaugeClass and TriColorGaugeClass, game::GaugeClass and
// game::TriColorGaugeClass drawn Red Alert's way.

#ifndef CNC_RED_ALERT_RA_UI_GAUGE_H_
#define CNC_RED_ALERT_RA_UI_GAUGE_H_

#include "game/ui/gauge.h"
#include "ra/ui/ui_skin.h"

namespace ra {

using GaugeClass = game::GaugeClass<UiSkin>;
using TriColorGaugeClass = game::TriColorGaugeClass<UiSkin>;

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_UI_GAUGE_H_
