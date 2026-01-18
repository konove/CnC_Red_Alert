#ifndef CNC_RED_ALERT_RA_MPLAYER_H_
#define CNC_RED_ALERT_RA_MPLAYER_H_

#include "ra/defines.h"
#include "ra/list.h"
#include "ra/session.h"
#include "ra/vector_dynamic.h"

GameType Select_MPlayer_Game();
void Clear_Listbox(ListClass* list);
void Clear_Vector(DynamicVectorClass<NodeNameType*>* vector);
void Computer_Message();
int Surrender_Dialog(int text);
// Stalemate games.
int Surrender_Dialog(const char* text);
bool Determine_If_Using_DVD();
bool Using_DVD();
int Abort_Dialog();

#if (MPATH)
int Read_MPATH_Game_Options();
#endif  // MPATH

#if (TEN)
int Read_TEN_Game_Options();
#endif  // TEN

#endif  // CNC_RED_ALERT_RA_MPLAYER_H_
