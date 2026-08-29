/*
**	Command & Conquer(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* $Header:   F:\projects\c&c\vcs\code\mapsel.cpv   1.8   16 Oct 1995 16:49:48
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MAPSEL.CPP *
 *                                                                                             *
 *                   Programmer : Barry W. Green *
 *                                                                                             *
 *                   Start Date : April 17, 1995 *
 *                                                                                             *
 *                  Last Update : April 27, 1995   [BWG] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Bit_It_In -- Pixel fade graphic copy. * Map_Selection -- Starts
 *the whole process of selecting next map to go to                  *
 *   Print_Statistics -- Prints statistics on country selected *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include <cstdio>
#include <cstring>
#include <numeric>

#include "absl/algorithm/container.h"
#include "absl/random/random.h"
#include "absl/types/span.h"
#include "sdllib/drawbuff.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/shape.h"
#include "sdllib/timer.h"
#include "sdllib/wsa.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "td/audio.h"
#include "td/ccfile.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/externs.h"
#include "td/globals.h"
#include "td/goptions.h"
#include "td/house.h"
#include "td/interpal.h"
#include "td/jshell.h"
#include "td/palette.h"
#include "td/score.h"
#include "td/text.h"
#include "td/textblit.h"
#include "td/theme.h"
#include "td/type.h"

#ifndef DEMO

void Fading_Byte_Blit(int srcx, int srcy, int destx, int desty, int w, int h,
                      GraphicBufferClass* src, GraphicBufferClass* dest);
void Print_Statistics(int country, int xpos, int ypos);
void Cycle_Call_Back_Delay(int time, unsigned char* pal);
int LowMedHiStr(int percentage);
extern int ControlQ;

unsigned char CountryRemap[256];
#ifdef OBSOLETE
const unsigned char High16Remap[256] = {
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB,
    0xFC, 0xFD, 0xFE, 0xFF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xF0, 0xF1, 0xF2, 0xF3,
    0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB,
    0xFC, 0xFD, 0xFE, 0xFF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xF0, 0xF1, 0xF2, 0xF3,
    0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB,
    0xFC, 0xFD, 0xFE, 0xFF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xF0, 0xF1, 0xF2, 0xF3,
    0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB,
    0xFC, 0xFD, 0xFE, 0xFF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xF0, 0xF1, 0xF2, 0xF3,
    0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB,
    0xFC, 0xFD, 0xFE, 0xFF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xF0, 0xF1, 0xF2, 0xF3,
    0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB,
    0xFC, 0xFD, 0xFE, 0xFF};
#endif

#define SDE SCEN_DIR_EAST
#define SDW SCEN_DIR_WEST
#define SDN SCEN_DIR_NONE
#define SVA SCEN_VAR_A
#define SVB SCEN_VAR_B
#define SVC SCEN_VAR_C
#define SVN SCEN_VAR_NONE
struct countrylist {
  int Choices[2];  // # of map choices this time - 0 = no map selection screen
  int Start[2];
  int ContAnim[2];
  int CountryColor[2][3];
  int CountryShape[2][3];  // shape in COUNTRYE.SHP
  ScenarioDirType CountryDir[2][3];
  ScenarioVarType CountryVariant[2][3];
} const CountryArray[27] = {
    // GDI SCENARIO CHOICES
    /*  0 */ {},
    /*E  W*/ /* cont */ /* East colors */ /* West color*/ /* E frame   W frame
                                                           */
                                                          /*  1 */
    {{1, 1},
     {0, 0},
     {3, 3},
     {{0x95, 0, 0}, {0x95, 0, 0}},
     {{17, 0, 0}, {17, 0, 0}},
     {{SDE, SDN, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVN, SVN}, {SVN, SVN, SVN}}},
    /*  2 */
    {{1, 1},
     {16, 16},
     {19, 19},
     {{0x80, 0, 0}, {0x80, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}},
     {{SDE, SDN, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVN, SVN}, {SVN, SVN, SVN}}},
    /*  3 */
    {{3, 3},
     {32, 32},
     {35, 35},
     {{0x81, 0x82, 0x83}, {0x81, 0x82, 0x83}},
     {{3, 3, 1}, {3, 3, 1}},
     {{SDW, SDW, SDE}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVA}, {SVN, SVN, SVN}}},
    /*  4 */
    {{2, 2},
     {48, 64},
     {51, 67},
     {{0x84, 0x85, 0}, {0x86, 0x87, 0}},
     {{4, 4, 0}, {2, 2, 0}},
     {{SDE, SDE, SDN}, {SDW, SDW, SDN}},
     {{SVA, SVA, SVN}, {SVA, SVB, SVN}}},
    /*  5 */
    {{2, 2},
     {99, 99},
     {102, 102},
     {{0x88, 0x89, 0}, {0x88, 0x89, 0}},
     {{7, 7, 0}, {7, 7, 0}},
     {{SDE, SDE, SDN}, {SDE, SDE, SDN}},
     {{SVA, SVA, SVN}, {SVA, SVA, SVN}}},
    /*  6 */
    {{2, 2},
     {80, 83},
     {86, 86},
     {{0x88, 0x89, 0}, {0x88, 0x89, 0}},
     {{7, 7, 0}, {7, 7, 0}},
     {{SDE, SDE, SDN}, {SDE, SDE, SDN}},
     {{SVA, SVA, SVN}, {SVA, SVA, SVN}}},
    /*  7 */
    {{2, 2},
     {115, 0},
     {118, 0},
     {{0x8B, 0x8A, 0}, {0x8B, 0x8A, 0}},
     {{6, 8, 0}, {6, 8, 0}},
     {{SDE, SDE, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVN}, {SVN, SVN, SVN}}},
    /*  8 */
    {{1, 1},
     {131, 0},
     {134, 0},
     {{0x8C, 0, 0}, {0x8C, 0, 0}},
     {{9, 0, 0}, {9, 0, 0}},
     {{SDE, SDN, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVN, SVN}, {SVN, SVN, SVN}}},
    /*  9 */
    {{2, 1},
     {147, 0},
     {150, 0},
     {{0x8D, 0x8E, 0}, {0, 0, 0}},
     {{10, 13, 0}, {0, 0, 0}},
     {{SDE, SDE, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVN}, {SVN, SVN, SVN}}},
    /* 10 */
    {{1, 1},
     {163, 0},
     {166, 0},
     {{0x8F, 0, 0}, {0, 0, 0}},
     {{16, 0, 0}, {0, 0, 0}},
     {{SDE, SDN, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVN, SVN}, {SVN, SVN, SVN}}},
    /* 11 */
    {{2, 1},
     {179, 0},
     {182, 0},
     {{0x90, 0x91, 0}, {0, 0, 0}},
     {{14, 15, 0}, {0, 0, 0}},
     {{SDE, SDE, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVN}, {SVN, SVN, SVN}}},
    /* 12 */
    {{2, 1},
     {195, 0},
     {198, 0},
     {{0x92, 0x93, 0}, {0, 0, 0}},
     {{12, 12, 0}, {0, 0, 0}},
     {{SDE, SDE, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVN}, {SVN, SVN, SVN}}},
    /* 13 */
    {{1, 1},
     {211, 0},
     {214, 0},
     {{0x93, 0, 0}, {0, 0, 0}},
     {{12, 0, 0}, {0, 0, 0}},
     {{SDE, SDN, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVN, SVN}, {SVN, SVN, SVN}}},
    /* 14 */
    {{3, 1},
     {0, 0},
     {3, 0},
     {{0x81, 0x82, 0x83}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}},
     {{SDE, SDE, SDE}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVC}, {SVN, SVN, SVN}}},

    // NOD SCENARIO CHOICES
    //	  		choices   E/W start continue   East colors    West
    // colors  E shape    W shape	  direction
    // variant
    /*  1 */
    {{2, 1},
     {0, 0},
     {3, 0},
     {{0x80, 0x81, 0x00}, {0, 0, 0}},
     {{4, 4, 0}, {0, 0, 0}},
     {{SDE, SDE, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVN}, {SVN, SVN, SVN}}},
    /*  2 */
    {{2, 1},
     {16, 0},
     {19, 0},
     {{0x82, 0x83, 0x00}, {0, 0, 0}},
     {{6, 6, 0}, {0, 0, 0}},
     {{SDE, SDE, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVN}, {SVN, SVN, SVN}}},
    /*  3 */
    {{2, 1},
     {32, 0},
     {35, 0},
     {{0x84, 0x85, 0x00}, {0, 0, 0}},
     {{5, 5, 0}, {0, 0, 0}},
     {{SDE, SDE, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVN}, {SVN, SVN, SVN}}},
    /*  4 */
    {{1, 1},
     {48, 0},
     {51, 0},
     {{0x86, 0x00, 0x00}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}},
     {{SDE, SDN, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVN, SVN}, {SVN, SVN, SVN}}},
    /*  5 */
    {{3, 1},
     {64, 0},
     {67, 0},
     {{0x87, 0x88, 0x89}, {0, 0, 0}},
     {{1, 2, 3}, {0, 0, 0}},
     {{SDE, SDE, SDE}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVC}, {SVN, SVN, SVN}}},
    /*  6 */
    {{3, 1},
     {80, 0},
     {83, 0},
     {{0x8A, 0x8B, 0x8C}, {0, 0, 0}},
     {{9, 7, 8}, {0, 0, 0}},
     {{SDE, SDE, SDE}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVC}, {SVN, SVN, SVN}}},
    /*  7 */
    {{2, 1},
     {96, 0},
     {99, 0},
     {{0x8D, 0x8E, 0x00}, {0, 0, 0}},
     {{10, 10, 0}, {0, 0, 0}},
     {{SDE, SDE, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVN}, {SVN, SVN, SVN}}},
    /*  8 */
    {{1, 1},
     {112, 0},
     {115, 0},
     {{0xA0, 0x00, 0x00}, {0, 0, 0}},
     {{4, 4, 0}, {0, 0, 0}},
     {{SDE, SDN, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVN, SVN}, {SVN, SVN, SVN}}},
    /*  9 */
    {{2, 1},
     {128, 0},
     {131, 0},
     {{0x8F, 0x90, 0x00}, {0, 0, 0}},
     {{11, 15, 0}, {0, 0, 0}},
     {{SDE, SDE, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVN}, {SVN, SVN, SVN}}},
    /* 10 */
    {{2, 1},
     {144, 0},
     {147, 0},
     {{0x91, 0x92, 0x00}, {0, 0, 0}},
     {{12, 16, 0}, {0, 0, 0}},
     {{SDE, SDE, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVN}, {SVN, SVN, SVN}}},
    /* 11 */
    {{1, 1},
     {160, 0},
     {163, 0},
     {{0x93, 0x00, 0x00}, {0, 0, 0}},
     {{13, 0, 0}, {0, 0, 0}},
     {{SDE, SDN, SDN}, {SDN, SDN, SDN}},
     {{SVA, SVN, SVN}, {SVN, SVN, SVN}}},
    /* 12 */
    {{3, 1},
     {0, 0},
     {3, 0},
     {{0x81, 0x82, 0x83}, {0, 0, 0}},
     {{14, 0, 0}, {0, 0, 0}},
     {{SDE, SDE, SDE}, {SDN, SDN, SDN}},
     {{SVA, SVB, SVC}, {SVN, SVN, SVN}}}};

struct gdistats {
  int nameindex;
  int pop;
  int area;
  int capital;
  int govt;
  int gdp;
  int conflict;
  int military;
} const GDIStats[] = {
    // Name   Pop        Area  	  Capital	Government     GDP Conflict
    // Military
    {0, TXT_MAP_P01, TXT_MAP_A00, TXT_MAP_C00, 0, TXT_MAP_GDP00, TXT_MAP_PC00,
     0},
    {1, TXT_MAP_P02, TXT_MAP_A01, TXT_MAP_C01, 1, TXT_MAP_GDP01, TXT_MAP_PC01,
     3},
    {1, TXT_MAP_P02, TXT_MAP_A01, TXT_MAP_C01, 1, TXT_MAP_GDP01, TXT_MAP_PC02,
     3},
    {2, TXT_MAP_P03, TXT_MAP_A02, TXT_MAP_C02, 0, TXT_MAP_GDP00, TXT_MAP_PC03,
     1},
    {3, TXT_MAP_P04, TXT_MAP_A03, TXT_MAP_C03, 3, TXT_MAP_GDP02, TXT_MAP_PC04,
     1},
    {3, TXT_MAP_P04, TXT_MAP_A03, TXT_MAP_C03, 3, TXT_MAP_GDP02, TXT_MAP_PC04,
     1},
    {4, TXT_MAP_P05, TXT_MAP_A04, TXT_MAP_C04, 2, TXT_MAP_GDP03, TXT_MAP_PC05,
     5},
    {4, TXT_MAP_P05, TXT_MAP_A04, TXT_MAP_C04, 2, TXT_MAP_GDP03, TXT_MAP_PC06,
     5},
    {5, TXT_MAP_P06, TXT_MAP_A05, TXT_MAP_C05, 0, TXT_MAP_GDP04, TXT_MAP_PC07,
     2},
    {5, TXT_MAP_P06, TXT_MAP_A05, TXT_MAP_C05, 0, TXT_MAP_GDP04, TXT_MAP_PC07,
     2},
    {6, TXT_MAP_P07, TXT_MAP_A06, TXT_MAP_C06, 0, TXT_MAP_GDP00, TXT_MAP_PC08,
     0},
    {7, TXT_MAP_P08, TXT_MAP_A07, TXT_MAP_C07, 4, TXT_MAP_GDP05, TXT_MAP_PC00,
     2},
    {8, TXT_MAP_P09, TXT_MAP_A08, TXT_MAP_C08, 4, TXT_MAP_GDP06, TXT_MAP_PC10,
     2},
    {9, TXT_MAP_P10, TXT_MAP_A09, TXT_MAP_C09, 0, TXT_MAP_GDP07, TXT_MAP_PC11,
     1},
    {10, TXT_MAP_P11, TXT_MAP_A10, TXT_MAP_C10, 0, TXT_MAP_GDP08, TXT_MAP_PC12,
     2},
    {11, TXT_MAP_P12, TXT_MAP_A11, TXT_MAP_C11, 5, TXT_MAP_GDP09, TXT_MAP_PC13,
     3},
    {12, TXT_MAP_P13, TXT_MAP_A12, TXT_MAP_C12, 6, TXT_MAP_GDP10, TXT_MAP_PC14,
     2},
    {13, TXT_MAP_P14, TXT_MAP_A13, TXT_MAP_C13, 0, TXT_MAP_GDP11, TXT_MAP_PC15,
     2},
    {14, TXT_MAP_P15, TXT_MAP_A14, TXT_MAP_C14, 0, TXT_MAP_GDP12, TXT_MAP_PC16,
     3},
    {14, TXT_MAP_P15, TXT_MAP_A14, TXT_MAP_C14, 0, TXT_MAP_GDP12, TXT_MAP_PC17,
     3},
    {15, TXT_MAP_P16, TXT_MAP_A15, TXT_MAP_C15, 7, TXT_MAP_GDP13, TXT_MAP_PC18,
     4},
    // Hack in a slot for Estonia
    {34, TXT_MAP_P17, TXT_MAP_A16, TXT_MAP_C16, 0, TXT_MAP_GDP00, TXT_MAP_PC19,
     0}};

struct nodstats {
  int nameindex;
  int pop;
  int expendable;
  int capital;
  int govt;
  int corruptible;
  int worth;
  int conflict;
  int military;
  int probability;
} const NodStats[] = {
    // Name   Pop     Expendable   Capital   Government Corruptible   Worth
    // Conflict   Military  Probability
    {16, TXT_MAP_P18, 38, TXT_MAP_C17, 8, 86, TXT_MAP_GDP14, TXT_MAP_PC20, 0,
     23},
    {17, TXT_MAP_P19, 75, TXT_MAP_C18, 0, 18, TXT_MAP_GDP15, TXT_MAP_PC21, 1,
     82},
    {17, TXT_MAP_P19, 75, TXT_MAP_C18, 0, 18, TXT_MAP_GDP15, TXT_MAP_PC22, 1,
     82},
    {18, TXT_MAP_P20, 50, TXT_MAP_C19, 9, 52, TXT_MAP_GDP16, TXT_MAP_PC23, 0,
     72},
    {18, TXT_MAP_P20, 50, TXT_MAP_C19, 9, 52, TXT_MAP_GDP16, TXT_MAP_PC24, 0,
     72},
    {19, TXT_MAP_P21, 80, TXT_MAP_C20, 0, 85, TXT_MAP_GDP17, TXT_MAP_PC25, 2,
     35},
    {19, TXT_MAP_P21, 80, TXT_MAP_C20, 0, 85, TXT_MAP_GDP17, TXT_MAP_PC26, 2,
     35},
    {20, TXT_MAP_P22, 50, TXT_MAP_C21, 10, 48, TXT_MAP_GDP17, TXT_MAP_PC27, 2,
     24},
    {21, TXT_MAP_P23, 33, TXT_MAP_C22, 0, 28, TXT_MAP_GDP18, TXT_MAP_PC28, 3,
     67},
    {22, TXT_MAP_P24, 75, TXT_MAP_C23, 6, 17, TXT_MAP_GDP19, TXT_MAP_PC29, 2,
     80},
    {23, TXT_MAP_P25, 60, TXT_MAP_C24, 7, 93, TXT_MAP_GDP20, TXT_MAP_PC30, 3,
     50},
    {24, TXT_MAP_P26, 5, TXT_MAP_C25, 0, 84, TXT_MAP_GDP21, TXT_MAP_PC31, 2,
     22},
    {25, TXT_MAP_P27, 55, TXT_MAP_C26, 0, 48, TXT_MAP_GDP22, TXT_MAP_PC32, 3,
     62},
    {26, TXT_MAP_P28, 65, TXT_MAP_C27, 0, 41, TXT_MAP_GDP23, TXT_MAP_PC33, 2,
     49},
    {27, TXT_MAP_P29, 72, TXT_MAP_C28, 0, 74, TXT_MAP_GDP24, TXT_MAP_PC34, 3,
     54},
    {27, TXT_MAP_P29, 72, TXT_MAP_C28, 0, 74, TXT_MAP_GDP24, TXT_MAP_PC35, 3,
     54},
    {17, TXT_MAP_P30, 45, TXT_MAP_C29, 6, 3, TXT_MAP_GDP15, TXT_MAP_PC36, 3,
     100},
    {28, TXT_MAP_P31, 45, TXT_MAP_C30, 0, 63, TXT_MAP_GDP25, TXT_MAP_PC37, 2,
     66},
    {29, TXT_MAP_P32, 55, TXT_MAP_C31, 0, 27, TXT_MAP_GDP26, TXT_MAP_PC38, 2,
     68},
    {30, TXT_MAP_P33, 5, TXT_MAP_C32, 0, 65, TXT_MAP_GDP27, TXT_MAP_PC39, 4,
     74},
    {31, TXT_MAP_P34, 65, TXT_MAP_C33, 0, 52, TXT_MAP_GDP19, TXT_MAP_PC40, 2,
     84},
    {32, TXT_MAP_P35, 2, TXT_MAP_C34, 11, 12, TXT_MAP_GDP28, TXT_MAP_PC41, 2,
     92},
    {33, TXT_MAP_P36, 10, TXT_MAP_C35, 0, 8, TXT_MAP_GDP29, TXT_MAP_PC42, 1,
     100}};

/***********************************************************************************************
 * Map_Selection -- Starts the whole process of selecting next map to go to *
 *                                                                                             *
 *                                                                                             *
 * INPUT: *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/17/1995 BWG : Created. *
 *=============================================================================================*/
void Map_Selection() {
  void *anim, *progress, *greyearth, *greyearth2;
  unsigned char localpalette[768];
  int scenario, lastscenario;
  int house = PlayerPtr->Class->House;
  int attackxcoord = 0;

  static const int _countryx[] = {195, 217, 115, 167, 244, 97, 130, 142, 171,
                                  170, 139, 158, 180, 207, 177, 213, 201, 198,
                                  /* Nod countries */
                                  69, 82, 105, 119, 184, 149, 187, 130, 153,
                                  124, 162, 144, 145, 164, 166, 200, 201};
  static const int _countryy[] = {35, 57, 82, 75, 93, 111, 108, 91, 100, 111,
                                  120, 136, 136, 117, 158, 143, 167, 21,
                                  /* Nod countries */
                                  45, 80, 75, 76, 31, 64, 69, 89, 88, 106, 115,
                                  139, 168, 164, 183, 123, 154};
  static const char _greenpal[] = {0,    0x41, 0x42, 0x43, 0x44, 0x44,
                                   0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
                                   0x44, 0x44, 0x44, 0x44};
  static const char _othergreenpal[] = {0,    0x21, 0x22, 0x23, 0x24, 0x25,
                                        0x26, 0x26, 0x26, 0x26, 0x26, 0x26,
                                        0x26, 0x26, 0x26, 0x26};
  static const char _regpal[] = {0, 1, 2,  3,  4,  5,  6,  7,
                                 8, 9, 10, 11, 12, 13, 14, 15};
  GraphicBufferClass backpage(20 * 6, 8);

  unsigned char* grey2palette = new unsigned char[768];
  unsigned char* progresspalette = new unsigned char[768];

  Keyboard::Clear();
  Set_Font(ScoreFontPtr);
  Set_Font_Palette(_regpal);
  Set_Palette(BlackPalette);

  scenario = Scenario + (house == HOUSE_GOOD ? 0 : 14);
  if (house == HOUSE_GOOD) {
    lastscenario = Scenario == 14;
    if (Scenario == 15) {
      delete[] progresspalette;
      delete[] grey2palette;
      return;
    }
  } else {
    lastscenario = Scenario == 12;
    if (Scenario == 13) {
      delete[] progresspalette;
      delete[] grey2palette;
      return;
    }
  }

  // Check if they're even entitled to map selection this time
  if (CountryArray[scenario].Choices[ScenDir] == 0) {
    delete[] progresspalette;
    delete[] grey2palette;
    return;
  }

  Theme.Queue_Song(THEME_MAP1);

  PseudoSeenBuff =
      new GraphicBufferClass(320, 200, static_cast<void*>(nullptr));

  /*
  ** Extra graphic buffer to draw text into
  */
  TextPrintBuffer = new GraphicBufferClass(
      SeenBuff.Get_Width(), SeenBuff.Get_Height(), static_cast<void*>(nullptr));
  TextPrintBuffer->Clear();
  BlitList.Clear();

  /*
  ** Now start the process where we fade the gray earth in.
  */
  greyearth =
      Open_Animation("GREYERTH.WSA", nullptr, 0,
                     WSA_OPEN_FROM_MEM | WSA_OPEN_TO_PAGE, localpalette);
  greyearth2 =
      Open_Animation("E-BWTOCL.WSA", nullptr, 0,
                     WSA_OPEN_FROM_MEM | WSA_OPEN_TO_PAGE, grey2palette);

  /*
  ** Load the spinning-globe anim
  */
  if (house == HOUSE_GOOD) {
    anim = Open_Animation("HEARTH_E.WSA", nullptr, 0,
                          WSA_OPEN_FROM_MEM | WSA_OPEN_TO_PAGE, Palette);
    progress =
        Open_Animation(lastscenario ? "HBOSNIA.WSA" : "EUROPE.WSA", nullptr, 0,
                       WSA_OPEN_FROM_MEM | WSA_OPEN_TO_PAGE, progresspalette);
  } else {
    anim = Open_Animation("HEARTH_A.WSA", nullptr, 0,
                          WSA_OPEN_FROM_MEM | WSA_OPEN_TO_PAGE, Palette);
    progress =
        Open_Animation(lastscenario ? "HSAFRICA.WSA" : "AFRICA.WSA", nullptr, 0,
                       WSA_OPEN_FROM_MEM | WSA_OPEN_TO_PAGE, progresspalette);
  }

  const void* appear1 = MFCD::Retrieve("APPEAR1.AUD");
  const void* sfx4 = MFCD::Retrieve("SFX4.AUD");
  const void* text2 = MFCD::Retrieve("TEXT2.AUD");
  const void* target1 = MFCD::Retrieve("TARGET1.AUD");
  const void* target2 = MFCD::Retrieve("TARGET2.AUD");
  //	void const * target3 = MFCD::Retrieve("TARGET3.AUD");
  const void* newtarg1 = MFCD::Retrieve("NEWTARG1.AUD");
  const void* beepy2 = MFCD::Retrieve("BEEPY2.AUD");
  const void* beepy3 = MFCD::Retrieve("BEEPY3.AUD");
  const void* beepy6 = MFCD::Retrieve("BEEPY6.AUD");
  const void* world2 = MFCD::Retrieve("WORLD2.AUD");
  const void* country1 = MFCD::Retrieve("COUNTRY1.AUD");
  const void* scold1 = MFCD::Retrieve("SCOLD1.AUD");

  SysMemPage.Clear();
  PseudoSeenBuff->Clear();
  WWMouse->Erase_Mouse(&HidPage, true);
  HiddenPage.Clear();

  InterpolationPaletteChanged = true;
  InterpolationPalette = Palette;
  Increase_Palette_Luminance(InterpolationPalette, 30, 30, 30, 63);
  Read_Interpolation_Palette("MAP1.PAL");

  //	SeenBuff.Blit(HidPage);
  Animate_Frame(greyearth, SysMemPage, 0);

  Bit_It_In(0, 0, 320, 200, &SysMemPage, PseudoSeenBuff);
  PseudoSeenBuff->Put_Pixel(237, 92, TBLACK);
  PseudoSeenBuff->Put_Pixel(237, 93, TBLACK);

  Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, "MAP1.PAL");

  InterpolationPaletteChanged = true;
  InterpolationPalette = localpalette;
  Increase_Palette_Luminance(InterpolationPalette, 30, 30, 30, 63);
  Read_Interpolation_Palette("MAP_LOCL.PAL");

  Play_Sample(appear1, 255, Options.Normalize_Sound(110));
  Fade_Palette_To(localpalette, kFadePaletteMedium, Call_Back);
  for (int i = 1; i < Get_Animation_Frame_Count(greyearth); i++) {
    Call_Back_Delay(4);
    Animate_Frame(greyearth, *PseudoSeenBuff, i);
  }
  Close_Animation(greyearth);

  Write_Interpolation_Palette("MAP_LOCL.PAL");

  Call_Back_Delay(4);

  SysMemPage.Clear();
  Animate_Frame(greyearth2, SysMemPage, 0);

  InterpolationPaletteChanged = true;
  InterpolationPalette = grey2palette;
  Increase_Palette_Luminance(InterpolationPalette, 30, 30, 30, 63);
  Read_Interpolation_Palette("MAP_GRY2.PAL");
  Wait_Vert_Blank();
  Set_Palette(grey2palette);

  SysMemPage.Blit(*PseudoSeenBuff);

  Call_Back_Delay(4);
  for (int i = 1; i < Get_Animation_Frame_Count(greyearth2); i++) {
    Animate_Frame(greyearth2, *PseudoSeenBuff, i);
    Call_Back_Delay(4);
  }
  Close_Animation(greyearth2);

  Write_Interpolation_Palette("MAP_GRY2.PAL");

  /*
  ** Copy the first frame up to the seenpage (while screen is black)
  */
  SysMemPage.Clear();
  Animate_Frame(anim, SysMemPage, 1);  //, 0,0, (WSAType)0,0,0);
  SysMemPage.Blit(*PseudoSeenBuff);

  Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, nullptr);

  Stop_Speaking();

  while (CountDownTimer.Time() || Is_Speaking()) {
    Call_Back();
    //		if (Keyboard::Check()) CountDownTimer.Set(0);
  }

  //	Keyboard::Clear();

  /*
  ** now make the grid appear
  */
  SysMemPage.Blit(*PseudoSeenBuff);
  Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, nullptr);

  Play_Sample(sfx4, 255, Options.Normalize_Sound(130));
  Play_Sample(text2, 255, Options.Normalize_Sound(90));

  int frame = 1;

  while (frame < Get_Animation_Frame_Count(anim)) {
    if (frame == 16 || frame == 33 || frame == 44 || frame == 70 ||
        frame == 73) {
      Play_Sample(text2, 255, Options.Normalize_Sound(90));
    }
    if (frame == 21 || frame == 27) {
      Play_Sample(target1, 255, Options.Normalize_Sound(90));
    }
    if (frame == 45 || frame == 47 || frame == 49) {
      Play_Sample(beepy6, 255, Options.Normalize_Sound(90));
    }
    if (frame == 51) {
      Play_Sample(world2, 255, Options.Normalize_Sound(90));
    }
    if (frame == 70 || frame == 72) {
      Play_Sample(beepy2, 255, Options.Normalize_Sound(90));
    }
    if (frame == 74) {
      Play_Sample(target2, 255, Options.Normalize_Sound(110));
    }

    // the HEARTH_* animations don't have the text, but the EARTH_* ones do
    // (they're the same resolution, but the H are only available with
    // UPDATE.MIX)
    switch (frame) {
      case 1:
        Alloc_Object(new MultiStagePrintClass(
            Text_String(TXT_READING_IMAGE_DATA), 0, 10, _othergreenpal));
        break;

      case 16:
        TextPrintBuffer->Fill_Rect(
            0, 20, 2 * String_Pixel_Width(Text_String(TXT_READING_IMAGE_DATA)),
            2 * (10 + 12), BLACK);
        break;

      case 17:
        TextPrintBuffer->Fill_Rect(
            0, 20, 2 * String_Pixel_Width(Text_String(TXT_READING_IMAGE_DATA)),
            2 * (10 + 12), TBLACK);
        Alloc_Object(
            new MultiStagePrintClass("ANALYZING", 0, 10, _othergreenpal));
        break;

      case 33:
        TextPrintBuffer->Fill_Rect(
            0, 20, 2 * String_Pixel_Width(Text_String(TXT_ANALYZING)),
            2 * (10 + 12), BLACK);
        break;

      case 34:
        TextPrintBuffer->Fill_Rect(
            0, 20, 2 * String_Pixel_Width(Text_String(TXT_ANALYZING)),
            2 * (10 + 12), TBLACK);
        Alloc_Object(new MultiStagePrintClass(
            Text_String(TXT_ENHANCING_IMAGE_DATA), 0, 10, _othergreenpal));
        break;

      case 44:
        TextPrintBuffer->Fill_Rect(
            0, 20,
            2 * String_Pixel_Width(Text_String(TXT_ENHANCING_IMAGE_DATA)),
            2 * (10 + 12), BLACK);
        break;

      case 45:
        TextPrintBuffer->Fill_Rect(
            0, 20,
            2 * String_Pixel_Width(Text_String(TXT_ENHANCING_IMAGE_DATA)),
            2 * (10 + 12), TBLACK);
        Alloc_Object(new MultiStagePrintClass(
            Text_String(TXT_ISOLATING_OPERATIONAL_THEATER), 0, 10,
            _othergreenpal));
        break;

      case 70:
        TextPrintBuffer->Fill_Rect(0, 20,
                                   2 * String_Pixel_Width(Text_String(
                                           TXT_ISOLATING_OPERATIONAL_THEATER)),
                                   2 * (10 + 12), BLACK);
        break;

      case 71:
        TextPrintBuffer->Fill_Rect(0, 20,
                                   2 * String_Pixel_Width(Text_String(
                                           TXT_ISOLATING_OPERATIONAL_THEATER)),
                                   2 * (10 + 12), TBLACK);
        Alloc_Object(new MultiStagePrintClass(
            Text_String(TXT_ESTABLISHING_TRADITIONAL_BOUNDARIES), 0, 10,
            _othergreenpal));
        break;

      case 74:
        Alloc_Object(new MultiStagePrintClass(
            Text_String(TXT_FOR_VISUAL_REFERENCE), 0, 22, _othergreenpal));
        break;
    }

    Animate_Frame(anim, *PseudoSeenBuff, frame++);
    Call_Back_Delay(/*Keyboard::Check() ? 0 :*/ 3);
  }

  TextPrintBuffer->Fill_Rect(0, 20,
                             2 * String_Pixel_Width(Text_String(
                                     TXT_ESTABLISHING_TRADITIONAL_BOUNDARIES)),
                             2 * (10 + 24), BLACK);
  Call_Back_Delay(1);
  TextPrintBuffer->Fill_Rect(0, 20,
                             2 * String_Pixel_Width(Text_String(
                                     TXT_ESTABLISHING_TRADITIONAL_BOUNDARIES)),
                             2 * (10 + 24), TBLACK);
  Call_Back_Delay(1);

  Close_Animation(anim);

  Keyboard::Clear();
  BlitList.Clear();

  /*
  ** Freeze on the map of Europe or Africa
  */

  SysMemPage.Clear();
  Animate_Frame(progress, SysMemPage, 0);

  SysMemPage.Blit(*PseudoSeenBuff);

  InterpolationPaletteChanged = true;
  InterpolationPalette = progresspalette;
  Increase_Palette_Luminance(InterpolationPalette, 30, 30, 30, 63);
  Read_Interpolation_Palette("MAP_PROG.PAL");

  GraphicBufferClass* europe =
      new GraphicBufferClass(SysMemPage.Get_Width(), SysMemPage.Get_Height());
  SysMemPage.Blit(*europe);

  /*
  ** Now show territories as they existed last scenario
  */
  int startframe = CountryArray[scenario].Start[ScenDir];
  if (startframe) {
    Animate_Frame(progress, SysMemPage, startframe);
    SysMemPage.Blit(*PseudoSeenBuff);
  }
  Set_Palette(progresspalette);
  Call_Back_Delay(45);
  // Write_Interpolation_Palette("MAP_PROG.PAL");

  /*
  ** Now dissolve in first advance of territories
  */
  int xcoord = house == HOUSE_GOOD ? 0 : 204;
  SysMemPage.Blit(backpage, xcoord, 1, 0, 0, 20 * 6, 8);
  Play_Sample(text2, 255, Options.Normalize_Sound(90));
  if (house == HOUSE_GOOD) {
    Alloc_Object(new ScorePrintClass(TXT_MAP_GDI, 0, 2, _greenpal));
  } else {
    Alloc_Object(new ScorePrintClass(TXT_MAP_NOD, xcoord, 2, _greenpal));
  }
  Call_Back_Delay(60);

  Play_Sample(country1, 255, Options.Normalize_Sound(90));
  Animate_Frame(progress, SysMemPage, startframe + 1);
  Animate_Frame(progress, SysMemPage, startframe + 1);
  Bit_It_In(0, 0, 320, 200, &SysMemPage, PseudoSeenBuff, 1, true);
  backpage.Blit(SysMemPage, 0, 0, xcoord, 1, 20 * 6, 8);
  Call_Back_Delay(85);

  /*
  ** Now dissolve in second advance of territories
  */
#ifdef FRENCH
  PseudoSeenBuff->Fill_Rect(xcoord, 0, xcoord + 6 * 16 + 10, 8, BLACK);
  TextPrintBuffer->Fill_Rect(xcoord * 2, 0, 2 * (xcoord + 6 * 16 + 10), 16,
                             BLACK);
#else
  PseudoSeenBuff->Fill_Rect(xcoord, 0, xcoord + 6 * 16, 8, BLACK);
  TextPrintBuffer->Fill_Rect(2 * xcoord, 0, 2 * (xcoord + 6 * 16), 16, BLACK);
#endif

  Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, nullptr);

  SysMemPage.Blit(backpage, xcoord, 1, 0, 0, 20 * 6, 8);
  if (!lastscenario) {
    Play_Sample(text2, 255, Options.Normalize_Sound(90));
    if (house == HOUSE_GOOD) {
      Alloc_Object(new ScorePrintClass(TXT_MAP_NOD, 0, 12, _greenpal));
    } else {
      Alloc_Object(new ScorePrintClass(TXT_MAP_GDI, xcoord, 12, _greenpal));
    }
    Call_Back_Delay(65);
  }

  Play_Sample(country1, 255, Options.Normalize_Sound(90));
  Animate_Frame(progress, SysMemPage, startframe + 2);
  Bit_It_In(0, 0, 320, 200, &SysMemPage, PseudoSeenBuff, 1, true);
  backpage.Blit(SysMemPage, 0, 0, xcoord, 11, 20 * 6, 8);
  if (!lastscenario) {
    Call_Back_Delay(85);
  }
//	Set_Font(oldfont);
#ifdef FRENCH
  PseudoSeenBuff->Fill_Rect(xcoord, 12, xcoord + 6 * 16 + 10, 20, BLACK);
  TextPrintBuffer->Fill_Rect(2 * xcoord, 24, 2 * (xcoord + 6 * 16 + 10), 40,
                             BLACK);
#else
  PseudoSeenBuff->Fill_Rect(xcoord, 12, xcoord + 6 * 16, 20, BLACK);
  TextPrintBuffer->Fill_Rect(2 * xcoord, 24, 2 * (xcoord + 6 * 16), 40, BLACK);
#endif

  Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, nullptr);

  startframe = CountryArray[scenario].ContAnim[ScenDir];

  /*
  ** Now print the text over the page
  */
  Play_Sample(text2, 255, Options.Normalize_Sound(90));
  Alloc_Object(new ScorePrintClass(TXT_MAP_LOCATE, 0, 160, _greenpal));
  Call_Back_Delay(20);
  Alloc_Object(new ScorePrintClass(TXT_MAP_NEXT_MISSION, 0, 170, _greenpal));
#if (GERMAN | FRENCH)
  Call_Back_Delay(20);
  Alloc_Object(new ScorePrintClass(TXT_MAP_NEXT_MISS2, 0, 180, _greenpal));
#endif
  Call_Back_Delay(50);

  /*
  ** If we're on the last scenario, erase that text before doing the crosshairs
  */
  if (lastscenario) {
#if (GERMAN | FRENCH)
    SysMemPage.Fill_Rect(0, 160, 20 * 6, 186, TBLACK);
    PseudoSeenBuff->Fill_Rect(0, 160, 20 * 6, 186, TBLACK);
    TextPrintBuffer->Fill_Rect(0, 320, 40 * 6, 372, BLACK);
    SeenBuff.Fill_Rect(0, 320, 40 * 6, 372, TBLACK);
    HidPage.Fill_Rect(0, 320, 40 * 6, 372, TBLACK);
#else
    SysMemPage.Fill_Rect(0, 160, 20 * 6, 176, TBLACK);
    PseudoSeenBuff->Fill_Rect(0, 160, 20 * 6, 176, TBLACK);
    TextPrintBuffer->Fill_Rect(0, 320, 40 * 6, 352, BLACK);
    SeenBuff.Fill_Rect(0, 320, 40 * 6, 352, TBLACK);
    HidPage.Fill_Rect(0, 320, 40 * 6, 352, TBLACK);
#endif
    BlitList.Clear();
    Bit_It_In(0, 0, 320, 200, &SysMemPage, PseudoSeenBuff);
  }

  /*
  ** Fix up the palette that seems different for the last scenario
  */
  if (lastscenario) {
    InterpolationPaletteChanged = true;
    InterpolationPalette = CurrentPalette;
    if (house == HOUSE_GOOD) {
      Read_Interpolation_Palette("LASTSCNG.PAL");
      Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, "LASTSCNG.PAL");
    } else {
      Read_Interpolation_Palette("LASTSCNB.PAL");
      Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, "LASTSCNB.PAL");
    }
  }

  int q = 0;
  for (frame = 0;
       frame < (lastscenario ? Get_Animation_Frame_Count(progress) - 4 : 13);
       frame++) {
    if (!frame) {
      Play_Sample(beepy3, 255, Options.Normalize_Sound(90));
    }
    if (frame == 2) {
      Play_Sample(beepy3, 255, Options.Normalize_Sound(90));
    }
    if (frame == 6) {
      Play_Sample(newtarg1, 255, Options.Normalize_Sound(90));
    }

    if (lastscenario) {
      switch (frame) {
        case 23:
          if (house == HOUSE_GOOD) {
            Alloc_Object(new MultiStagePrintClass(
                Text_String(TXT_ENHANCING_IMAGE), 0, 10, _othergreenpal));
          } else {
#if (FRENCH)
            Alloc_Object(new MultiStagePrintClass(
                Text_String(TXT_ENHANCING_IMAGE), 180, 10, _othergreenpal));
#else
            Alloc_Object(new MultiStagePrintClass(
                Text_String(TXT_ENHANCING_IMAGE), 210, 10, _othergreenpal));
#endif  //(FRENCH)
          }
          // Frame 35 blacks out the region this frame starts printing into,
          // so this case has to stop here.
          break;

        case 35:
          if (house == HOUSE_GOOD) {
            TextPrintBuffer->Fill_Rect(
                0, 20, 2 * String_Pixel_Width(Text_String(TXT_ENHANCING_IMAGE)),
                2 * (10 + 12), BLACK);
          } else {
#if (FRENCH)
            TextPrintBuffer->Fill_Rect(
                360, 20,
                2 * (180 +
                     String_Pixel_Width(Text_String(TXT_ENHANCING_IMAGE))),
                2 * (10 + 12), BLACK);
#else
            TextPrintBuffer->Fill_Rect(
                420, 20,
                2 * (210 +
                     String_Pixel_Width(Text_String(TXT_ENHANCING_IMAGE))),
                2 * (10 + 12), BLACK);
#endif  //(FRENCH)
          }
          break;

        case 36:
          if (house == HOUSE_GOOD) {
            TextPrintBuffer->Fill_Rect(
                0, 20, 2 * String_Pixel_Width(Text_String(TXT_ENHANCING_IMAGE)),
                2 * (10 + 12), TBLACK);
          } else {
#if (FRENCH)
            TextPrintBuffer->Fill_Rect(
                360, 20,
                2 * (180 +
                     String_Pixel_Width(Text_String(TXT_ENHANCING_IMAGE))),
                2 * (10 + 12), TBLACK);
#else
            TextPrintBuffer->Fill_Rect(
                420, 20,
                2 * (210 +
                     String_Pixel_Width(Text_String(TXT_ENHANCING_IMAGE))),
                2 * (10 + 12), TBLACK);
#endif  //(FRENCH)
          }
          break;
      }
    }

    Animate_Frame(progress, *PseudoSeenBuff, startframe + frame);
    Call_Back_Delay(6);
    /* Cause it to cycle on the flashing on the country for a little while */
    if (!lastscenario && frame == 4 && q < 4) {
      frame = 2;
      q++;
    }
  }

  int selection = 0, color = 0;
  // erase the "Locating Coordinates" message...
  Play_Sample(beepy6, 255, Options.Normalize_Sound(90));
  if (!lastscenario) {
#if (GERMAN | FRENCH)
    SysMemPage.Fill_Rect(0, 160, 20 * 6, 186, TBLACK);
    PseudoSeenBuff->Fill_Rect(0, 160, 20 * 6, 186, TBLACK);
    TextPrintBuffer->Fill_Rect(0, 320, 40 * 6, 372, BLACK);
#else
    SysMemPage.Fill_Rect(0, 160, 20 * 6, 176, TBLACK);
    PseudoSeenBuff->Fill_Rect(0, 160, 20 * 6, 176, TBLACK);
    TextPrintBuffer->Fill_Rect(0, 320, 40 * 6, 352, BLACK);
#endif
  }

  Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, nullptr);

  /*
  ** Now the crosshairs are over the target countries - loop until a
  ** selection is made?
  */
  int done = 0;

  if (house == HOUSE_GOOD) {
    CCFileClass f(lastscenario ? "CLICK_EB.CPS" : "CLICK_E.CPS");
    Load_Uncompress(f, SysMemPage, SysMemPage, nullptr);
  } else {
    CCFileClass f(lastscenario ? "CLICK_SA.CPS" : "CLICK_A.CPS");
    Load_Uncompress(f, SysMemPage, SysMemPage, nullptr);
    if (lastscenario) {
      attackxcoord = 200;
    }
  }

  //	Set_Font(ScoreFontPtr);
  Play_Sample(text2, 255, Options.Normalize_Sound(90));
  Alloc_Object(
      new ScorePrintClass(TXT_MAP_SELECT, attackxcoord, 160, _greenpal));
  Cycle_Call_Back_Delay(16, progresspalette);
  Alloc_Object(
      new ScorePrintClass(TXT_MAP_TO_ATTACK, attackxcoord, 170, _greenpal));
  Cycle_Call_Back_Delay(24, progresspalette);
  while (Get_Mouse_State() > 0) {
    Show_Mouse();
  }

  Keyboard::Clear();
  while (!done) {
    Cycle_Call_Back_Delay(1, progresspalette);

    // Check for the mouse button
    if (Keyboard::Check()) {
      if ((Keyboard::Get() & 0x10FF) == KN_LMOUSE) {
        for (selection = 0; selection < CountryArray[scenario].Choices[ScenDir];
             selection++) {
          color = SysMemPage.Get_Pixel(Get_Mouse_X() / 2, Get_Mouse_Y() / 2);

          /*
          ** Special hack for Egypt the second time through
          */
          if (CountryArray[scenario].CountryColor[ScenDir][selection] == 0xA0) {
            if (color == 0x80 || color == 0x81) {
              color = 0xA0;
            }
          }

          if (CountryArray[scenario].CountryColor[ScenDir][selection] ==
              color) {
            Play_Sample(world2, 255, Options.Normalize_Sound(90));
            done = 1;
            break;
          }
          Play_Sample(scold1, 255, Options.Normalize_Sound(90));
        }
      }
    }
  }
  ScenVar = CountryArray[scenario].CountryVariant[ScenDir][selection];
  ScenDir = CountryArray[scenario].CountryDir[ScenDir][selection];

  if (!lastscenario) {
    Close_Animation(progress);

    /*
    ** Now it's time to highlight the country we're going to.
    */
    const void* countryshape =
        MFCD::Retrieve(house == HOUSE_GOOD ? "COUNTRYE.SHP" : "COUNTRYA.SHP");

    Hide_Mouse();
    // erase "Select country to attack"
    PseudoSeenBuff->Fill_Rect(attackxcoord, 160, attackxcoord + 17 * 6, 178,
                              BLACK);
    TextPrintBuffer->Fill_Rect(2 * attackxcoord, 320,
                               2 * (attackxcoord + 17 * 6), 2 * 178, BLACK);
#if (GERMAN | FRENCH)
    PseudoSeenBuff->Fill_Rect(attackxcoord + (17 * 6), 160,
                              attackxcoord + (21 * 6), 178, BLACK);
    TextPrintBuffer->Fill_Rect(2 * attackxcoord + (17 * 6 * 2), 320,
                               2 * (attackxcoord + (21 * 6)), 2 * 178, BLACK);
#endif  // GERMAN

    Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, nullptr);

    /*
    ** Draw the country's shape in non-fading colors
    */
    Set_Logic_Page(SysMemPage);
    europe->Blit(SysMemPage);
    int shape = CountryArray[scenario].CountryShape[ScenDir][selection];
    int xshuffled_rows = shape + (house == HOUSE_GOOD ? 0 : 18);
    CC_Draw_Shape(countryshape, shape, _countryx[xshuffled_rows],
                  _countryy[xshuffled_rows], WINDOW_MAIN,
                  SHAPE_WIN_REL | SHAPE_CENTER, nullptr, nullptr);
    SysMemPage.Blit(*PseudoSeenBuff);
    Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, nullptr);

    /*
    ** Now clear the palette of all but the country's colors, and fade
    ** the palette down
    */
    CCFileClass("DARK_E.PAL").Read(localpalette, 768);
    //		Load_Data("DARK_E.PAL", localpalette, 768);
    InterpolationPaletteChanged = true;
    InterpolationPalette = localpalette;
    Increase_Palette_Luminance(localpalette, 30, 30, 30, 63);
    Read_Interpolation_Palette("MAP_LOC2.PAL");
    Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, "MAP_LOC2.PAL");
    Fade_Palette_To(localpalette, kFadePaletteMedium, Call_Back);

    countryshape = nullptr;

    Print_Statistics(color & 0x7F, _countryx[xshuffled_rows],
                     _countryy[xshuffled_rows]);
  } else {
    CCFileClass(house == HOUSE_GOOD ? "DARK_B.PAL" : "DARK_SA.PAL")
        .Read(localpalette, 768);
    InterpolationPaletteChanged = true;
    InterpolationPalette = localpalette;
    Increase_Palette_Luminance(localpalette, 30, 30, 30, 63);
    Read_Interpolation_Palette("MAP_LOC3.PAL");
    Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, "MAP_LOC3.PAL");
    Set_Palette(localpalette);
    //		Load_Data(house == HOUSE_GOOD ? "DARK_B.PAL" : "DARK_SA.PAL",
    // localpalette, 768);

    Hide_Mouse();
#if (GERMAN | FRENCH)
    PseudoSeenBuff->Fill_Rect(attackxcoord, 160, 319, 178,
                              BLACK);  // erase "Select country to attack"
    TextPrintBuffer->Fill_Rect(2 * attackxcoord, 320, 639, 356,
                               BLACK);  // erase "Select country to attack"
#else
    PseudoSeenBuff->Fill_Rect(attackxcoord, 160, attackxcoord + 17 * 6, 199,
                              BLACK);  // erase "Select country to attack"
    TextPrintBuffer->Fill_Rect(2 * attackxcoord, 320,
                               2 * (attackxcoord + 17 * 6), 398,
                               BLACK);  // erase "Select country to attack"
#endif
    Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, nullptr);

    Animate_Frame(progress, *PseudoSeenBuff,
                  Get_Animation_Frame_Count(progress) - 1);
    Set_Palette(localpalette);
    Close_Animation(progress);
    PseudoSeenBuff->Blit(SysMemPage);
    Print_Statistics(20, 160, house == HOUSE_GOOD ? 0 : 160);
  }

  Theme.Queue_Song(THEME_NONE);
  Fade_Palette_To(BlackPalette, kFadePaletteMedium, nullptr);
  delete europe;
  delete[] progresspalette;
  delete[] grey2palette;
  delete TextPrintBuffer;
  TextPrintBuffer = nullptr;
  BlitList.Clear();
}

/***************************************************************************
 * Print_Statistics -- Prints statistics on country selected               *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * INPUT:   shape = country #, x & y = country's on-screen coords          *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/27/1995 BWG : Created.                                             *
 *=========================================================================*/
void Print_Statistics(int country, int xpos, int ypos) {
  int index, newx;
  const void* oldfont;
  static const int _gdistatnames[] = {
      TXT_MAP_GDISTAT0, TXT_MAP_GDISTAT1, TXT_MAP_GDISTAT2, TXT_MAP_GDISTAT3,
      TXT_MAP_GDISTAT4, TXT_MAP_GDISTAT5, TXT_MAP_GDISTAT6};
  static const int _nodstatnames[] = {
      TXT_MAP_GDISTAT0, TXT_MAP_NODSTAT0, TXT_MAP_GDISTAT2,
      TXT_MAP_GDISTAT3, TXT_MAP_NODSTAT1, TXT_MAP_NODSTAT2,
      TXT_MAP_GDISTAT5, TXT_MAP_NODSTAT3, TXT_MAP_NODSTAT4};
  static const int _countryname[] = {
      TXT_MAP_COUNTRYNAME0,  TXT_MAP_COUNTRYNAME1,  TXT_MAP_COUNTRYNAME2,
      TXT_MAP_COUNTRYNAME3,  TXT_MAP_COUNTRYNAME4,  TXT_MAP_COUNTRYNAME5,
      TXT_MAP_COUNTRYNAME6,  TXT_MAP_COUNTRYNAME7,  TXT_MAP_COUNTRYNAME8,
      TXT_MAP_COUNTRYNAME9,  TXT_MAP_COUNTRYNAME10, TXT_MAP_COUNTRYNAME11,
      TXT_MAP_COUNTRYNAME12, TXT_MAP_COUNTRYNAME13, TXT_MAP_COUNTRYNAME14,
      TXT_MAP_COUNTRYNAME15, TXT_MAP_COUNTRYNAME16, TXT_MAP_COUNTRYNAME17,
      TXT_MAP_COUNTRYNAME18, TXT_MAP_COUNTRYNAME19, TXT_MAP_COUNTRYNAME20,
      TXT_MAP_COUNTRYNAME21, TXT_MAP_COUNTRYNAME22, TXT_MAP_COUNTRYNAME23,
      TXT_MAP_COUNTRYNAME24, TXT_MAP_COUNTRYNAME25, TXT_MAP_COUNTRYNAME26,
      TXT_MAP_COUNTRYNAME27, TXT_MAP_COUNTRYNAME28, TXT_MAP_COUNTRYNAME29,
      TXT_MAP_COUNTRYNAME30, TXT_MAP_COUNTRYNAME31, TXT_MAP_COUNTRYNAME32,
      TXT_MAP_COUNTRYNAME33, TXT_MAP_COUNTRYNAME34};

  static const int _govtnames[] = {
      TXT_MAP_GOVT0, TXT_MAP_GOVT1, TXT_MAP_GOVT2,  TXT_MAP_GOVT3,
      TXT_MAP_GOVT4, TXT_MAP_GOVT5, TXT_MAP_GOVT6,  TXT_MAP_GOVT7,
      TXT_MAP_GOVT8, TXT_MAP_GOVT9, TXT_MAP_GOVT10, TXT_MAP_GOVT11};
  static const int _armynames[] = {TXT_MAP_ARMY0, TXT_MAP_ARMY1, TXT_MAP_ARMY2,
                                   TXT_MAP_ARMY3, TXT_MAP_ARMY4, TXT_MAP_ARMY5};
  static const int _military[] = {TXT_MAP_MILITARY0, TXT_MAP_MILITARY1,
                                  TXT_MAP_MILITARY2, TXT_MAP_MILITARY3,
                                  TXT_MAP_MILITARY4};

  static const char _greenpal[] = {0,    0x41, 0x42, 0x43, 0x44, 0x44,
                                   0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
                                   0x44, 0x44, 0x44, 0x44};
  // static char const
  // _greenpal[]={0,1,0x42,3,0x43,5,0x44,7,0x44,9,10,1,12,13,0x41,15};
  static char _deststr[16];

  /* Change to the six-point font for Text_Print */
  oldfont = Set_Font(ScoreFontPtr);

#ifdef GERMAN
  xpos = 8;
#else
  xpos = xpos > 128 ? 8 : 136;
#endif
  ypos = ypos > 100 ? 8 : 104 - 6;
  if (PlayerPtr->Class->House == HOUSE_GOOD) {
    Alloc_Object(new ScorePrintClass(_countryname[GDIStats[country].nameindex],
                                     xpos, ypos, _greenpal));
    Call_Back_Delay(
        strlen(Text_String(_countryname[GDIStats[country].nameindex])) * 3);
    ypos += 16;
    for (index = 0; index < 7; index++) {
      Alloc_Object(
          new ScorePrintClass(_gdistatnames[index], xpos, ypos, _greenpal));
      Call_Back_Delay(strlen(Text_String(_gdistatnames[index] + 3)));
      newx = xpos + 6 * strlen(Text_String(_gdistatnames[index]));
      switch (index) {
        case 0:
          Alloc_Object(new ScorePrintClass(GDIStats[country].pop, newx, ypos,
                                           _greenpal));
          break;
        case 1:
          Alloc_Object(new ScorePrintClass(GDIStats[country].area, newx, ypos,
                                           _greenpal));
          break;
        case 2:
          Alloc_Object(new ScorePrintClass(GDIStats[country].capital, newx,
                                           ypos, _greenpal));
          break;
        case 3:
          Alloc_Object(new ScorePrintClass(_govtnames[GDIStats[country].govt],
                                           newx, ypos, _greenpal));
          break;
        case 4:
          Alloc_Object(new ScorePrintClass(GDIStats[country].gdp, newx, ypos,
                                           _greenpal));
          break;
        case 5:
          Alloc_Object(new ScorePrintClass(GDIStats[country].conflict, newx,
                                           ypos, _greenpal));
          break;
        case 6:
          Alloc_Object(new ScorePrintClass(
              _armynames[GDIStats[country].military], newx, ypos, _greenpal));
          break;
      }
      ypos += 8;
    }
  } else {  // Nod statistics
    if (country > 30) {
      country = 15;  // hack for 2nd time in Egypt
    } else {
      if (country >= 15) {
        country++;  // hack to account for Egypt
      }
    }
    country++;

    Alloc_Object(new ScorePrintClass(_countryname[NodStats[country].nameindex],
                                     xpos, ypos, _greenpal));
    Call_Back_Delay(
        strlen(Text_String(_countryname[NodStats[country].nameindex])) * 3);
    ypos += 16;
    for (index = 0; index < 9; index++) {
      Alloc_Object(
          new ScorePrintClass(_nodstatnames[index], xpos, ypos, _greenpal));
      Call_Back_Delay(strlen(Text_String(_nodstatnames[index] + 3)));
      newx = xpos + 6 * strlen(Text_String(_nodstatnames[index]));
      switch (index) {
        case 0:
          Alloc_Object(new ScorePrintClass(NodStats[country].pop, newx, ypos,
                                           _greenpal));
          break;
        case 1:
          sprintf(_deststr, "%d%%", NodStats[country].expendable);
          Alloc_Object(new ScorePrintClass(_deststr, newx, ypos, _greenpal));
          break;
        case 2:
          Alloc_Object(new ScorePrintClass(NodStats[country].capital, newx,
                                           ypos, _greenpal));
          break;
        case 3:
          Alloc_Object(new ScorePrintClass(_govtnames[NodStats[country].govt],
                                           newx, ypos, _greenpal));
          break;
        case 4:
#ifdef FIX_ME_LATER
          sprintf(_deststr, "%s %d%%",
                  LowMedHiStr(NodStats[country].corruptible),
                  NodStats[country].corruptible);
#endif  // FIX_ME_LATER
          sprintf(_deststr, "%d%%", NodStats[country].corruptible);
          Alloc_Object(new ScorePrintClass(_deststr, newx, ypos, _greenpal));
          break;
        case 5:
          Alloc_Object(new ScorePrintClass(NodStats[country].worth, newx, ypos,
                                           _greenpal));
          break;
        case 6:
          Alloc_Object(new ScorePrintClass(NodStats[country].conflict, newx,
                                           ypos, _greenpal));
          break;
        case 7:
          Alloc_Object(new ScorePrintClass(
              _military[NodStats[country].military], newx, ypos, _greenpal));
          break;
        case 8:
          sprintf(_deststr, "%d%%", NodStats[country].probability);
          Alloc_Object(new ScorePrintClass(_deststr, newx, ypos, _greenpal));
          break;
      }
      ypos += 8;
    }
  }

#ifdef FRENCH
  Alloc_Object(new ScorePrintClass(TXT_MAP_CLICK2, 94, 193 - 6, _greenpal));
#else
  Alloc_Object(
      new ScorePrintClass(TXT_MAP_CLICK2, 160 - 17 * 3, 193 - 6, _greenpal));
#endif

  int done = 0;
  while (!done) {
    done = 1;
    for (int x = 0; x < MAXSCOREOBJS; x++) {
      if (ScoreObjs[x]) {
        done = 0;
        Call_Back_Delay(1);
      }
    }
  }
  Keyboard::Clear();
  while (Keyboard::Check()) {
    Keyboard::Clear();
  }
  while (!Keyboard::Check() && !ControlQ) {
    Call_Back_Delay(1);
  }
  Keyboard::Clear();
  Set_Font(oldfont);
}

#ifdef NEVER
/***************************************************************************
 * FADING_BYTE_BLIT -- 'Pixelized' incremental byte blit.                  *
 *                                                                         *
 *    This routine will perform the same function as Byte_Blit, but will   *
 *    do so in an incremental (one piece at a time) method.  This is       *
 *    usefull for graphic 'fades' from one display to another.             *
 *                                                                         *
 * INPUT:   srcx  - Source page X byte position of upper left corner.      *
 *                                                                         *
 *          srcy  - Source page Y pixel position of upper left corner.     *
 *                                                                         *
 *          destx - Dest page X byte position of upper left corner.        *
 *                                                                         *
 *          desty - Dest page Y pixel position of upper left corner.       *
 *                                                                         *
 *          w     - Width of the blit in bytes.                            *
 *                                                                         *
 *          h     - Height of the blit in pixels.                          *
 *                                                                         *
 *          src   - PageType of the source page.                           *
 *                                                                         *
 *          dest  - Page type of the destination.                          *
 *                                                                         *
 * OUTPUT:  none                                                           *
 *                                                                         *
 * WARNINGS:   This routine, although functionally equivalent to the       *
 *             normal Byte_Blit, is very slow.  Only use this when         *
 *             the 'fading' graphic effect is desired.  This means the     *
 *             destination page should probably be the SEENPAGE.           *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/17/1991 JLB : Created from Bit_It_In() (LJC code).                 *
 *   04/17/1995 BWG : Adapted to the C++ system.
 **
 *=========================================================================*/
void Fading_Byte_Blit(int srcx, int srcy, int destx, int desty, int w, int h,
                      GraphicBufferClass* src, GraphicBufferClass* dest) {
  unsigned int shuffled_cols,  // Working array index var.
      shuffled_rows;           // Working y index var.
  unsigned int x, y;           // Extraction position indexes.
  unsigned int tempy;          // Temporary working Y index var.
  int _shuffled_cols[40];      // X position array.
  int _shuffled_rows[200];     // Y position array.

  // Anticipate two pixel rows per blit.
  h >>= 1;

  // This routine is byte-aligned
  srcx >>= 3;
  destx >>= 3;
  w >>= 3;

  for (shuffled_cols = 0; shuffled_cols < w; shuffled_cols++) {
    _shuffled_cols[shuffled_cols] = shuffled_cols; /* init the index array */
  }
  for (shuffled_rows = 0; shuffled_rows < h; shuffled_rows++) {
    _shuffled_rows[shuffled_rows] = shuffled_rows; /* init the index array */
  }

  /*
  **	Shuffle the X indexes around a bit.  This gives it
  **	that 'random' feel while remaining precise.
  */
  for (shuffled_cols = 0; shuffled_cols < w; shuffled_cols++) {
    int temp;

    x = IRandom(0, w - 1);
    temp = _shuffled_cols[x];
    _shuffled_cols[x] = _shuffled_cols[shuffled_cols];
    _shuffled_cols[shuffled_cols] = temp;
  }

  /*
  **	Shuffle the Y indexes around a bit for the same reason that
  **	the x indexes were shuffled.
  */
  for (shuffled_rows = 0; shuffled_rows < h; shuffled_rows++) {
    int temp;

    y = IRandom(0, h - 1);
    temp = _shuffled_rows[y];
    _shuffled_rows[y] = _shuffled_rows[shuffled_rows];
    _shuffled_rows[shuffled_rows] = temp;
  }

  /*
  **	Sweep through the indexes and 'construct' the destination display
  **	from a series of miniature Byte_Blits.
  */
  for (shuffled_rows = 0; shuffled_rows < h; shuffled_rows++) {
    tempy = shuffled_rows;
    Call_Back();
    for (shuffled_cols = 0; shuffled_cols < w; shuffled_cols++) {
      x = _shuffled_cols[shuffled_cols];
      y = _shuffled_rows[tempy];
      tempy++;
      if (tempy >= h) {
        tempy = 0;
      }
      src->Blit(*dest, (srcx + x) << 3, srcy + (y << 1), (destx + x) << 3,
                desty + (y << 1), 1 << 3, 2);
    }
  }
}
#endif

void Cycle_Call_Back_Delay(int time, unsigned char* pal) {
  static int _counter;
  unsigned char r, g, b;
  int i;

  while (time--) {
    _counter = ++_counter & 3;

    if (!(_counter & 3)) {
      r = pal[249 * 3 + 0];
      g = pal[249 * 3 + 1];
      b = pal[249 * 3 + 2];

      for (i = 249; i < 254; i++) {
        pal[i * 3 + 0] = pal[(i + 1) * 3 + 0];
        pal[i * 3 + 1] = pal[(i + 1) * 3 + 1];
        pal[i * 3 + 2] = pal[(i + 1) * 3 + 2];
      }
      pal[254 * 3 + 0] = r;
      pal[254 * 3 + 1] = g;
      pal[254 * 3 + 2] = b;

      Set_Palette(pal);
    }
    Call_Back_Delay(1);
  }
}

int LowMedHiStr(int percentage) {
  if (percentage < 30) {
    return TXT_MAP_LMH0;
  }
  if (percentage < 70) {
    return TXT_MAP_LMH1;
  }
  return TXT_MAP_LMH2;
}

#endif  // DEMO

/***************************************************************************
 * Bit_It_In -- Pixel fade graphic copy.                                   *
 *                                                                         *
 *    Copies a block of graphic memory using a 'random' pixel algorithm.   *
 *    Typical use would be to 'fade' some graphic display into another.    *
 *                                                                         *
 * INPUT:   x,y   - Pixel position of upper left corner of block.          *
 *                                                                         *
 *          w,y   - Pixel width and height of block to pixel blit.         *
 *                                                                         *
 *          src   - Page number of the source page.                        *
 *                                                                         *
 *          dest  - Page number of the destination page.                   *
 *                                                                         *
 *          delay - # of frames to wait after each line fades in           *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * WARNINGS:   This function uses PIXEL coordinates for the X and width    *
 *             parameters.  This is unlike the Byte_Blit() routine that    *
 *             it most closely resembles.                                  *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/16/1991 JLB : Created.                                             *
 *   04/17/1995 BWG : Adapted to C++ library.                              *
 *=========================================================================*/
void Bit_It_In(const int x, const int y, const int w, const int h,
               GraphicBufferClass* src, GraphicBufferClass* dest,
               const int delay, const bool dagger) {
  // Build shuffled coordinate tables so pixels are copied in random order,
  // creating a dissolve transition where the new image materializes from
  // scattered dots rather than appearing all at once.
  int shuffled_cols[320];
  int shuffled_rows[200];
  std::iota(shuffled_cols, shuffled_cols + w, 0);
  std::iota(shuffled_rows, shuffled_rows + h, 0);

  absl::BitGen gen;
  auto x_span = absl::MakeSpan(shuffled_cols, w);
  auto y_span = absl::MakeSpan(shuffled_rows, h);
  absl::c_shuffle(x_span, gen);
  absl::c_shuffle(y_span, gen);

  // Copy w pixels per iteration in shuffled order. The Call_Back_Delay
  // between iterations processes the game loop (including screen
  // rendering), so each batch of random pixels becomes visible before
  // the next batch is drawn.
  for (int line = 0; line < h; line++) {
    int row_offset = line;
    if (line & 1) {
      int remaining = delay;
      do {
        Call_Back_Delay(remaining ? 1 : 0);
      } while (remaining--);
    } else {
      Call_Back();
    }

    if (src->Lock() && dest->Lock()) {
      // Each pixel uses a shuffled x and a wrapping y offset, so pixels
      // scatter across the entire image rather than filling row by row.
      for (int col = 0; col < w; col++) {
        const int px = x + shuffled_cols[col];
        const int py = y + shuffled_rows[row_offset];
        row_offset++;
        if (row_offset >= h) {
          row_offset = 0;
        }

        dest->Buffer_Put_Pixel(px, py, Buffer_Get_Pixel(src, px, py));
      }
      if (dagger) {
        // Overlay a downward-pointing wedge from screen center (x=160),
        // expanding one pixel wider per row. This adds a dagger-shaped
        // reveal on top of the random dissolve.
        // NOTE: Ignores x/y/w/h and assumes a full 320-wide screen.
        // Only used with full-screen (0,0,320,200) dissolves.
        for (int row = line; row >= 0; row--) {
          const unsigned offset = line - row;
          const unsigned x_left = 160 - offset;
          const unsigned x_right = 160 + offset;
          dest->Buffer_Put_Pixel(x_left, row,
                                 Buffer_Get_Pixel(src, x_left, row));
          dest->Buffer_Put_Pixel(x_right, row,
                                 Buffer_Get_Pixel(src, x_right, row));
        }
      }
    }
    src->Unlock();
    dest->Unlock();
  }
}
