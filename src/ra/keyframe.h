#ifndef CNC_RED_ALERT_RA_KEYFRAME_H_
#define CNC_RED_ALERT_RA_KEYFRAME_H_

void* Build_Frame(const void* dataptr, unsigned short framenumber,
                  void* buffptr);
unsigned short Get_Build_Frame_Count(const void* dataptr);
unsigned short Get_Build_Frame_X(const void* dataptr);
unsigned short Get_Build_Frame_Y(const void* dataptr);
unsigned short Get_Build_Frame_Width(const void* dataptr);
unsigned short Get_Build_Frame_Height(const void* dataptr);
bool Get_Build_Frame_Palette(const void* dataptr, void* palette);

#endif  // CNC_RED_ALERT_RA_KEYFRAME_H_
