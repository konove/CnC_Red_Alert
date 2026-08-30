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

// Grows the big shape buffer once it is nearly full, so subsequent shapes
// have somewhere to be uncompressed into. Existing shapes stay valid: they
// are addressed as offsets from the buffer base, not as raw pointers.
void Reallocate_Big_Shape_Buffer();

// Returns the pixel data for a shape allocated in the big shape buffer.
// Shapes stored there keep a header whose shape_data is an offset from the
// buffer base, so the pointer has to be rebased before it can be read. When
// the big shape buffer is disabled the shape is its own data, and `ptr` is
// returned unchanged.
void* Get_Shape_Header_Data(void* ptr);

#endif  // CNC_RED_ALERT_RA_KEYFRAME_H_
