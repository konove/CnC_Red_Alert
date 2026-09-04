#ifndef CNC_RED_ALERT_PORT_EX_STRING_H_
#define CNC_RED_ALERT_PORT_EX_STRING_H_

#include <cstdio>

#define _MAX_PATH 260
#define _MAX_FNAME 256
#define _MAX_EXT 256
#define _MAX_DRIVE 3

// The Microsoft CRT ships all of these; redeclaring them with C++ linkage is
// an error there, so the portable versions exist only on other platforms.
#ifdef _WIN32
#include <string.h>  // IWYU pragma: keep
#else
// case-insensitive comparisons
int stricmp(const char* string1, const char* string2);
int strnicmp(const char* string1, const char* string2, std::size_t count);
int memicmp(const void* buffer1, const void* buffer2, std::size_t count);

// in-place modification
char* strupr(char* str);
char* strlwr(char* str);
char* strrev(char* str);
#endif  // _WIN32

#endif  // CNC_RED_ALERT_PORT_EX_STRING_H_
