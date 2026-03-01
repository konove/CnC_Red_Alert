#ifndef CNC_RED_ALERT_RA_KEYFRAME_H_
#define CNC_RED_ALERT_RA_KEYFRAME_H_

#include <cstdint>

void* Build_Frame(const void* dataptr, uint16_t framenumber,
                  void* buffptr);
uint16_t Get_Build_Frame_Count(const void* dataptr);
uint16_t Get_Build_Frame_X(const void* dataptr);
uint16_t Get_Build_Frame_Y(const void* dataptr);
uint16_t Get_Build_Frame_Width(const void* dataptr);
uint16_t Get_Build_Frame_Height(const void* dataptr);
bool Get_Build_Frame_Palette(const void* dataptr, void* palette);

#endif  // CNC_RED_ALERT_RA_KEYFRAME_H_
