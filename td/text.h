#ifndef CNC_RED_ALERT_TD_TEXT_H_
#define CNC_RED_ALERT_TD_TEXT_H_

#include "td/conquer.h"

extern char const *SystemStrings;
char *Extract_String(void const *data, int string);

inline char const *Text_String(int string) {
  // can't find a conquer.eng that contains these
  switch (string) {
    case TXT_READING_IMAGE_DATA:
      return "READING IMAGE DATA";
    case TXT_ANALYZING:
      return "ANALYZING";
    case TXT_ENHANCING_IMAGE_DATA:
      return "ENHANCING IMAGE DATA";
    case TXT_ISOLATING_OPERATIONAL_THEATER:
      return "ISOLATING OPERATIONAL THEATER";
    case TXT_ESTABLISHING_TRADITIONAL_BOUNDARIES:
      return "ESTABLISHING TRADITIONAL BOUNDARIES";
    case TXT_FOR_VISUAL_REFERENCE:
      return "FOR VISUAL REFERENCE";
    case TXT_ENHANCING_IMAGE:
      return "ENHANCING IMAGE";
    case TXT_BONUS_MISSIONS:
      return "Bonus Missions";
    case TXT_BONUS_MISSION_1:
      return "Bonus Mission 1";
    case TXT_BONUS_MISSION_2:
      return "Bonus Mission 2";
    case TXT_BONUS_MISSION_3:
      return "Bonus Mission 3";
    case TXT_BONUS_MISSION_4:
      return "Bonus Mission 4";
    case TXT_BONUS_MISSION_5:
      return "Bonus Mission 5";
  }
  return (Extract_String(SystemStrings, string));
}

#endif  // CNC_RED_ALERT_TD_TEXT_H_
