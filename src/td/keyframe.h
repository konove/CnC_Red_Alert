#ifndef CNC_RED_ALERT_TD_KEYFRAME_H_
#define CNC_RED_ALERT_TD_KEYFRAME_H_

#include <cstdint>

#include "absl/base/attributes.h"

int Get_Last_Frame_Length();
void* Build_Frame(const void* dataptr, uint16_t framenumber,
                  void* buffptr ABSL_ATTRIBUTE_LIFETIME_BOUND);
uint16_t Get_Build_Frame_Count(const void* dataptr);
uint16_t Get_Build_Frame_X(const void* dataptr);
uint16_t Get_Build_Frame_Y(const void* dataptr);
uint16_t Get_Build_Frame_Width(const void* dataptr);
uint16_t Get_Build_Frame_Height(const void* dataptr);
bool Get_Build_Frame_Palette(const void* dataptr, void* palette);

void Check_Use_Compressed_Shapes();
void Disable_Uncompressed_Shapes();
void Enable_Uncompressed_Shapes();
void Reallocate_Big_Shape_Buffer();
void* Get_Shape_Header_Data(void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND);

#endif  // CNC_RED_ALERT_TD_KEYFRAME_H_
