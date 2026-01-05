#ifndef CNC_RED_ALERT_RA_KEYFRAME_H_
#define CNC_RED_ALERT_RA_KEYFRAME_H_

void *Build_Frame(void const *dataptr, unsigned short framenumber,
                  void *buffptr);
unsigned short Get_Build_Frame_Count(void const *dataptr);
unsigned short Get_Build_Frame_X(void const *dataptr);
unsigned short Get_Build_Frame_Y(void const *dataptr);
unsigned short Get_Build_Frame_Width(void const *dataptr);
unsigned short Get_Build_Frame_Height(void const *dataptr);
bool Get_Build_Frame_Palette(void const *dataptr, void *palette);

#endif  // CNC_RED_ALERT_RA_KEYFRAME_H_
