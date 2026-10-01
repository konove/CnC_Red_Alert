// File: The colour scheme Red Alert's dialogs and gadgets draw with.

#ifndef CNC_RED_ALERT_RA_UI_COLOR_SCHEME_H_
#define CNC_RED_ALERT_RA_UI_COLOR_SCHEME_H_

#include "ra/core/defines.h"

namespace ra {

// Returns the current scheme: a plain white-on-black one until startup sets
// the dialog scheme. Never null.
RemapControlType* Get_Color_Scheme();

// Makes `scheme`, which must outlive its use, the current scheme.
void Set_Color_Scheme(RemapControlType* scheme);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_UI_COLOR_SCHEME_H_
