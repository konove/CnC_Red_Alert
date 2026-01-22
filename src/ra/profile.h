#ifndef CNC_RED_ALERT_RA_PROFILE_H_
#define CNC_RED_ALERT_RA_PROFILE_H_

int WWGetPrivateProfileInt(char const* section, char const* entry, int def,
                           char* profile);
bool WWWritePrivateProfileInt(char const* section, char const* entry, int value,
                              char* profile);
bool WWWritePrivateProfileString(char const* section, char const* entry,
                                 char const* string, char* profile);
char* WWGetPrivateProfileString(char const* section, char const* entry,
                                char const* def, char* retbuffer, int retlen,
                                char const* profile);
unsigned WWGetPrivateProfileHex(char const* section, char const* entry,
                                char* profile);

char* Read_Bin_Buffer();
bool Read_Bin_Init(char* buffer, int length);
int Read_Bin_Length(char* buffer);
bool Read_Bin_Num(void* num, int length, char* buffer);
int Read_Bin_Pos(char* buffer);
int Read_Bin_PosSet(unsigned int pos, char* buffer);
bool Read_Bin_String(char* string, char* buffer);

char* Write_Bin_Buffer();
bool Write_Bin_Init(char* buffer, int length);
int Write_Bin_Length(char* buffer);
bool Write_Bin_Num(void* num, int length, char* buffer);
int Write_Bin_Pos(char* buffer);
int Write_Bin_PosSet(unsigned int pos, char* buffer);
bool Write_Bin_String(char* string, int length, char* buffer);

#endif  // CNC_RED_ALERT_RA_PROFILE_H_
