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

#include <array>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <span>
#include <string_view>

#include "absl/algorithm/container.h"
#include "absl/random/random.h"
#include "absl/strings/str_format.h"
#include "engine/audio/audio_mixer.h"
#include "engine/base/array.h"
#include "engine/base/numeric.h"
#include "engine/file/game_file.h"
#include "engine/file/mix_archive.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "engine/gfx/wsa_animation.h"
#include "engine/gfx/wwstd.h"
#include "engine/platform/timer.h"
#include "engine/window/keyboard.h"
#include "engine/window/misc.h"
#include "engine/window/ww_mouse.h"
#include "td/assets.h"
#include "td/audio.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/game_state.h"
#include "td/goptions.h"
#include "td/house.h"
#include "td/input.h"
#include "td/interpal.h"
#include "td/jshell.h"
#include "td/palette.h"
#include "td/palettes.h"
#include "td/presentation.h"
#include "td/score.h"
#include "td/screen.h"
#include "td/text.h"
#include "td/theme.h"
#include "td/type.h"
#include "td/world.h"

#ifndef DEMO

void Fading_Byte_Blit(int srcx, int srcy, int destx, int desty, int w, int h,
                      PixelBuffer* src, PixelBuffer* dest);
static void Print_Statistics(Presentation& show, int country, int xpos,
                             int ypos);
static void Cycle_Call_Back_Delay(Presentation& show, int time,
                                  std::span<unsigned char> pal);
[[maybe_unused]] static int LowMedHiStr(int percentage);

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
  unsigned char localpalette[768]{};
  bool lastscenario = false;
  const HousesType house = ThePlayer()->Class->House;
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
  static const unsigned char greenpal[] = {0,    0x41, 0x42, 0x43, 0x44, 0x44,
                                           0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
                                           0x44, 0x44, 0x44, 0x44};
  static const unsigned char _othergreenpal[] = {
      0,    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x26,
      0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26};
  PixelBuffer backpage(20 * 6, 8);

  std::array<unsigned char, 768> grey2palette{};
  std::array<unsigned char, 768> progresspalette{};

  Keyboard::Clear();
  // The score animations print this screen's captions in the score font; the
  // rectangles that clear them again are measured in it too.
  const FontStyle score_font = ScoreFontStyle();
  Set_Palette(ThePalettes().black_palette());

  const int scenario = TheWorld().scenario() + (house == HOUSE_GOOD ? 0 : 14);
  if (house == HOUSE_GOOD) {
    lastscenario = TheWorld().scenario() == 14;
    if (TheWorld().scenario() == 15) {
      return;
    }
  } else {
    lastscenario = TheWorld().scenario() == 12;
    if (TheWorld().scenario() == 13) {
      return;
    }
  }

  // Check if they're even entitled to map selection this time
  if (base::At(base::At(CountryArray, scenario).Choices,
               static_cast<int>(TheWorld().scen_dir())) == 0) {
    return;
  }

  TheTheme().Queue_Song(THEME_MAP1);

  Presentation show;

  /*
  ** Now start the process where we fade the gray earth in.
  */
  WsaAnimation greyearth("GREYERTH.WSA", localpalette);
  WsaAnimation greyearth2("E-BWTOCL.WSA", grey2palette);

  /*
  ** Load the spinning-globe anim
  */
  const bool good = house == HOUSE_GOOD;
  WsaAnimation anim(good ? "HEARTH_E.WSA" : "HEARTH_A.WSA",
                    ThePalettes().title_palette());
  const char* progress_name = lastscenario ? "HSAFRICA.WSA" : "AFRICA.WSA";
  if (good) {
    progress_name = lastscenario ? "HBOSNIA.WSA" : "EUROPE.WSA";
  }
  WsaAnimation progress(progress_name, progresspalette);

  const auto appear1 = MixArchive::RetrieveData("APPEAR1.AUD");
  const auto sfx4 = MixArchive::RetrieveData("SFX4.AUD");
  const auto text2 = MixArchive::RetrieveData("TEXT2.AUD");
  const auto target1 = MixArchive::RetrieveData("TARGET1.AUD");
  const auto target2 = MixArchive::RetrieveData("TARGET2.AUD");
  //	void const * target3 = MixArchive::RetrieveData("TARGET3.AUD");
  const auto newtarg1 = MixArchive::RetrieveData("NEWTARG1.AUD");
  const auto beepy2 = MixArchive::RetrieveData("BEEPY2.AUD");
  const auto beepy3 = MixArchive::RetrieveData("BEEPY3.AUD");
  const auto beepy6 = MixArchive::RetrieveData("BEEPY6.AUD");
  const auto world2 = MixArchive::RetrieveData("WORLD2.AUD");
  const auto country1 = MixArchive::RetrieveData("COUNTRY1.AUD");
  const auto scold1 = MixArchive::RetrieveData("SCOLD1.AUD");

  TheScreen().sys_mem_page().view().Clear();
  show.page().view().Clear();
  TheMouse()->Erase_Mouse(&TheScreen().hidden_view(), true);
  TheScreen().hidden_page().view().Clear();

  Increase_Palette_Luminance(ThePalettes().title_palette(), 30, 30, 30, 63);

  //	SeenBuff.Blit(HidPage);
  greyearth.DrawFrame(TheScreen().sys_mem_page().view(), 0);

  Bit_It_In(show, 0, 0, 320, 200, &TheScreen().sys_mem_page(), &show.page());
  show.page().view().PutPixel(237, 92, kTBlack);
  show.page().view().PutPixel(237, 93, kTBlack);

  Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(), "MAP1.PAL");

  Increase_Palette_Luminance(localpalette, 30, 30, 30, 63);

  TheAudio().Play(appear1, 255, TheOptions().Normalize_Sound(110));
  Fade_Palette_To(localpalette, kFadePaletteMedium, Call_Back);
  for (int i = 1; i < greyearth.frame_count(); i++) {
    Call_Back_Delay(show, 4);
    greyearth.DrawFrame(show.page().view(), i);
  }
  greyearth.Close();

  Call_Back_Delay(show, 4);

  TheScreen().sys_mem_page().view().Clear();
  greyearth2.DrawFrame(TheScreen().sys_mem_page().view(), 0);

  Increase_Palette_Luminance(grey2palette, 30, 30, 30, 63);
  Wait_Vert_Blank();
  Set_Palette(grey2palette);

  TheScreen().sys_mem_page().view().BlitTo(show.page().view());

  Call_Back_Delay(show, 4);
  for (int i = 1; i < greyearth2.frame_count(); i++) {
    greyearth2.DrawFrame(show.page().view(), i);
    Call_Back_Delay(show, 4);
  }
  greyearth2.Close();


  /*
  ** Copy the first frame up to the seenpage (while screen is black)
  */
  TheScreen().sys_mem_page().view().Clear();
  anim.DrawFrame(TheScreen().sys_mem_page().view(), 1);
  TheScreen().sys_mem_page().view().BlitTo(show.page().view());

  Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(), {});

  Stop_Speaking();

  while (TheGameState().speech_timer().Time() || Is_Speaking()) {
    Call_Back();
    //		if (Keyboard::Check()) CountDownTimer.Set(0);
  }

  //	Keyboard::Clear();

  /*
  ** now make the grid appear
  */
  TheScreen().sys_mem_page().view().BlitTo(show.page().view());
  Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(), {});

  TheAudio().Play(sfx4, 255, TheOptions().Normalize_Sound(130));
  TheAudio().Play(text2, 255, TheOptions().Normalize_Sound(90));

  int frame = 1;

  while (frame < anim.frame_count()) {
    if (frame == 16 || frame == 33 || frame == 44 || frame == 70 ||
        frame == 73) {
      TheAudio().Play(text2, 255, TheOptions().Normalize_Sound(90));
    }
    if (frame == 21 || frame == 27) {
      TheAudio().Play(target1, 255, TheOptions().Normalize_Sound(90));
    }
    if (frame == 45 || frame == 47 || frame == 49) {
      TheAudio().Play(beepy6, 255, TheOptions().Normalize_Sound(90));
    }
    if (frame == 51) {
      TheAudio().Play(world2, 255, TheOptions().Normalize_Sound(90));
    }
    if (frame == 70 || frame == 72) {
      TheAudio().Play(beepy2, 255, TheOptions().Normalize_Sound(90));
    }
    if (frame == 74) {
      TheAudio().Play(target2, 255, TheOptions().Normalize_Sound(110));
    }

    // the HEARTH_* animations don't have the text, but the EARTH_* ones do
    // (they're the same resolution, but the H are only available with
    // UPDATE.MIX)
    switch (frame) {
      case 1:
        Alloc_Object(new MultiStagePrintClass(
            show, Text_String(TXT_READING_IMAGE_DATA), 0, 10, _othergreenpal));
        break;

      case 16:
        show.text_page().view().FillRect(
            0, 20,
            2 * StringPixelWidth(score_font,
                                 Text_String(TXT_READING_IMAGE_DATA)),
            2 * (10 + 12), kBlack);
        break;

      case 17:
        show.text_page().view().FillRect(
            0, 20,
            2 * StringPixelWidth(score_font,
                                 Text_String(TXT_READING_IMAGE_DATA)),
            2 * (10 + 12), kTBlack);
        Alloc_Object(
            new MultiStagePrintClass(show, "ANALYZING", 0, 10, _othergreenpal));
        break;

      case 33:
        show.text_page().view().FillRect(
            0, 20, 2 * StringPixelWidth(score_font, Text_String(TXT_ANALYZING)),
            2 * (10 + 12), kBlack);
        break;

      case 34:
        show.text_page().view().FillRect(
            0, 20, 2 * StringPixelWidth(score_font, Text_String(TXT_ANALYZING)),
            2 * (10 + 12), kTBlack);
        Alloc_Object(new MultiStagePrintClass(
            show, Text_String(TXT_ENHANCING_IMAGE_DATA), 0, 10,
            _othergreenpal));
        break;

      case 44:
        show.text_page().view().FillRect(
            0, 20,
            2 * StringPixelWidth(score_font,
                                 Text_String(TXT_ENHANCING_IMAGE_DATA)),
            2 * (10 + 12), kBlack);
        break;

      case 45:
        show.text_page().view().FillRect(
            0, 20,
            2 * StringPixelWidth(score_font,
                                 Text_String(TXT_ENHANCING_IMAGE_DATA)),
            2 * (10 + 12), kTBlack);
        Alloc_Object(new MultiStagePrintClass(
            show, Text_String(TXT_ISOLATING_OPERATIONAL_THEATER), 0, 10,
            _othergreenpal));
        break;

      case 70:
        show.text_page().view().FillRect(
            0, 20,
            2 * StringPixelWidth(
                    score_font, Text_String(TXT_ISOLATING_OPERATIONAL_THEATER)),
            2 * (10 + 12), kBlack);
        break;

      case 71:
        show.text_page().view().FillRect(
            0, 20,
            2 * StringPixelWidth(
                    score_font, Text_String(TXT_ISOLATING_OPERATIONAL_THEATER)),
            2 * (10 + 12), kTBlack);
        Alloc_Object(new MultiStagePrintClass(
            show, Text_String(TXT_ESTABLISHING_TRADITIONAL_BOUNDARIES), 0, 10,
            _othergreenpal));
        break;

      case 74:
        Alloc_Object(new MultiStagePrintClass(
            show, Text_String(TXT_FOR_VISUAL_REFERENCE), 0, 22,
            _othergreenpal));
        break;
      default:
        break;
    }

    anim.DrawFrame(show.page().view(), frame++);
    Call_Back_Delay(show, /*Keyboard::Check() ? 0 :*/ 3);
  }

  show.text_page().view().FillRect(
      0, 20,
      2 * StringPixelWidth(
              score_font, Text_String(TXT_ESTABLISHING_TRADITIONAL_BOUNDARIES)),
      2 * (10 + 24), kBlack);
  Call_Back_Delay(show, 1);
  show.text_page().view().FillRect(
      0, 20,
      2 * StringPixelWidth(
              score_font, Text_String(TXT_ESTABLISHING_TRADITIONAL_BOUNDARIES)),
      2 * (10 + 24), kTBlack);
  Call_Back_Delay(show, 1);

  anim.Close();

  Keyboard::Clear();
  show.ClearTextRects();

  /*
  ** Freeze on the map of Europe or Africa
  */

  TheScreen().sys_mem_page().view().Clear();
  progress.DrawFrame(TheScreen().sys_mem_page().view(), 0);

  TheScreen().sys_mem_page().view().BlitTo(show.page().view());

  Increase_Palette_Luminance(progresspalette, 30, 30, 30, 63);

  auto* europe = new PixelBuffer(TheScreen().sys_mem_page().width(),
                                 TheScreen().sys_mem_page().height());
  TheScreen().sys_mem_page().view().BlitTo(europe->view());

  /*
  ** Now show territories as they existed last scenario
  */
  int startframe = base::At(base::At(CountryArray, scenario).Start,
                            static_cast<int>(TheWorld().scen_dir()));
  if (startframe) {
    progress.DrawFrame(TheScreen().sys_mem_page().view(), startframe);
    TheScreen().sys_mem_page().view().BlitTo(show.page().view());
  }
  Set_Palette(progresspalette);
  Call_Back_Delay(show, 45);

  /*
  ** Now dissolve in first advance of territories
  */
  const int xcoord = house == HOUSE_GOOD ? 0 : 204;
  TheScreen().sys_mem_page().view().BlitTo(backpage.view(), xcoord, 1, 0, 0,
                                           20 * 6, 8);
  TheAudio().Play(text2, 255, TheOptions().Normalize_Sound(90));
  if (house == HOUSE_GOOD) {
    Alloc_Object(new ScorePrintClass(show, TXT_MAP_GDI, 0, 2, greenpal));
  } else {
    Alloc_Object(new ScorePrintClass(show, TXT_MAP_NOD, xcoord, 2, greenpal));
  }
  Call_Back_Delay(show, 60);

  TheAudio().Play(country1, 255, TheOptions().Normalize_Sound(90));
  progress.DrawFrame(TheScreen().sys_mem_page().view(), startframe + 1);
  progress.DrawFrame(TheScreen().sys_mem_page().view(), startframe + 1);
  Bit_It_In(show, 0, 0, 320, 200, &TheScreen().sys_mem_page(), &show.page(), 1,
            true);
  backpage.view().BlitTo(TheScreen().sys_mem_page().view(), 0, 0, xcoord, 1,
                         20 * 6, 8);
  Call_Back_Delay(show, 85);

  /*
  ** Now dissolve in second advance of territories
  */
#ifdef FRENCH
  show.page().view().FillRect(xcoord, 0, xcoord + 6 * 16 + 10, 8, kBlack);
  show.text_page().view().FillRect(xcoord * 2, 0, 2 * (xcoord + 6 * 16 + 10),
                                   16, kBlack);
#else
  show.page().view().FillRect(xcoord, 0, xcoord + (6 * 16), 8, kBlack);
  show.text_page().view().FillRect(2 * xcoord, 0, 2 * (xcoord + (6 * 16)), 16,
                                   kBlack);
#endif

  Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(), {});

  TheScreen().sys_mem_page().view().BlitTo(backpage.view(), xcoord, 1, 0, 0,
                                           20 * 6, 8);
  if (!lastscenario) {
    TheAudio().Play(text2, 255, TheOptions().Normalize_Sound(90));
    if (house == HOUSE_GOOD) {
      Alloc_Object(new ScorePrintClass(show, TXT_MAP_NOD, 0, 12, greenpal));
    } else {
      Alloc_Object(
          new ScorePrintClass(show, TXT_MAP_GDI, xcoord, 12, greenpal));
    }
    Call_Back_Delay(show, 65);
  }

  TheAudio().Play(country1, 255, TheOptions().Normalize_Sound(90));
  progress.DrawFrame(TheScreen().sys_mem_page().view(), startframe + 2);
  Bit_It_In(show, 0, 0, 320, 200, &TheScreen().sys_mem_page(), &show.page(), 1,
            true);
  backpage.view().BlitTo(TheScreen().sys_mem_page().view(), 0, 0, xcoord, 11,
                         20 * 6, 8);
  if (!lastscenario) {
    Call_Back_Delay(show, 85);
  }
#ifdef FRENCH
  show.page().view().FillRect(xcoord, 12, xcoord + 6 * 16 + 10, 20, kBlack);
  show.text_page().view().FillRect(2 * xcoord, 24, 2 * (xcoord + 6 * 16 + 10),
                                   40, kBlack);
#else
  show.page().view().FillRect(xcoord, 12, xcoord + (6 * 16), 20, kBlack);
  show.text_page().view().FillRect(2 * xcoord, 24, 2 * (xcoord + (6 * 16)), 40,
                                   kBlack);
#endif

  Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(), {});

  startframe = base::At(base::At(CountryArray, scenario).ContAnim,
                        static_cast<int>(TheWorld().scen_dir()));

  /*
  ** Now print the text over the page
  */
  TheAudio().Play(text2, 255, TheOptions().Normalize_Sound(90));
  Alloc_Object(new ScorePrintClass(show, TXT_MAP_LOCATE, 0, 160, greenpal));
  Call_Back_Delay(show, 20);
  Alloc_Object(
      new ScorePrintClass(show, TXT_MAP_NEXT_MISSION, 0, 170, greenpal));
#if (defined(GERMAN) || defined(FRENCH))
  Call_Back_Delay(show, 20);
  Alloc_Object(new ScorePrintClass(show, TXT_MAP_NEXT_MISS2, 0, 180, greenpal));
#endif
  Call_Back_Delay(show, 50);

  /*
  ** If we're on the last scenario, erase that text before doing the crosshairs
  */
  if (lastscenario) {
#if (defined(GERMAN) || defined(FRENCH))
    TheScreen().sys_mem_page().FillRect(0, 160, 20 * 6, 186, kTBlack);
    show.page().view().FillRect(0, 160, 20 * 6, 186, kTBlack);
    show.text_page().view().FillRect(0, 320, 40 * 6, 372, kBlack);
    TheScreen().visible_view().FillRect(0, 320, 40 * 6, 372, kTBlack);
    TheScreen().hidden_view().FillRect(0, 320, 40 * 6, 372, kTBlack);
#else
    TheScreen().sys_mem_page().view().FillRect(0, 160, 20 * 6, 176, kTBlack);
    show.page().view().FillRect(0, 160, 20 * 6, 176, kTBlack);
    show.text_page().view().FillRect(0, 320, 40 * 6, 352, kBlack);
    TheScreen().visible_view().FillRect(0, 320, 40 * 6, 352, kTBlack);
    TheScreen().hidden_view().FillRect(0, 320, 40 * 6, 352, kTBlack);
#endif
    show.ClearTextRects();
    Bit_It_In(show, 0, 0, 320, 200, &TheScreen().sys_mem_page(), &show.page());
  }

  /*
  ** Fix up the palette that seems different for the last scenario
  */
  if (lastscenario) {
    if (house == HOUSE_GOOD) {
      Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(),
                           "LASTSCNG.PAL");
    } else {
      Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(),
                           "LASTSCNB.PAL");
    }
  }

  int q = 0;
  for (frame = 0; frame < (lastscenario ? progress.frame_count() - 4 : 13);
       frame++) {
    if (!frame) {
      TheAudio().Play(beepy3, 255, TheOptions().Normalize_Sound(90));
    }
    if (frame == 2) {
      TheAudio().Play(beepy3, 255, TheOptions().Normalize_Sound(90));
    }
    if (frame == 6) {
      TheAudio().Play(newtarg1, 255, TheOptions().Normalize_Sound(90));
    }

    if (lastscenario) {
      switch (frame) {
        case 23:
          if (house == HOUSE_GOOD) {
            Alloc_Object(new MultiStagePrintClass(
                show, Text_String(TXT_ENHANCING_IMAGE), 0, 10, _othergreenpal));
          } else {
#ifdef FRENCH
            Alloc_Object(
                new MultiStagePrintClass(show, Text_String(TXT_ENHANCING_IMAGE),
                                         180, 10, _othergreenpal));
#else
            Alloc_Object(
                new MultiStagePrintClass(show, Text_String(TXT_ENHANCING_IMAGE),
                                         210, 10, _othergreenpal));
#endif  //(FRENCH)
          }
          // Frame 35 blacks out the region this frame starts printing into,
          // so this case has to stop here.
          break;

        case 35:
          if (house == HOUSE_GOOD) {
            show.text_page().view().FillRect(
                0, 20,
                2 * StringPixelWidth(score_font,
                                     Text_String(TXT_ENHANCING_IMAGE)),
                2 * (10 + 12), kBlack);
          } else {
#ifdef FRENCH
            show.text_page().view().FillRect(
                360, 20,
                2 * (180 + StringPixelWidth(score_font,
                                            Text_String(TXT_ENHANCING_IMAGE))),
                2 * (10 + 12), kBlack);
#else
            show.text_page().view().FillRect(
                420, 20,
                2 * (210 + StringPixelWidth(score_font,
                                            Text_String(TXT_ENHANCING_IMAGE))),
                2 * (10 + 12), kBlack);
#endif  //(FRENCH)
          }
          break;

        case 36:
          if (house == HOUSE_GOOD) {
            show.text_page().view().FillRect(
                0, 20,
                2 * StringPixelWidth(score_font,
                                     Text_String(TXT_ENHANCING_IMAGE)),
                2 * (10 + 12), kTBlack);
          } else {
#ifdef FRENCH
            show.text_page().view().FillRect(
                360, 20,
                2 * (180 + StringPixelWidth(score_font,
                                            Text_String(TXT_ENHANCING_IMAGE))),
                2 * (10 + 12), kTBlack);
#else
            show.text_page().view().FillRect(
                420, 20,
                2 * (210 + StringPixelWidth(score_font,
                                            Text_String(TXT_ENHANCING_IMAGE))),
                2 * (10 + 12), kTBlack);
#endif  //(FRENCH)
          }
          break;
        default:
          break;
      }
    }

    progress.DrawFrame(show.page().view(), startframe + frame);
    Call_Back_Delay(show, 6);
    /* Cause it to cycle on the flashing on the country for a little while */
    if (!lastscenario && frame == 4 && q < 4) {
      frame = 2;
      q++;
    }
  }

  int selection = 0;
  int color = 0;
  // erase the "Locating Coordinates" message...
  TheAudio().Play(beepy6, 255, TheOptions().Normalize_Sound(90));
  if (!lastscenario) {
#if (defined(GERMAN) || defined(FRENCH))
    TheScreen().sys_mem_page().FillRect(0, 160, 20 * 6, 186, kTBlack);
    show.page().view().FillRect(0, 160, 20 * 6, 186, kTBlack);
    show.text_page().view().FillRect(0, 320, 40 * 6, 372, kBlack);
#else
    TheScreen().sys_mem_page().view().FillRect(0, 160, 20 * 6, 176, kTBlack);
    show.page().view().FillRect(0, 160, 20 * 6, 176, kTBlack);
    show.text_page().view().FillRect(0, 320, 40 * 6, 352, kBlack);
#endif
  }

  Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(), {});

  /*
  ** Now the crosshairs are over the target countries - loop until a
  ** selection is made?
  */
  int done = 0;

  if (house == HOUSE_GOOD) {
    if (const auto file =
            OpenGameFile(lastscenario ? "CLICK_EB.CPS" : "CLICK_E.CPS")) {
      Load_Uncompress(*file, TheScreen().sys_mem_page().bytes(),
                      TheScreen().sys_mem_page().bytes(), {});
    }
  } else {
    if (const auto file =
            OpenGameFile(lastscenario ? "CLICK_SA.CPS" : "CLICK_A.CPS")) {
      Load_Uncompress(*file, TheScreen().sys_mem_page().bytes(),
                      TheScreen().sys_mem_page().bytes(), {});
    }
    if (lastscenario) {
      attackxcoord = 200;
    }
  }

  TheAudio().Play(text2, 255, TheOptions().Normalize_Sound(90));
  Alloc_Object(
      new ScorePrintClass(show, TXT_MAP_SELECT, attackxcoord, 160, greenpal));
  Cycle_Call_Back_Delay(show, 16, progresspalette);
  Alloc_Object(new ScorePrintClass(show, TXT_MAP_TO_ATTACK, attackxcoord, 170,
                                   greenpal));
  Cycle_Call_Back_Delay(show, 24, progresspalette);
  while (Get_Mouse_State() > 0) {
    Show_Mouse();
  }

  Keyboard::Clear();
  while (!done) {
    Cycle_Call_Back_Delay(show, 1, progresspalette);

    // Check for the mouse button
    if (Keyboard::Check() && KeyCode(Keyboard::Get()) == KN_LMOUSE) {
      for (selection = 0;
           selection < base::At(base::At(CountryArray, scenario).Choices,
                                static_cast<int>(TheWorld().scen_dir()));
           selection++) {
        color = TheScreen().sys_mem_page().view().GetPixel(Get_Mouse_X() / 2,
                                                           Get_Mouse_Y() / 2);

        /*
        ** Special hack for Egypt the second time through
        */
        if ((base::At(base::At(base::At(CountryArray, scenario).CountryColor,
                               static_cast<int>(TheWorld().scen_dir())),
                      selection) == 0xA0) &&
            (color == 0x80 || color == 0x81)) {
          color = 0xA0;
        }

        if (base::At(base::At(base::At(CountryArray, scenario).CountryColor,
                              static_cast<int>(TheWorld().scen_dir())),
                     selection) == color) {
          TheAudio().Play(world2, 255, TheOptions().Normalize_Sound(90));
          done = 1;
          break;
        }
        TheAudio().Play(scold1, 255, TheOptions().Normalize_Sound(90));
      }
    }
  }
  TheWorld().scen_var() =
      base::At(base::At(base::At(CountryArray, scenario).CountryVariant,
                        static_cast<int>(TheWorld().scen_dir())),
               selection);
  TheWorld().scen_dir() =
      base::At(base::At(base::At(CountryArray, scenario).CountryDir,
                        static_cast<int>(TheWorld().scen_dir())),
               selection);

  if (!lastscenario) {
    progress.Close();

    /*
    ** Now it's time to highlight the country we're going to.
    */
    auto countryshape = MixArchive::RetrieveData(
        house == HOUSE_GOOD ? "COUNTRYE.SHP" : "COUNTRYA.SHP");

    Hide_Mouse();
    // erase "Select country to attack"
    show.page().view().FillRect(attackxcoord, 160, attackxcoord + (17 * 6), 178,
                                kBlack);
    show.text_page().view().FillRect(
        2 * attackxcoord, 320, 2 * (attackxcoord + (17 * 6)), 2 * 178, kBlack);
#if (defined(GERMAN) || defined(FRENCH))
    show.page().view().FillRect(attackxcoord + (17 * 6), 160,
                                attackxcoord + (21 * 6), 178, kBlack);
    show.text_page().view().FillRect(2 * attackxcoord + (17 * 6 * 2), 320,
                                     2 * (attackxcoord + (21 * 6)), 2 * 178,
                                     kBlack);
#endif  // GERMAN

    Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(), {});

    /*
    ** Draw the country's shape in non-fading colors
    */
    PixelView& view = TheScreen().sys_mem_page().view();
    europe->view().BlitTo(view);
    const int shape =
        base::At(base::At(base::At(CountryArray, scenario).CountryShape,
                          static_cast<int>(TheWorld().scen_dir())),
                 selection);
    const int xshuffled_rows = shape + (house == HOUSE_GOOD ? 0 : 18);
    CC_Draw_Shape(view, countryshape, shape,
                  base::At(_countryx, xshuffled_rows),
                  base::At(_countryy, xshuffled_rows), WINDOW_MAIN,
                  SHAPE_WIN_REL | SHAPE_CENTER, {}, {});
    TheScreen().sys_mem_page().view().BlitTo(show.page().view());
    Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(), {});

    /*
    ** Now clear the palette of all but the country's colors, and fade
    ** the palette down
    */
    if (const auto file = OpenGameFile("DARK_E.PAL")) {
      file->Read(localpalette, 768);
    }
    //		Load_Data("DARK_E.PAL", localpalette, 768);
    Increase_Palette_Luminance(localpalette, 30, 30, 30, 63);
    Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(),
                         "MAP_LOC2.PAL");
    Fade_Palette_To(localpalette, kFadePaletteMedium, Call_Back);

    countryshape = {};

    Print_Statistics(show, color % 128, base::At(_countryx, xshuffled_rows),
                     base::At(_countryy, xshuffled_rows));
  } else {
    if (const auto file =
            OpenGameFile(house == HOUSE_GOOD ? "DARK_B.PAL" : "DARK_SA.PAL")) {
      file->Read(localpalette, 768);
    }
    Increase_Palette_Luminance(localpalette, 30, 30, 30, 63);
    Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(),
                         "MAP_LOC3.PAL");
    Set_Palette(localpalette);
    //		Load_Data(house == HOUSE_GOOD ? "DARK_B.PAL" : "DARK_SA.PAL",
    // localpalette, 768);

    Hide_Mouse();
#if (defined(GERMAN) || defined(FRENCH))
    show.page().view().FillRect(attackxcoord, 160, 319, 178,
                                kBlack);  // erase "Select country to attack"
    show.text_page().view().FillRect(
        2 * attackxcoord, 320, 639, 356,
        kBlack);  // erase "Select country to attack"
#else
    show.page().view().FillRect(attackxcoord, 160, attackxcoord + (17 * 6), 199,
                                kBlack);  // erase "Select country to attack"
    show.text_page().view().FillRect(
        2 * attackxcoord, 320, 2 * (attackxcoord + (17 * 6)), 398,
        kBlack);  // erase "Select country to attack"
#endif
    Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(), {});

    progress.DrawFrame(show.page().view(), progress.frame_count() - 1);
    Set_Palette(localpalette);
    progress.Close();
    show.page().view().BlitTo(TheScreen().sys_mem_page().view());
    Print_Statistics(show, 20, 160, house == HOUSE_GOOD ? 0 : 160);
  }

  TheTheme().Queue_Song(THEME_NONE);
  Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium, nullptr);
  delete europe;

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
void Print_Statistics(Presentation& show, int country, int xpos, int ypos) {
  int index = 0;
  int newx = 0;
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

  static const unsigned char greenpal[] = {0,    0x41, 0x42, 0x43, 0x44, 0x44,
                                           0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
                                           0x44, 0x44, 0x44, 0x44};
  // static char const
  // _greenpal[]={0,1,0x42,3,0x43,5,0x44,7,0x44,9,10,1,12,13,0x41,15};
  static char _deststr[16];

  /* Change to the six-point font for Text_Print */

#ifdef GERMAN
  xpos = 8;
#else
  xpos = xpos > 128 ? 8 : 136;
#endif
  ypos = ypos > 100 ? 8 : 104 - 6;
  if (ThePlayer()->Class->House == HOUSE_GOOD) {
    Alloc_Object(new ScorePrintClass(
        show, base::At(_countryname, base::At(GDIStats, country).nameindex),
        xpos, ypos, greenpal));
    Call_Back_Delay(
        show, static_cast<int>(
                  std::string_view(
                      Text_String(base::At(
                          _countryname, base::At(GDIStats, country).nameindex)))
                      .size()) *
                  3);
    ypos += 16;
    for (index = 0; index < 7; index++) {
      Alloc_Object(new ScorePrintClass(show, base::At(_gdistatnames, index),
                                       xpos, ypos, greenpal));
      Call_Back_Delay(
          show,
          static_cast<int>(
              std::string_view(Text_String(base::At(_gdistatnames, index) + 3))
                  .size()));
      newx =
          xpos +
          (6 * static_cast<int>(
                   std::string_view(Text_String(base::At(_gdistatnames, index)))
                       .size()));
      switch (index) {
        case 0:
          Alloc_Object(new ScorePrintClass(
              show, base::At(GDIStats, country).pop, newx, ypos, greenpal));
          break;
        case 1:
          Alloc_Object(new ScorePrintClass(
              show, base::At(GDIStats, country).area, newx, ypos, greenpal));
          break;
        case 2:
          Alloc_Object(new ScorePrintClass(
              show, base::At(GDIStats, country).capital, newx, ypos, greenpal));
          break;
        case 3:
          Alloc_Object(new ScorePrintClass(
              show, base::At(_govtnames, base::At(GDIStats, country).govt),
              newx, ypos, greenpal));
          break;
        case 4:
          Alloc_Object(new ScorePrintClass(
              show, base::At(GDIStats, country).gdp, newx, ypos, greenpal));
          break;
        case 5:
          Alloc_Object(new ScorePrintClass(show,
                                           base::At(GDIStats, country).conflict,
                                           newx, ypos, greenpal));
          break;
        case 6:
          Alloc_Object(new ScorePrintClass(
              show, base::At(_armynames, base::At(GDIStats, country).military),
              newx, ypos, greenpal));
          break;
        default:
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

    Alloc_Object(new ScorePrintClass(
        show, base::At(_countryname, base::At(NodStats, country).nameindex),
        xpos, ypos, greenpal));
    Call_Back_Delay(
        show, static_cast<int>(
                  std::string_view(
                      Text_String(base::At(
                          _countryname, base::At(NodStats, country).nameindex)))
                      .size()) *
                  3);
    ypos += 16;
    for (index = 0; index < 9; index++) {
      Alloc_Object(new ScorePrintClass(show, base::At(_nodstatnames, index),
                                       xpos, ypos, greenpal));
      Call_Back_Delay(
          show,
          static_cast<int>(
              std::string_view(Text_String(base::At(_nodstatnames, index) + 3))
                  .size()));
      newx =
          xpos +
          (6 * static_cast<int>(
                   std::string_view(Text_String(base::At(_nodstatnames, index)))
                       .size()));
      switch (index) {
        case 0:
          Alloc_Object(new ScorePrintClass(
              show, base::At(NodStats, country).pop, newx, ypos, greenpal));
          break;
        case 1:
          absl::SNPrintF(_deststr, sizeof(_deststr), "%d%%",
                         base::At(NodStats, country).expendable);
          Alloc_Object(
              new ScorePrintClass(show, _deststr, newx, ypos, greenpal));
          break;
        case 2:
          Alloc_Object(new ScorePrintClass(
              show, base::At(NodStats, country).capital, newx, ypos, greenpal));
          break;
        case 3:
          Alloc_Object(new ScorePrintClass(
              show, base::At(_govtnames, base::At(NodStats, country).govt),
              newx, ypos, greenpal));
          break;
        case 4:
#ifdef FIX_ME_LATER
          absl::SNPrintF(_deststr, sizeof(_deststr), "%s %d%%",
                         LowMedHiStr(NodStats[country].corruptible),
                         NodStats[country].corruptible);
#endif  // FIX_ME_LATER
          absl::SNPrintF(_deststr, sizeof(_deststr), "%d%%",
                         base::At(NodStats, country).corruptible);
          Alloc_Object(
              new ScorePrintClass(show, _deststr, newx, ypos, greenpal));
          break;
        case 5:
          Alloc_Object(new ScorePrintClass(
              show, base::At(NodStats, country).worth, newx, ypos, greenpal));
          break;
        case 6:
          Alloc_Object(new ScorePrintClass(show,
                                           base::At(NodStats, country).conflict,
                                           newx, ypos, greenpal));
          break;
        case 7:
          Alloc_Object(new ScorePrintClass(
              show, base::At(_military, base::At(NodStats, country).military),
              newx, ypos, greenpal));
          break;
        case 8:
          absl::SNPrintF(_deststr, sizeof(_deststr), "%d%%",
                         base::At(NodStats, country).probability);
          Alloc_Object(
              new ScorePrintClass(show, _deststr, newx, ypos, greenpal));
          break;
        default:
          break;
      }
      ypos += 8;
    }
  }

#ifdef FRENCH
  Alloc_Object(
      new ScorePrintClass(show, TXT_MAP_CLICK2, 94, 193 - 6, greenpal));
#else
  Alloc_Object(new ScorePrintClass(show, TXT_MAP_CLICK2, 160 - (17 * 3),
                                   193 - 6, greenpal));
#endif

  int done = 0;
  while (!done) {
    done = 1;
    for (auto& ScoreObj : ScoreObjs) {
      if (ScoreObj) {
        done = 0;
        Call_Back_Delay(show, 1);
      }
    }
  }
  Keyboard::Clear();
  while (Keyboard::Check()) {
    Keyboard::Clear();
  }
  while (!Keyboard::Check() && !ControlQ) {
    Call_Back_Delay(show, 1);
  }
  Keyboard::Clear();
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
                      PixelBuffer* src, PixelBuffer* dest) {
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

    x = GameRandomRange(0, w - 1);
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

    y = GameRandomRange(0, h - 1);
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
      src->BlitTo(*dest, (srcx + x) << 3, srcy + (y << 1), (destx + x) << 3,
                  desty + (y << 1), 1 << 3, 2);
    }
  }
}
#endif

void Cycle_Call_Back_Delay(Presentation& show, int time,
                           std::span<unsigned char> pal) {
  static int _counter;

  while (time--) {
    _counter = (_counter + 1) % 4;

    if (_counter == 0) {
      const unsigned char r = base::At(pal, (249 * 3) + 0);
      const unsigned char g = base::At(pal, (249 * 3) + 1);
      const unsigned char b = base::At(pal, (249 * 3) + 2);

      for (int i = 249; i < 254; i++) {
        base::At(pal, (base::ToSize(i) * 3) + 0) =
            base::At(pal, (base::ToSize(i + 1) * 3) + 0);
        base::At(pal, (base::ToSize(i) * 3) + 1) =
            base::At(pal, (base::ToSize(i + 1) * 3) + 1);
        base::At(pal, (base::ToSize(i) * 3) + 2) =
            base::At(pal, (base::ToSize(i + 1) * 3) + 2);
      }
      base::At(pal, (254 * 3) + 0) = r;
      base::At(pal, (254 * 3) + 1) = g;
      base::At(pal, (254 * 3) + 2) = b;

      Set_Palette(pal);
    }
    Call_Back_Delay(show, 1);
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
void Bit_It_In(Presentation& show, const int x, const int y, const int w,
               const int h, PixelBuffer* src, PixelBuffer* dest,
               const int delay, const bool dagger) {
  // Build shuffled coordinate tables so pixels are copied in random order,
  // creating a dissolve transition where the new image materializes from
  // scattered dots rather than appearing all at once.
  int shuffled_cols[320];
  int shuffled_rows[200];
  if (w < 0 || w > 320 || h < 0 || h > 200) {
    return;
  }
  const auto x_span = std::span(shuffled_cols).first(base::ToSize(w));
  const auto y_span = std::span(shuffled_rows).first(base::ToSize(h));
  std::ranges::iota(x_span, 0);
  std::ranges::iota(y_span, 0);

  absl::BitGen gen;
  absl::c_shuffle(x_span, gen);
  absl::c_shuffle(y_span, gen);

  // Copy w pixels per iteration in shuffled order. The Call_Back_Delay
  // between iterations processes the game loop (including screen
  // rendering), so each batch of random pixels becomes visible before
  // the next batch is drawn.
  for (int line = 0; line < h; line++) {
    int row_offset = line;
    if (line % 2 != 0) {
      int remaining = delay;
      do {
        Call_Back_Delay(show, remaining ? 1 : 0);
      } while (remaining--);
    } else {
      Call_Back();
    }

    // Both views stay locked for the whole scatter: the destination is the
    // window's surface, so locking it per pixel would lock and unlock an SDL
    // surface thousands of times a frame.
    if (src->view().Lock()) {
      if (dest->view().Lock()) {
        // Each pixel uses a shuffled x and a wrapping y offset, so pixels
        // scatter across the entire image rather than filling row by row.
        for (int col = 0; col < w; col++) {
          const int px = x + base::At(shuffled_cols, col);
          const int py = y + base::At(shuffled_rows, row_offset);
          row_offset++;
          if (row_offset >= h) {
            row_offset = 0;
          }

          dest->view().PutPixelLocked(
              px, py,
              static_cast<unsigned char>(src->view().GetPixelLocked(px, py)));
        }
        if (dagger) {
          // Overlay a downward-pointing wedge from screen center (x=160),
          // expanding one pixel wider per row. This adds a dagger-shaped
          // reveal on top of the random dissolve.
          // NOTE: Ignores x/y/w/h and assumes a full 320-wide screen.
          // Only used with full-screen (0,0,320,200) dissolves.
          for (int row = line; row >= 0; row--) {
            const int offset = line - row;
            const int x_left = 160 - offset;
            const int x_right = 160 + offset;
            dest->view().PutPixelLocked(
                x_left, row,
                static_cast<unsigned char>(
                    src->view().GetPixelLocked(x_left, row)));
            dest->view().PutPixelLocked(
                x_right, row,
                static_cast<unsigned char>(
                    src->view().GetPixelLocked(x_right, row)));
          }
        }
        dest->view().Unlock();
      }
      src->view().Unlock();
    }
  }
}
