#ifndef CNC_RED_ALERT_PORT_EX_STRING_H_
#define CNC_RED_ALERT_PORT_EX_STRING_H_

// paths
#define _MAX_PATH 260
#define _MAX_FNAME 256
#define _MAX_EXT 256
#define _MAX_DRIVE 3

// case-insensitive comparisons
int stricmp(const char *string1, const char *string2);
int strnicmp(const char *string1, const char *string2, size_t count);
int memicmp(const void *buffer1, const void *buffer2, size_t count);

// in-place modification
char *strupr(char *str);
char *strlwr(char *str);
char *strrev(char *str);

#endif  // CNC_RED_ALERT_PORT_EX_STRING_H_
