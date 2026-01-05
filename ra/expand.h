#ifndef CNC_RED_ALERT_RA_EXPAND_H_
#define CNC_RED_ALERT_RA_EXPAND_H_

#include "ra/defines.h"

bool Expansion_Present(void);
bool Expansion_Dialog(void);
bool Expansion_CS_Present(void);
#ifdef FIXIT_CSII  //	checked - ajw 9/28/98
bool Expansion_AM_Present(void);
#endif

#endif  // CNC_RED_ALERT_RA_EXPAND_H_
