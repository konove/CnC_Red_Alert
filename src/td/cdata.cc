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

/* $Header:   F:\projects\c&c\vcs\code\cdata.cpv   2.18   16 Oct 1995 16:50:22
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CDATA.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 16, 1994 *
 *                                                                                             *
 *                  Last Update : July 29, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * TemplateTypeClass::Create_And_Place -- Creates and places a
 *template object on the map.   * TemplateTypeClass::Create_One_Of -- Creates an
 *object of this template type.              * TemplateTypeClass::Display --
 *Displays a generic representation of template.              *
 *   TemplateTypeClass::From_Name -- Determine template from ASCII name. *
 *   TemplateTypeClass::Init -- Loads graphic data for templates. *
 *   TemplateTypeClass::Occupy_List -- Determines occupation list. *
 *   TemplateTypeClass::One_Time -- Performs one-time initialization *
 *   TemplateTypeClass::Prep_For_Add -- Prepares to add template to scenario. *
 *   TemplateTypeClass::TemplateTypeClass -- Constructor for template type
 *objects.            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include <cstddef>
#include <cstdint>
#include <filesystem>

#include "base/enum_array.h"
#include "base/numeric.h"
#include "port/ex_string.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/iconcach.h"
#include "sdllib/memflag.h"
#include "sdllib/tile.h"
#include "sdllib/ww_win.h"
#include "td/conquer.h"
#include "td/const.h"
#include "td/defines.h"
#include "td/display_constants.h"
#include "td/externs.h"
#include "td/house.h"
#include "td/jshell.h"
#include "td/mapedit.h"
#include "td/object.h"
#include "td/template.h"
#include "td/type.h"
#include "tech/mix_archive.h"

static const char slope00000001[] = {7, -1};
static const char slope000000101[] = {6, 8, -1};
static const char slope00000011[] = {6, 7, -1};
static const char slope0000001[] = {6, -1};
static const char slope000001001[] = {5, 8, -1};
static const char slope000001[] = {5, -1};
static const char slope000101[] = {3, 5, -1};
static const char slope00011010000100000001000011[] = {3,  4,  6,  11,
                                                       19, 25, 25, -1};
static const char slope00011010010100100001000011[] = {3,  4,  6,  9,  11,
                                                       14, 19, 24, 25, -1};
static const char slope0001[] = {3, -1};
static const char slope001001001[] = {2, 5, 8, -1};
static const char slope00110000000011[] = {2, 3, 12, 13, -1};
static const char slope00110010010011[] = {2, 3, 6, 9, 12, 13, -1};
static const char slope001111001[] = {2, 3, 4, 5, 8, -1};
static const char slope0011[] = {2, 3, -1};
static const char slope001[] = {2, -1};
static const char slope01000000000000000000001[] = {1, 22, -1};
static const char slope01000000100000010000001[] = {1, 8, 15, 22, -1};
static const char slope0111[] = {1, 2, 3, -1};
static const char slope01[] = {1, -1};
static const char slope1001001[] = {0, 3, 6, -1};
static const char slope1001[] = {0, 3, -1};
static const char slope1100000000000000001100011[] = {0, 1, 18, 19, 23, 24, -1};
static const char slope1100001000001000001100011[] = {0,  1,  6,  12, 18,
                                                      19, 23, 24, -1};
static const char slope1101101[] = {0, 1, 3, 4, 6, -1};
static const char slope1101[] = {0, 1, 3, -1};
static const char slope111[] = {0, 1, 2, -1};
static const char slope111010011[] = {0, 1, 2, 4, 7, 8, -1};
static const char slope11101[] = {0, 1, 2, 4, -1};
static const char slope111111011[] = {0, 1, 2, 3, 4, 5, 7, 8, -1};
static const char slope11111111[] = {0, 1, 2, 3, 4, 5, 6, 7, -1};
static const char slope111111[] = {0, 1, 2, 3, 4, 5, -1};
static const char slope1[] = {0, -1};

static const TemplateTypeClass Empty(TEMPLATE_CLEAR1,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate | kTheaterFlagJungle,
                                     "CLEAR1", TXT_CLEAR, LAND_CLEAR, 1, 1,
                                     LAND_CLEAR, nullptr);
static const TemplateTypeClass Clear(TEMPLATE_CLEAR1,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate | kTheaterFlagJungle,
                                     "CLEAR1", TXT_CLEAR, LAND_CLEAR, 1, 1,
                                     LAND_CLEAR, nullptr);
static const TemplateTypeClass Road1(TEMPLATE_ROAD1,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate,
                                     "D01", TXT_ROAD, LAND_CLEAR, 2, 2,
                                     LAND_CLEAR, nullptr);
static const TemplateTypeClass Road2(TEMPLATE_ROAD2,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate,
                                     "D02", TXT_ROAD, LAND_CLEAR, 2, 2,
                                     LAND_CLEAR, nullptr);
static const TemplateTypeClass Road3(TEMPLATE_ROAD3,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate,
                                     "D03", TXT_ROAD, LAND_CLEAR, 1, 2,
                                     LAND_CLEAR, nullptr);
static const TemplateTypeClass Road4(TEMPLATE_ROAD4,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate,
                                     "D04", TXT_ROAD, LAND_CLEAR, 2, 2,
                                     LAND_CLEAR, nullptr);
static const TemplateTypeClass Road5(TEMPLATE_ROAD5,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate,
                                     "D05", TXT_ROAD, LAND_CLEAR, 3, 4,
                                     LAND_CLEAR, nullptr);
static const TemplateTypeClass Road6(TEMPLATE_ROAD6,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate,
                                     "D06", TXT_ROAD, LAND_CLEAR, 2, 3,
                                     LAND_CLEAR, nullptr);
static const TemplateTypeClass Road7(TEMPLATE_ROAD7,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate,
                                     "D07", TXT_ROAD, LAND_CLEAR, 3, 2,
                                     LAND_CLEAR, nullptr);
static const TemplateTypeClass Road8(TEMPLATE_ROAD8,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate,
                                     "D08", TXT_ROAD, LAND_CLEAR, 3, 2,
                                     LAND_CLEAR, nullptr);
static const TemplateTypeClass Road9(TEMPLATE_ROAD9,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate,
                                     "D09", TXT_ROAD, LAND_CLEAR, 4, 3,
                                     LAND_CLEAR, nullptr);
static const TemplateTypeClass Road10(TEMPLATE_ROAD10,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D10", TXT_ROAD, LAND_CLEAR, 4, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road11(TEMPLATE_ROAD11,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D11", TXT_ROAD, LAND_CLEAR, 2, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road12(TEMPLATE_ROAD12,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D12", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road13(TEMPLATE_ROAD13,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D13", TXT_ROAD, LAND_CLEAR, 4, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road14(TEMPLATE_ROAD14,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D14", TXT_ROAD, LAND_CLEAR, 3, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road15(TEMPLATE_ROAD15,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D15", TXT_ROAD, LAND_CLEAR, 3, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road16(TEMPLATE_ROAD16,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D16", TXT_ROAD, LAND_CLEAR, 3, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road17(TEMPLATE_ROAD17,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D17", TXT_ROAD, LAND_CLEAR, 3, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road18(TEMPLATE_ROAD18,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D18", TXT_ROAD, LAND_CLEAR, 3, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road19(TEMPLATE_ROAD19,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D19", TXT_ROAD, LAND_CLEAR, 3, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road20(TEMPLATE_ROAD20,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D20", TXT_ROAD, LAND_CLEAR, 3, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road21(TEMPLATE_ROAD21,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D21", TXT_ROAD, LAND_CLEAR, 3, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road22(TEMPLATE_ROAD22,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D22", TXT_ROAD, LAND_CLEAR, 3, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road23(TEMPLATE_ROAD23,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D23", TXT_ROAD, LAND_CLEAR, 3, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road24(TEMPLATE_ROAD24,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D24", TXT_ROAD, LAND_CLEAR, 3, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road25(TEMPLATE_ROAD25,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D25", TXT_ROAD, LAND_CLEAR, 3, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road26(TEMPLATE_ROAD26,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D26", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road27(TEMPLATE_ROAD27,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D27", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road28(TEMPLATE_ROAD28,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D28", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road29(TEMPLATE_ROAD29,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D29", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road30(TEMPLATE_ROAD30,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D30", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road31(TEMPLATE_ROAD31,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D31", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road32(TEMPLATE_ROAD32,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D32", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road33(TEMPLATE_ROAD33,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D33", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road34(TEMPLATE_ROAD34,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D34", TXT_ROAD, LAND_CLEAR, 3, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road35(TEMPLATE_ROAD35,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D35", TXT_ROAD, LAND_CLEAR, 3, 3,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road36(TEMPLATE_ROAD36,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D36", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road37(TEMPLATE_ROAD37,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D37", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road38(TEMPLATE_ROAD38,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D38", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road39(TEMPLATE_ROAD39,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D39", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road40(TEMPLATE_ROAD40,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D40", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road41(TEMPLATE_ROAD41,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D41", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road42(TEMPLATE_ROAD42,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D42", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Road43(TEMPLATE_ROAD43,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "D43", TXT_ROAD, LAND_CLEAR, 2, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Water(TEMPLATE_WATER,
                                     kTheaterFlagWinter | kTheaterFlagTemperate |
                                         kTheaterFlagDesert,
                                     "W1", TXT_WATER, LAND_WATER, 1, 1,
                                     LAND_WATER, nullptr);
static const TemplateTypeClass Water2(TEMPLATE_WATER2,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "W2", TXT_WATER, LAND_WATER, 2, 2,
                                      LAND_WATER, nullptr);
static const TemplateTypeClass Shore1(TEMPLATE_SHORE1,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "SH1", TXT_WATER, LAND_WATER, 3, 3,
                                      LAND_BEACH, slope111111);
static const TemplateTypeClass Shore2(TEMPLATE_SHORE2,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "SH2", TXT_WATER, LAND_ROCK, 3, 3,
                                      LAND_BEACH, slope111);
static const TemplateTypeClass Shore3(TEMPLATE_SHORE3,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "SH3", TXT_WATER, LAND_ROCK, 1, 1,
                                      LAND_WATER, nullptr);
static const TemplateTypeClass Shore4(TEMPLATE_SHORE4,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "SH4", TXT_WATER, LAND_ROCK, 2, 1,
                                      LAND_WATER, nullptr);
static const TemplateTypeClass Shore5(TEMPLATE_SHORE5,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "SH5", TXT_WATER, LAND_WATER, 3, 3,
                                      LAND_BEACH, slope111111);
static const TemplateTypeClass Shore6(TEMPLATE_SHORE6,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "SH6", TXT_WATER, LAND_WATER, 3, 3,
                                      LAND_BEACH, slope111111);
static const TemplateTypeClass Shore7(TEMPLATE_SHORE7,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "SH7", TXT_WATER, LAND_WATER, 2, 2,
                                      LAND_BEACH, slope1);
static const TemplateTypeClass Shore8(TEMPLATE_SHORE8,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "SH8", TXT_WATER, LAND_WATER, 3, 3,
                                      LAND_BEACH, slope11111111);
static const TemplateTypeClass Shore9(TEMPLATE_SHORE9,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "SH9", TXT_WATER, LAND_WATER, 3, 3,
                                      LAND_BEACH, slope111111011);
static const TemplateTypeClass Shore10(TEMPLATE_SHORE10,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "SH10", TXT_WATER, LAND_WATER, 2, 2,
                                       LAND_BEACH, slope01);
static const TemplateTypeClass Shore11(TEMPLATE_SHORE11,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "SH11", TXT_WATER, LAND_WATER, 3, 3,
                                       LAND_BEACH, slope1001);
static const TemplateTypeClass Shore12(TEMPLATE_SHORE12,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "SH12", TXT_WATER, LAND_WATER, 3, 3,
                                       LAND_BEACH, slope000001001);
static const TemplateTypeClass Shore13(TEMPLATE_SHORE13,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "SH13", TXT_WATER, LAND_WATER, 3, 3,
                                       LAND_BEACH, slope0000001);
static const TemplateTypeClass Shore14(TEMPLATE_SHORE14,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "SH14", TXT_WATER, LAND_ROCK, 3, 3,
                                       LAND_BEACH, slope00000011);
static const TemplateTypeClass Shore15(TEMPLATE_SHORE15,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "SH15", TXT_WATER, LAND_ROCK, 3, 3,
                                       LAND_BEACH, slope000000101);
static const TemplateTypeClass Shore16(TEMPLATE_SHORE16,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "SH16", TXT_WATER, LAND_ROCK, 3, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Shore17(TEMPLATE_SHORE17,
                                       kTheaterFlagWinter | kTheaterFlagTemperate |
                                           kTheaterFlagDesert,
                                       "SH17", TXT_WATER, LAND_WATER, 2, 2,
                                       LAND_WATER, nullptr);
static const TemplateTypeClass Shore18(TEMPLATE_SHORE18,
                                       kTheaterFlagWinter | kTheaterFlagTemperate |
                                           kTheaterFlagDesert,
                                       "SH18", TXT_WATER, LAND_WATER, 2, 2,
                                       LAND_WATER, nullptr);
static const TemplateTypeClass Shore19(TEMPLATE_SHORE19, kTheaterFlagDesert,
                                       "SH19", TXT_WATER, LAND_ROCK, 3, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Shore20(TEMPLATE_SHORE20, kTheaterFlagDesert,
                                       "SH20", TXT_WATER, LAND_ROCK, 4, 1,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Shore21(TEMPLATE_SHORE21, kTheaterFlagDesert,
                                       "SH21", TXT_WATER, LAND_ROCK, 3, 1,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Shore22(TEMPLATE_SHORE22, kTheaterFlagDesert,
                                       "SH22", TXT_WATER, LAND_ROCK, 6, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Shore23(TEMPLATE_SHORE23, kTheaterFlagDesert,
                                       "SH23", TXT_WATER, LAND_ROCK, 2, 2,
                                       LAND_CLEAR, slope01);
static const TemplateTypeClass Shore24(TEMPLATE_SHORE24, kTheaterFlagDesert,
                                       "SH24", TXT_WATER, LAND_ROCK, 3, 3,
                                       LAND_CLEAR, slope000001);
static const TemplateTypeClass Shore25(TEMPLATE_SHORE25, kTheaterFlagDesert,
                                       "SH25", TXT_WATER, LAND_ROCK, 3, 2,
                                       LAND_CLEAR, slope0001);
static const TemplateTypeClass Shore26(TEMPLATE_SHORE26, kTheaterFlagDesert,
                                       "SH26", TXT_WATER, LAND_ROCK, 3, 2,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore27(TEMPLATE_SHORE27, kTheaterFlagDesert,
                                       "SH27", TXT_WATER, LAND_ROCK, 4, 1,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore28(TEMPLATE_SHORE28, kTheaterFlagDesert,
                                       "SH28", TXT_WATER, LAND_ROCK, 3, 1,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore29(TEMPLATE_SHORE29, kTheaterFlagDesert,
                                       "SH29", TXT_WATER, LAND_ROCK, 6, 2,
                                       LAND_CLEAR, slope00000001);
static const TemplateTypeClass Shore30(TEMPLATE_SHORE30, kTheaterFlagDesert,
                                       "SH30", TXT_WATER, LAND_ROCK, 2, 2,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore31(TEMPLATE_SHORE31, kTheaterFlagDesert,
                                       "SH31", TXT_WATER, LAND_ROCK, 3, 3,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore32(TEMPLATE_SHORE32,
                                       kTheaterFlagTemperate | kTheaterFlagWinter,
                                       "SH32", TXT_WATER, LAND_CLEAR, 3, 3,
                                       LAND_WATER, slope1);
static const TemplateTypeClass Shore33(TEMPLATE_SHORE33,
                                       kTheaterFlagTemperate | kTheaterFlagWinter,
                                       "SH33", TXT_WATER, LAND_CLEAR, 3, 3,
                                       LAND_WATER, slope001);
static const TemplateTypeClass Shore34(TEMPLATE_SHORE34,
                                       kTheaterFlagTemperate | kTheaterFlagWinter,
                                       "SH34", TXT_WATER, LAND_CLEAR, 3, 3,
                                       LAND_WATER, slope001001001);
static const TemplateTypeClass Shore35(TEMPLATE_SHORE35,
                                       kTheaterFlagTemperate | kTheaterFlagWinter,
                                       "SH35", TXT_WATER, LAND_CLEAR, 3, 3,
                                       LAND_WATER, slope1001001);
static const TemplateTypeClass Shore36(TEMPLATE_SHORE36, kTheaterFlagDesert,
                                       "SH36", TXT_WATER, LAND_CLEAR, 1, 1,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore37(TEMPLATE_SHORE37, kTheaterFlagDesert,
                                       "SH37", TXT_WATER, LAND_CLEAR, 1, 1,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore38(TEMPLATE_SHORE38, kTheaterFlagDesert,
                                       "SH38", TXT_WATER, LAND_CLEAR, 1, 1,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore39(TEMPLATE_SHORE39, kTheaterFlagDesert,
                                       "SH39", TXT_WATER, LAND_CLEAR, 1, 1,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore40(TEMPLATE_SHORE40, kTheaterFlagDesert,
                                       "SH40", TXT_WATER, LAND_WATER, 3, 3,
                                       LAND_CLEAR, slope1);
static const TemplateTypeClass Shore41(TEMPLATE_SHORE41, kTheaterFlagDesert,
                                       "SH41", TXT_WATER, LAND_CLEAR, 3, 3,
                                       LAND_WATER, slope1101101);
static const TemplateTypeClass Shore42(TEMPLATE_SHORE42, kTheaterFlagDesert,
                                       "SH42", TXT_WATER, LAND_WATER, 1, 2,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore43(TEMPLATE_SHORE43, kTheaterFlagDesert,
                                       "SH43", TXT_WATER, LAND_WATER, 1, 3,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore44(TEMPLATE_SHORE44, kTheaterFlagDesert,
                                       "SH44", TXT_WATER, LAND_WATER, 1, 3,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore45(TEMPLATE_SHORE45, kTheaterFlagDesert,
                                       "SH45", TXT_WATER, LAND_WATER, 1, 2,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore46(TEMPLATE_SHORE46, kTheaterFlagDesert,
                                       "SH46", TXT_WATER, LAND_WATER, 3, 3,
                                       LAND_CLEAR, slope1101);
static const TemplateTypeClass Shore47(TEMPLATE_SHORE47, kTheaterFlagDesert,
                                       "SH47", TXT_WATER, LAND_WATER, 3, 3,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore48(TEMPLATE_SHORE48, kTheaterFlagDesert,
                                       "SH48", TXT_WATER, LAND_WATER, 3, 3,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore49(TEMPLATE_SHORE49, kTheaterFlagDesert,
                                       "SH49", TXT_WATER, LAND_WATER, 3, 3,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore50(TEMPLATE_SHORE50, kTheaterFlagDesert,
                                       "SH50", TXT_WATER, LAND_WATER, 4, 3,
                                       LAND_CLEAR, slope00000001);
static const TemplateTypeClass Shore51(TEMPLATE_SHORE51, kTheaterFlagDesert,
                                       "SH51", TXT_WATER, LAND_WATER, 4, 3,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore52(TEMPLATE_SHORE52, kTheaterFlagDesert,
                                       "SH52", TXT_WATER, LAND_WATER, 4, 3,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore53(TEMPLATE_SHORE53, kTheaterFlagDesert,
                                       "SH53", TXT_WATER, LAND_WATER, 4, 3,
                                       LAND_CLEAR, slope11101);
static const TemplateTypeClass Shore54(TEMPLATE_SHORE54, kTheaterFlagDesert,
                                       "SH54", TXT_WATER, LAND_WATER, 3, 2,
                                       LAND_CLEAR, slope1);
static const TemplateTypeClass Shore55(TEMPLATE_SHORE55, kTheaterFlagDesert,
                                       "SH55", TXT_WATER, LAND_WATER, 3, 2,
                                       LAND_CLEAR, slope001);
static const TemplateTypeClass Shore56(TEMPLATE_SHORE56, kTheaterFlagDesert,
                                       "SH56", TXT_WATER, LAND_WATER, 3, 2,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore57(TEMPLATE_SHORE57, kTheaterFlagDesert,
                                       "SH57", TXT_WATER, LAND_WATER, 3, 2,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore58(TEMPLATE_SHORE58, kTheaterFlagDesert,
                                       "SH58", TXT_WATER, LAND_WATER, 2, 3,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore59(TEMPLATE_SHORE59, kTheaterFlagDesert,
                                       "SH59", TXT_WATER, LAND_WATER, 2, 3,
                                       LAND_CLEAR, slope1);
static const TemplateTypeClass Shore60(TEMPLATE_SHORE60, kTheaterFlagDesert,
                                       "SH60", TXT_WATER, LAND_WATER, 2, 3,
                                       LAND_CLEAR, slope000101);
static const TemplateTypeClass Shore61(TEMPLATE_SHORE61, kTheaterFlagDesert,
                                       "SH61", TXT_WATER, LAND_WATER, 2, 3,
                                       LAND_CLEAR, slope01);
static const TemplateTypeClass Shore62(TEMPLATE_SHORE62, kTheaterFlagDesert,
                                       "SH62", TXT_WATER, LAND_WATER, 6, 1,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Shore63(TEMPLATE_SHORE63, kTheaterFlagDesert,
                                       "SH63", TXT_WATER, LAND_WATER, 4, 1,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Boulder1(TEMPLATE_BOULDER1,
                                        kTheaterFlagWinter | kTheaterFlagTemperate |
                                            kTheaterFlagDesert,
                                        "B1", TXT_SLOPE, LAND_ROCK, 1, 1,
                                        LAND_ROCK, nullptr);
static const TemplateTypeClass Boulder2(TEMPLATE_BOULDER2,
                                        kTheaterFlagWinter | kTheaterFlagTemperate |
                                            kTheaterFlagDesert,
                                        "B2", TXT_SLOPE, LAND_ROCK, 2, 1,
                                        LAND_ROCK, nullptr);
static const TemplateTypeClass Boulder3(TEMPLATE_BOULDER3,
                                        kTheaterFlagWinter | kTheaterFlagTemperate,
                                        "B3", TXT_SLOPE, LAND_ROCK, 3, 1,
                                        LAND_ROCK, nullptr);
static const TemplateTypeClass Boulder4(TEMPLATE_BOULDER4, kTheaterFlagTemperate,
                                        "B4", TXT_SLOPE, LAND_ROCK, 1, 1,
                                        LAND_ROCK, nullptr);
static const TemplateTypeClass Boulder5(TEMPLATE_BOULDER5, kTheaterFlagTemperate,
                                        "B5", TXT_SLOPE, LAND_ROCK, 1, 1,
                                        LAND_ROCK, nullptr);
static const TemplateTypeClass Boulder6(TEMPLATE_BOULDER6, kTheaterFlagTemperate,
                                        "B6", TXT_SLOPE, LAND_ROCK, 1, 1,
                                        LAND_ROCK, nullptr);
static const TemplateTypeClass Slope1(TEMPLATE_SLOPE1,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "S01", TXT_SLOPE, LAND_ROCK, 2, 2,
                                      LAND_CLEAR, slope001);
static const TemplateTypeClass Slope2(TEMPLATE_SLOPE2,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "S02", TXT_SLOPE, LAND_ROCK, 2, 3,
                                      LAND_CLEAR, slope01);
static const TemplateTypeClass Slope3(TEMPLATE_SLOPE3,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "S03", TXT_SLOPE, LAND_ROCK, 2, 2,
                                      LAND_ROCK, nullptr);
static const TemplateTypeClass Slope4(TEMPLATE_SLOPE4,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "S04", TXT_SLOPE, LAND_ROCK, 2, 2,
                                      LAND_ROCK, nullptr);
static const TemplateTypeClass Slope5(TEMPLATE_SLOPE5,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "S05", TXT_SLOPE, LAND_ROCK, 2, 2,
                                      LAND_ROCK, nullptr);
static const TemplateTypeClass Slope6(TEMPLATE_SLOPE6,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "S06", TXT_SLOPE, LAND_ROCK, 2, 3,
                                      LAND_CLEAR, slope1);
static const TemplateTypeClass Slope7(TEMPLATE_SLOPE7,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "S07", TXT_SLOPE, LAND_ROCK, 2, 2,
                                      LAND_ROCK, nullptr);
static const TemplateTypeClass Slope8(TEMPLATE_SLOPE8,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "S08", TXT_SLOPE, LAND_ROCK, 2, 2,
                                      LAND_CLEAR, slope01);
static const TemplateTypeClass Slope9(TEMPLATE_SLOPE9,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "S09", TXT_SLOPE, LAND_ROCK, 3, 2,
                                      LAND_CLEAR, slope0001);
static const TemplateTypeClass Slope10(TEMPLATE_SLOPE10,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S10", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope11(TEMPLATE_SLOPE11,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S11", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope12(TEMPLATE_SLOPE12,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S12", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope13(TEMPLATE_SLOPE13,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S13", TXT_SLOPE, LAND_ROCK, 3, 2,
                                       LAND_CLEAR, slope000001);
static const TemplateTypeClass Slope14(TEMPLATE_SLOPE14,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S14", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_CLEAR, slope0111);
static const TemplateTypeClass Slope15(TEMPLATE_SLOPE15,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S15", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_CLEAR, slope01);
static const TemplateTypeClass Slope16(TEMPLATE_SLOPE16,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S16", TXT_SLOPE, LAND_ROCK, 2, 3,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope17(TEMPLATE_SLOPE17,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S17", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope18(TEMPLATE_SLOPE18,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S18", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope19(TEMPLATE_SLOPE19,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S19", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope20(TEMPLATE_SLOPE20,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S20", TXT_SLOPE, LAND_ROCK, 2, 3,
                                       LAND_CLEAR, slope000001);
static const TemplateTypeClass Slope21(TEMPLATE_SLOPE21,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S21", TXT_SLOPE, LAND_ROCK, 1, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope22(TEMPLATE_SLOPE22,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S22", TXT_SLOPE, LAND_ROCK, 2, 1,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope23(TEMPLATE_SLOPE23,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S23", TXT_SLOPE, LAND_ROCK, 3, 2,
                                       LAND_CLEAR, slope000001);
static const TemplateTypeClass Slope24(TEMPLATE_SLOPE24,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S24", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope25(TEMPLATE_SLOPE25,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S25", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope26(TEMPLATE_SLOPE26,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S26", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope27(TEMPLATE_SLOPE27,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S27", TXT_SLOPE, LAND_ROCK, 3, 2,
                                       LAND_CLEAR, slope0011);
static const TemplateTypeClass Slope28(TEMPLATE_SLOPE28,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S28", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope29(TEMPLATE_SLOPE29,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S29", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope30(TEMPLATE_SLOPE30,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S30", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope31(TEMPLATE_SLOPE31,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S31", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope32(TEMPLATE_SLOPE32,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S32", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope33(TEMPLATE_SLOPE33,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S33", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope34(TEMPLATE_SLOPE34,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S34", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope35(TEMPLATE_SLOPE35,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S35", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope36(TEMPLATE_SLOPE36,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S36", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope37(TEMPLATE_SLOPE37,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S37", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Slope38(TEMPLATE_SLOPE38,
                                       kTheaterFlagWinter | kTheaterFlagDesert |
                                           kTheaterFlagTemperate,
                                       "S38", TXT_SLOPE, LAND_ROCK, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Brush1(TEMPLATE_BRUSH1, kTheaterFlagDesert, "BR1",
                                      TXT_BRUSH, LAND_ROCK, 1, 1, LAND_ROCK,
                                      nullptr);
static const TemplateTypeClass Brush2(TEMPLATE_BRUSH2, kTheaterFlagDesert, "BR2",
                                      TXT_BRUSH, LAND_ROCK, 1, 1, LAND_ROCK,
                                      nullptr);
static const TemplateTypeClass Brush3(TEMPLATE_BRUSH3, kTheaterFlagDesert, "BR3",
                                      TXT_BRUSH, LAND_ROCK, 1, 1, LAND_ROCK,
                                      nullptr);
static const TemplateTypeClass Brush4(TEMPLATE_BRUSH4, kTheaterFlagDesert, "BR4",
                                      TXT_BRUSH, LAND_ROCK, 1, 1, LAND_ROCK,
                                      nullptr);
static const TemplateTypeClass Brush5(TEMPLATE_BRUSH5, kTheaterFlagDesert, "BR5",
                                      TXT_BRUSH, LAND_ROCK, 1, 1, LAND_ROCK,
                                      nullptr);
static const TemplateTypeClass Brush6(TEMPLATE_BRUSH6, kTheaterFlagDesert, "BR6",
                                      TXT_BRUSH, LAND_ROCK, 2, 2, LAND_ROCK,
                                      nullptr);
static const TemplateTypeClass Brush7(TEMPLATE_BRUSH7, kTheaterFlagDesert, "BR7",
                                      TXT_BRUSH, LAND_ROCK, 2, 2, LAND_ROCK,
                                      nullptr);
static const TemplateTypeClass Brush8(TEMPLATE_BRUSH8, kTheaterFlagDesert, "BR8",
                                      TXT_BRUSH, LAND_ROCK, 3, 2, LAND_ROCK,
                                      nullptr);
static const TemplateTypeClass Brush9(TEMPLATE_BRUSH9, kTheaterFlagDesert, "BR9",
                                      TXT_BRUSH, LAND_ROCK, 3, 2, LAND_ROCK,
                                      nullptr);
static const TemplateTypeClass Brush10(TEMPLATE_BRUSH10, kTheaterFlagDesert,
                                       "BR10", TXT_BRUSH, LAND_ROCK, 2, 1,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Patch1(TEMPLATE_PATCH1,
                                      kTheaterFlagTemperate | kTheaterFlagDesert,
                                      "P01", TXT_PATCH, LAND_CLEAR, 1, 1,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Patch2(TEMPLATE_PATCH2,
                                      kTheaterFlagTemperate | kTheaterFlagDesert,
                                      "P02", TXT_PATCH, LAND_ROCK, 1, 1,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Patch3(TEMPLATE_PATCH3,
                                      kTheaterFlagTemperate | kTheaterFlagDesert,
                                      "P03", TXT_PATCH, LAND_CLEAR, 1, 1,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Patch4(TEMPLATE_PATCH4,
                                      kTheaterFlagTemperate | kTheaterFlagDesert,
                                      "P04", TXT_PATCH, LAND_ROCK, 1, 1,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Patch5(TEMPLATE_PATCH5, kTheaterFlagDesert, "P05",
                                      TXT_PATCH, LAND_CLEAR, 2, 2, LAND_CLEAR,
                                      nullptr);
static const TemplateTypeClass Patch6(TEMPLATE_PATCH6, kTheaterFlagDesert, "P06",
                                      TXT_PATCH, LAND_CLEAR, 6, 4, LAND_CLEAR,
                                      nullptr);
static const TemplateTypeClass Patch7(TEMPLATE_PATCH7,
                                      kTheaterFlagWinter | kTheaterFlagTemperate |
                                          kTheaterFlagDesert,
                                      "P07", TXT_PATCH, LAND_CLEAR, 4, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Patch8(TEMPLATE_PATCH8,
                                      kTheaterFlagWinter | kTheaterFlagTemperate |
                                          kTheaterFlagDesert,
                                      "P08", TXT_PATCH, LAND_CLEAR, 3, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Patch13(TEMPLATE_PATCH13,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "P13", TXT_PATCH, LAND_CLEAR, 3, 2,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Patch14(TEMPLATE_PATCH14,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "P14", TXT_PATCH, LAND_CLEAR, 2, 1,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Patch15(TEMPLATE_PATCH15,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "P15", TXT_PATCH, LAND_CLEAR, 1, 1,
                                       LAND_CLEAR, nullptr);
static const TemplateTypeClass Patch16(TEMPLATE_PATCH16, kTheaterFlagWinter, "P16",
                                       TXT_PATCH, LAND_CLEAR, 2, 2, LAND_CLEAR,
                                       nullptr);
static const TemplateTypeClass Patch17(TEMPLATE_PATCH17, kTheaterFlagWinter, "P17",
                                       TXT_PATCH, LAND_CLEAR, 4, 2, LAND_CLEAR,
                                       nullptr);
static const TemplateTypeClass Patch18(TEMPLATE_PATCH18, kTheaterFlagWinter, "P18",
                                       TXT_PATCH, LAND_CLEAR, 4, 3, LAND_CLEAR,
                                       nullptr);
static const TemplateTypeClass Patch19(TEMPLATE_PATCH19, kTheaterFlagWinter, "P19",
                                       TXT_PATCH, LAND_CLEAR, 4, 3, LAND_CLEAR,
                                       nullptr);
static const TemplateTypeClass Patch20(TEMPLATE_PATCH20, kTheaterFlagWinter, "P20",
                                       TXT_PATCH, LAND_CLEAR, 4, 3, LAND_CLEAR,
                                       nullptr);
static const TemplateTypeClass River1(TEMPLATE_RIVER1,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "RV01", TXT_RIVER, LAND_WATER, 5, 4,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass River2(TEMPLATE_RIVER2,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "RV02", TXT_RIVER, LAND_WATER, 5, 3,
                                      LAND_ROCK, nullptr);
static const TemplateTypeClass River3(TEMPLATE_RIVER3,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "RV03", TXT_RIVER, LAND_WATER, 4, 4,
                                      LAND_CLEAR, slope00000001);
static const TemplateTypeClass River4(TEMPLATE_RIVER4,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "RV04", TXT_RIVER, LAND_WATER, 4, 4,
                                      LAND_ROCK, nullptr);
static const TemplateTypeClass River5(TEMPLATE_RIVER5,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "RV05", TXT_RIVER, LAND_WATER, 3, 3,
                                      LAND_ROCK, nullptr);
static const TemplateTypeClass River6(TEMPLATE_RIVER6,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "RV06", TXT_RIVER, LAND_WATER, 3, 2,
                                      LAND_ROCK, nullptr);
static const TemplateTypeClass River7(TEMPLATE_RIVER7,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "RV07", TXT_RIVER, LAND_WATER, 3, 2,
                                      LAND_ROCK, nullptr);
static const TemplateTypeClass River8(TEMPLATE_RIVER8,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "RV08", TXT_RIVER, LAND_WATER, 2, 2,
                                      LAND_ROCK, nullptr);
static const TemplateTypeClass River9(TEMPLATE_RIVER9,
                                      kTheaterFlagWinter | kTheaterFlagTemperate,
                                      "RV09", TXT_RIVER, LAND_WATER, 2, 2,
                                      LAND_ROCK, nullptr);
static const TemplateTypeClass River10(TEMPLATE_RIVER10,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "RV10", TXT_RIVER, LAND_WATER, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River11(TEMPLATE_RIVER11,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "RV11", TXT_RIVER, LAND_WATER, 2, 2,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River12(TEMPLATE_RIVER12,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "RV12", TXT_RIVER, LAND_WATER, 3, 4,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River13(TEMPLATE_RIVER13,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "RV13", TXT_RIVER, LAND_WATER, 4, 4,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River14(TEMPLATE_RIVER14, kTheaterFlagDesert,
                                       "RV14", TXT_RIVER, LAND_WATER, 4, 3,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River15(TEMPLATE_RIVER15, kTheaterFlagDesert,
                                       "RV15", TXT_RIVER, LAND_WATER, 4, 3,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River16(TEMPLATE_RIVER16, kTheaterFlagDesert,
                                       "RV16", TXT_RIVER, LAND_WATER, 6, 4,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River17(TEMPLATE_RIVER17, kTheaterFlagDesert,
                                       "RV17", TXT_RIVER, LAND_WATER, 6, 5,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River18(TEMPLATE_RIVER18, kTheaterFlagDesert,
                                       "RV18", TXT_RIVER, LAND_WATER, 4, 4,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River19(TEMPLATE_RIVER19, kTheaterFlagDesert,
                                       "RV19", TXT_RIVER, LAND_WATER, 4, 4,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River20(TEMPLATE_RIVER20, kTheaterFlagDesert,
                                       "RV20", TXT_RIVER, LAND_WATER, 6, 8,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River21(TEMPLATE_RIVER21, kTheaterFlagDesert,
                                       "RV21", TXT_RIVER, LAND_WATER, 5, 8,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River22(TEMPLATE_RIVER22, kTheaterFlagDesert,
                                       "RV22", TXT_RIVER, LAND_WATER, 3, 3,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River23(TEMPLATE_RIVER23, kTheaterFlagDesert,
                                       "RV23", TXT_RIVER, LAND_WATER, 3, 3,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River24(TEMPLATE_RIVER24, kTheaterFlagDesert,
                                       "RV24", TXT_RIVER, LAND_WATER, 3, 3,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass River25(TEMPLATE_RIVER25, kTheaterFlagDesert,
                                       "RV25", TXT_RIVER, LAND_WATER, 3, 3,
                                       LAND_ROCK, nullptr);
static const TemplateTypeClass Ford1(TEMPLATE_FORD1,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate,
                                     "FORD1", TXT_RIVER, LAND_WATER, 3, 3,
                                     LAND_CLEAR, slope001111001);
static const TemplateTypeClass Ford2(TEMPLATE_FORD2,
                                     kTheaterFlagWinter | kTheaterFlagDesert |
                                         kTheaterFlagTemperate,
                                     "FORD2", TXT_RIVER, LAND_WATER, 3, 3,
                                     LAND_CLEAR, slope111010011);
static const TemplateTypeClass Falls1(TEMPLATE_FALLS1,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "FALLS1", TXT_RIVER, LAND_WATER, 3, 3,
                                      LAND_CLEAR, slope1);
static const TemplateTypeClass Falls2(TEMPLATE_FALLS2,
                                      kTheaterFlagWinter | kTheaterFlagDesert |
                                          kTheaterFlagTemperate,
                                      "FALLS2", TXT_RIVER, LAND_WATER, 3, 2,
                                      LAND_CLEAR, nullptr);
static const TemplateTypeClass Bridge1(TEMPLATE_BRIDGE1,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "BRIDGE1", TXT_RIVER, LAND_WATER, 4, 4,
                                       LAND_CLEAR, slope00110010010011);
static const TemplateTypeClass Bridge1d(TEMPLATE_BRIDGE1D,
                                        kTheaterFlagWinter | kTheaterFlagTemperate,
                                        "BRIDGE1D", TXT_RIVER, LAND_WATER, 4, 4,
                                        LAND_CLEAR, slope00110000000011);
static const TemplateTypeClass Bridge2(TEMPLATE_BRIDGE2,
                                       kTheaterFlagWinter | kTheaterFlagTemperate,
                                       "BRIDGE2", TXT_RIVER, LAND_WATER, 5, 5,
                                       LAND_CLEAR,
                                       slope1100001000001000001100011);
static const TemplateTypeClass Bridge2d(TEMPLATE_BRIDGE2D,
                                        kTheaterFlagWinter | kTheaterFlagTemperate,
                                        "BRIDGE2D", TXT_RIVER, LAND_WATER, 5, 5,
                                        LAND_CLEAR,
                                        slope1100000000000000001100011);
static const TemplateTypeClass Bridge3(TEMPLATE_BRIDGE3, kTheaterFlagDesert,
                                       "BRIDGE3", TXT_RIVER, LAND_WATER, 6, 5,
                                       LAND_CLEAR,
                                       slope00011010010100100001000011);
static const TemplateTypeClass Bridge3d(TEMPLATE_BRIDGE3D, kTheaterFlagDesert,
                                        "BRIDGE3D", TXT_RIVER, LAND_WATER, 6, 5,
                                        LAND_CLEAR,
                                        slope00011010000100000001000011);
static const TemplateTypeClass Bridge4(TEMPLATE_BRIDGE4, kTheaterFlagDesert,
                                       "BRIDGE4", TXT_RIVER, LAND_WATER, 6, 4,
                                       LAND_CLEAR,
                                       slope01000000100000010000001);
static const TemplateTypeClass Bridge4d(TEMPLATE_BRIDGE4D, kTheaterFlagDesert,
                                        "BRIDGE4D", TXT_RIVER, LAND_WATER, 6, 4,
                                        LAND_CLEAR,
                                        slope01000000000000000000001);

const base::EnumArray<TemplateType, const TemplateTypeClass*, kTemplateCount>
    TemplateTypeClass::Pointers = {
        &Clear,     // TEMPLATE_CLEAR1
        &Water,     // TEMPLATE_WATER
        &Water2,    // TEMPLATE_WATER2
        &Shore1,    // TEMPLATE_SHORE1
        &Shore2,    // TEMPLATE_SHORE2
        &Shore3,    // TEMPLATE_SHORE3
        &Shore4,    // TEMPLATE_SHORE4
        &Shore5,    // TEMPLATE_SHORE5
        &Shore11,   //	TEMPLATE_SHORE11
        &Shore12,   // TEMPLATE_SHORE12
        &Shore13,   // TEMPLATE_SHORE13
        &Shore14,   // TEMPLATE_SHORE14
        &Shore15,   // TEMPLATE_SHORE15
        &Slope1,    //	TEMPLATE_SLOPE1
        &Slope2,    //	TEMPLATE_SLOPE2
        &Slope3,    //	TEMPLATE_SLOPE3
        &Slope4,    //	TEMPLATE_SLOPE4
        &Slope5,    //	TEMPLATE_SLOPE5
        &Slope6,    //	TEMPLATE_SLOPE6
        &Slope7,    //	TEMPLATE_SLOPE7
        &Slope8,    //	TEMPLATE_SLOPE8
        &Slope9,    //	TEMPLATE_SLOPE9
        &Slope10,   //	TEMPLATE_SLOPE10
        &Slope11,   //	TEMPLATE_SLOPE11
        &Slope12,   //	TEMPLATE_SLOPE12
        &Slope13,   //	TEMPLATE_SLOPE13
        &Slope14,   //	TEMPLATE_SLOPE14
        &Slope15,   //	TEMPLATE_SLOPE15
        &Slope16,   //	TEMPLATE_SLOPE16
        &Slope17,   //	TEMPLATE_SLOPE17
        &Slope18,   //	TEMPLATE_SLOPE18
        &Slope19,   //	TEMPLATE_SLOPE19
        &Slope20,   //	TEMPLATE_SLOPE20
        &Slope21,   //	TEMPLATE_SLOPE21
        &Slope22,   //	TEMPLATE_SLOPE22
        &Slope23,   //	TEMPLATE_SLOPE23
        &Slope24,   //	TEMPLATE_SLOPE24
        &Slope25,   //	TEMPLATE_SLOPE25
        &Slope26,   //	TEMPLATE_SLOPE26
        &Slope27,   //	TEMPLATE_SLOPE27
        &Slope28,   //	TEMPLATE_SLOPE28
        &Slope29,   //	TEMPLATE_SLOPE29
        &Slope30,   //	TEMPLATE_SLOPE30
        &Slope31,   //	TEMPLATE_SLOPE31
        &Slope32,   //	TEMPLATE_SLOPE32
        &Slope33,   //	TEMPLATE_SLOPE33
        &Slope34,   //	TEMPLATE_SLOPE34
        &Slope35,   //	TEMPLATE_SLOPE35
        &Slope36,   //	TEMPLATE_SLOPE36
        &Slope37,   //	TEMPLATE_SLOPE37
        &Slope38,   //	TEMPLATE_SLOPE38
        &Shore32,   // TEMPLATE_SHORE32
        &Shore33,   // TEMPLATE_SHORE33
        &Shore20,   // TEMPLATE_SHORE20
        &Shore21,   // TEMPLATE_SHORE21
        &Shore22,   //	TEMPLATE_SHORE22
        &Shore23,   // TEMPLATE_SHORE23
        &Brush1,    //	TEMPLATE_BRUSH1
        &Brush2,    //	TEMPLATE_BRUSH2
        &Brush3,    //	TEMPLATE_BRUSH3
        &Brush4,    //	TEMPLATE_BRUSH4
        &Brush5,    //	TEMPLATE_BRUSH5
        &Brush6,    //	TEMPLATE_BRUSH6
        &Brush7,    //	TEMPLATE_BRUSH7
        &Brush8,    //	TEMPLATE_BRUSH8
        &Brush9,    //	TEMPLATE_BRUSH9
        &Brush10,   //	TEMPLATE_BRUSH10
        &Patch1,    //	TEMPLATE_PATCH1
        &Patch2,    //	TEMPLATE_PATCH2
        &Patch3,    //	TEMPLATE_PATCH3
        &Patch4,    //	TEMPLATE_PATCH4
        &Patch5,    //	TEMPLATE_PATCH5
        &Patch6,    //	TEMPLATE_PATCH6
        &Patch7,    //	TEMPLATE_PATCH7
        &Patch8,    //	TEMPLATE_PATCH8
        &Shore16,   //	TEMPLATE_SHORE16
        &Shore17,   //	TEMPLATE_SHORE17
        &Shore18,   //	TEMPLATE_SHORE18
        &Shore19,   // TEMPLATE_SHORE19
        &Patch13,   //	TEMPLATE_PATCH13
        &Patch14,   //	TEMPLATE_PATCH14
        &Patch15,   //	TEMPLATE_PATCH15
        &Boulder1,  //	TEMPLATE_BOULDER1
        &Boulder2,  //	TEMPLATE_BOULDER2
        &Boulder3,  //	TEMPLATE_BOULDER3
        &Boulder4,  // TEMPLATE_BOULDER4
        &Boulder5,  //	TEMPLATE_BOULDER5
        &Boulder6,  //	TEMPLATE_BOULDER6
        &Shore6,    // TEMPLATE_SHORE6
        &Shore7,    // TEMPLATE_SHORE7
        &Shore8,    // TEMPLATE_SHORE8
        &Shore9,    // TEMPLATE_SHORE9
        &Shore10,   // TEMPLATE_SHORE10

        &Road1,   //	TEMPLATE_ROAD1
        &Road2,   //	TEMPLATE_ROAD2
        &Road3,   //	TEMPLATE_ROAD3
        &Road4,   //	TEMPLATE_ROAD4
        &Road5,   //	TEMPLATE_ROAD5
        &Road6,   //	TEMPLATE_ROAD6
        &Road7,   //	TEMPLATE_ROAD7
        &Road8,   //	TEMPLATE_ROAD8
        &Road9,   //	TEMPLATE_ROAD9
        &Road10,  //	TEMPLATE_ROAD10
        &Road11,  //	TEMPLATE_ROAD11
        &Road12,  //	TEMPLATE_ROAD12
        &Road13,  //	TEMPLATE_ROAD13
        &Road14,  //	TEMPLATE_ROAD14
        &Road15,  //	TEMPLATE_ROAD15
        &Road16,  //	TEMPLATE_ROAD16
        &Road17,  //	TEMPLATE_ROAD17
        &Road18,  //	TEMPLATE_ROAD18
        &Road19,  //	TEMPLATE_ROAD19
        &Road20,  //	TEMPLATE_ROAD20
        &Road21,  //	TEMPLATE_ROAD21
        &Road22,  //	TEMPLATE_ROAD22
        &Road23,  //	TEMPLATE_ROAD23
        &Road24,  //	TEMPLATE_ROAD24
        &Road25,  //	TEMPLATE_ROAD25
        &Road26,  //	TEMPLATE_ROAD26
        &Road27,  //	TEMPLATE_ROAD27
        &Road28,  //	TEMPLATE_ROAD28
        &Road29,  //	TEMPLATE_ROAD29
        &Road30,  //	TEMPLATE_ROAD30
        &Road31,  //	TEMPLATE_ROAD31
        &Road32,  //	TEMPLATE_ROAD32
        &Road33,  //	TEMPLATE_ROAD33
        &Road34,  //	TEMPLATE_ROAD34
        &Road35,  //	TEMPLATE_ROAD35
        &Road36,  //	TEMPLATE_ROAD36
        &Road37,  //	TEMPLATE_ROAD37
        &Road38,  //	TEMPLATE_ROAD38
        &Road39,  //	TEMPLATE_ROAD39
        &Road40,  //	TEMPLATE_ROAD40
        &Road41,  //	TEMPLATE_ROAD41
        &Road42,  //	TEMPLATE_ROAD42
        &Road43,  //	TEMPLATE_ROAD43

        &River1,    //	TEMPLATE_RIVER1
        &River2,    //	TEMPLATE_RIVER2
        &River3,    //	TEMPLATE_RIVER3
        &River4,    //	TEMPLATE_RIVER4
        &River5,    //	TEMPLATE_RIVER5
        &River6,    //	TEMPLATE_RIVER6
        &River7,    //	TEMPLATE_RIVER7
        &River8,    //	TEMPLATE_RIVER8
        &River9,    //	TEMPLATE_RIVER9
        &River10,   //	TEMPLATE_RIVER10
        &River11,   //	TEMPLATE_RIVER11
        &River12,   //	TEMPLATE_RIVER12
        &River13,   //	TEMPLATE_RIVER13
        &River14,   //	TEMPLATE_RIVER14
        &River15,   //	TEMPLATE_RIVER15
        &River16,   //	TEMPLATE_RIVER16
        &River17,   //	TEMPLATE_RIVER17
        &River18,   //	TEMPLATE_RIVER18
        &River19,   //	TEMPLATE_RIVER19
        &River20,   //	TEMPLATE_RIVER20
        &River21,   //	TEMPLATE_RIVER21
        &River22,   //	TEMPLATE_RIVER22
        &River23,   //	TEMPLATE_RIVER23
        &River24,   //	TEMPLATE_RIVER24
        &River25,   //	TEMPLATE_RIVER25
        &Ford1,     //	TEMPLATE_FORD1
        &Ford2,     //	TEMPLATE_FORD2
        &Falls1,    //	TEMPLATE_FALLS1
        &Falls2,    //	TEMPLATE_FALLS2
        &Bridge1,   //	TEMPLATE_BRIDGE1
        &Bridge1d,  //	TEMPLATE_BRIDGE1D
        &Bridge2,   //	TEMPLATE_BRIDGE2
        &Bridge2d,  //	TEMPLATE_BRIDGE2D
        &Bridge3,   //	TEMPLATE_BRIDGE3
        &Bridge3d,  //	TEMPLATE_BRIDGE3D
        &Bridge4,   //	TEMPLATE_BRIDGE4
        &Bridge4d,  //	TEMPLATE_BRIDGE4D

        &Shore24,  //	TEMPLATE_SHORE24
        &Shore25,  //	TEMPLATE_SHORE25
        &Shore26,  //	TEMPLATE_SHORE26
        &Shore27,  //	TEMPLATE_SHORE27
        &Shore28,  //	TEMPLATE_SHORE28
        &Shore29,  //	TEMPLATE_SHORE29
        &Shore30,  //	TEMPLATE_SHORE30
        &Shore31,  //	TEMPLATE_SHORE31

        &Patch16,  //	TEMPLATE_PATCH16
        &Patch17,  //	TEMPLATE_PATCH17
        &Patch18,  //	TEMPLATE_PATCH18
        &Patch19,  //	TEMPLATE_PATCH19
        &Patch20,  //	TEMPLATE_PATCH20

        &Shore34,  //	TEMPLATE_SHORE34
        &Shore35,  //	TEMPLATE_SHORE35
        &Shore36,  //	TEMPLATE_SHORE36
        &Shore37,  //	TEMPLATE_SHORE37
        &Shore38,  //	TEMPLATE_SHORE38
        &Shore39,  //	TEMPLATE_SHORE39
        &Shore40,  //	TEMPLATE_SHORE40
        &Shore41,  //	TEMPLATE_SHORE41
        &Shore42,  //	TEMPLATE_SHORE42
        &Shore43,  //	TEMPLATE_SHORE43
        &Shore44,  //	TEMPLATE_SHORE44
        &Shore45,  //	TEMPLATE_SHORE45

        &Shore46,  //	TEMPLATE_SHORE46
        &Shore47,  //	TEMPLATE_SHORE47
        &Shore48,  //	TEMPLATE_SHORE48
        &Shore49,  //	TEMPLATE_SHORE49
        &Shore50,  //	TEMPLATE_SHORE50
        &Shore51,  //	TEMPLATE_SHORE51
        &Shore52,  //	TEMPLATE_SHORE52
        &Shore53,  //	TEMPLATE_SHORE53
        &Shore54,  //	TEMPLATE_SHORE54
        &Shore55,  //	TEMPLATE_SHORE55
        &Shore56,  //	TEMPLATE_SHORE56
        &Shore57,  //	TEMPLATE_SHORE57
        &Shore58,  //	TEMPLATE_SHORE58
        &Shore59,  //	TEMPLATE_SHORE59
        &Shore60,  //	TEMPLATE_SHORE60
        &Shore61,  //	TEMPLATE_SHORE61

        &Shore62,  //	TEMPLATE_SHORE62
        &Shore63,  //	TEMPLATE_SHORE63
};

/***********************************************************************************************
 * TemplateTypeClass::TemplateTypeClass -- Constructor for template type
 *objects.              *
 *                                                                                             *
 *    This is the constructor for the template types. *
 *                                                                                             *
 * INPUT:   see below... *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/29/1994 JLB : Created. *
 *=============================================================================================*/
TemplateTypeClass::TemplateTypeClass(TemplateType iconset, int theater,
                                     const char* ininame, int fullname,
                                     LandType land, int width, int height,
                                     LandType altland,
                                     const char* alticons) noexcept
    : ObjectTypeClass(false, false, false, true, false, false, true, true,
                      fullname, ininame, ARMOR_NONE, 0),
      Type(iconset),
      Theater(static_cast<unsigned char>(theater)),
      Land(land),
      Width(static_cast<unsigned char>(width)),
      Height(static_cast<unsigned char>(height)),
      AltLand(altland),
      AltIcons(alticons) {}

/***********************************************************************************************
 * TemplateTypeClass::From_Name -- Determine template from ASCII name. *
 *                                                                                             *
 *    This routine is used to determine the template number given only * an
 *ASCII representation. The scenario loader uses this routine * to construct the
 *map from the INI control file.                                          *
 *                                                                                             *
 * INPUT:   name  -- Pointer to the ASCII name of the template. *
 *                                                                                             *
 * OUTPUT:  Returns with the template number. If the name had no match, * then
 *returns with TEMPLATE_NONE. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/23/1994 JLB : Created. *
 *=============================================================================================*/
TemplateType TemplateTypeClass::From_Name(const char* name) {
  if (name) {
    for (TemplateType index = TEMPLATE_CLEAR1; index < TEMPLATE_COUNT;
         index++) {
      if (stricmp(As_Reference(index).IniName, name) == 0) {
        return index;
      }
    }
  }
  return TEMPLATE_NONE;
}

/***********************************************************************************************
 * TemplateTypeClass::Occupy_List -- Determines occupation list. *
 *                                                                                             *
 *    This routine is used to examine the template map and build an * occupation
 *list. This list is used to render a template cursor as * well as placement of
 *icon numbers.                                                       *
 *                                                                                             *
 * INPUT:   placement   -- Is this for placement legality checking only? The
 *normal condition  * is for marking occupation flags. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the template occupation list. *
 *                                                                                             *
 * WARNINGS:   The return pointer is valid only until the next time that * this
 *routine is called.                                                         *
 *                                                                                             *
 * HISTORY: * 05/23/1994 JLB : Created. *
 *=============================================================================================*/
const int16_t* TemplateTypeClass::Occupy_List(bool /*placement*/) const {
  static int16_t _occupy[(13 * 8) + 5];
  unsigned char map[13 * 8];

  Mem_Copy(Get_Icon_Set_Map(Get_Image_Data()), map,
           static_cast<size_t>(Width) * Height);

  int16_t* ptr = &_occupy[0];
  for (int index = 0; index < Width * Height; index++) {
    if (map[index] != 0xFF) {
      *ptr++ =
          static_cast<int16_t>((index % Width) + (index / Width * MAP_CELL_W));
    }
  }
  *ptr = REFRESH_EOL;

  return &_occupy[0];
}

/***********************************************************************************************
 * TemplateTypeClass::Init -- Loads graphic data for templates. *
 *                                                                                             *
 *    This routine loads the template graphic data for all the template * type
 *supported for the specified theater. This routine is called * whenever the
 *theater for the scenario is first determined.                               *
 *                                                                                             *
 * INPUT:   theater  -- The theater that the template data is to be * loaded
 *for.                                                            *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   This routine goes to disk! *
 *                                                                                             *
 * HISTORY: * 05/23/1994 JLB : Created. * 06/02/1994 JLB : Only handles iconset
 *loading now (as it should).                         *
 *=============================================================================================*/
void TemplateTypeClass::Init(TheaterType theater) {

  for (TemplateType index = TEMPLATE_CLEAR1; index < TEMPLATE_COUNT; index++) {
    const TemplateTypeClass& tplate = As_Reference(index);

    tplate.Set_Image_Data(nullptr);
    if ((tplate.Theater & base::Bit<uint8_t>(theater)) != 0) {
      // Fully constructed iconset name.
      const auto fullname = std::filesystem::path(tplate.IniName)
                                .replace_extension(Theaters[theater].Suffix)
                                .string();
      const void* ptr =
          MixArchive::Retrieve(fullname);  // Working loaded iconset pointer.
      tplate.Set_Image_Data(ptr);
      Register_Icon_Set(ptr,
                        true);  // Register icon set for video memory caching
    }
  }
}

/***********************************************************************************************
 * TemplateTypeClass::Display -- Displays a generic representation of template.
 **
 *                                                                                             *
 *    This routine is used to display a generic view of the template * object.
 *This is necessary for selection in the scenario editor. *
 *                                                                                             *
 * INPUT:   x,y   -- The coordinates to center the display about. *
 *                                                                                             *
 *          window-- The window to base the coordinates upon. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/23/1994 JLB : Created. *
 *=============================================================================================*/
void TemplateTypeClass::Display(int x, int y, WindowNumberType window,
                                HousesType /*unused*/) const {
  unsigned char map[13 * 8];

  const int w = Bound(Width, 1, 13);
  const int h = Bound(Height, 1, 8);
  const bool scale = (w > 3 || h > 3);  // Should the template be half sized?
  if (scale) {
    x -= (w / 2) * (ICON_PIXEL_W / 2);
    y -= (h / 2) * (ICON_PIXEL_H / 2);
  } else {
    x -= (w / 2) * ICON_PIXEL_W;
    y -= (h / 2) * ICON_PIXEL_H;
  }
  x += WindowList[static_cast<int>(window)][kWindowX] * 8;
  y += WindowList[static_cast<int>(window)][kWindowY];

  Mem_Copy(Get_Icon_Set_Map(Get_Image_Data()), map,
           static_cast<size_t>(Width) * Height);

  for (int index = 0; index < w * h; index++) {
    if (map[index] != 0xFF) {
      HidPage.Draw_Stamp(Get_Image_Data(), index, 0, 0, nullptr,
                         static_cast<int>(WINDOW_MAIN));
      if (scale) {
        HidPage.Scale(
            (*LogicPage), 0, 0, x + ((index % w) * (ICON_PIXEL_W / 2)),
            y + ((index / w) * (ICON_PIXEL_H / 2)), ICON_PIXEL_W, ICON_PIXEL_H,
            ICON_PIXEL_W / 2, ICON_PIXEL_H / 2, nullptr);

      } else {
        HidPage.Blit((*LogicPage), 0, 0, x + ((index % w) * (ICON_PIXEL_W)),
                     y + ((index / w) * (ICON_PIXEL_H)), ICON_PIXEL_W,
                     ICON_PIXEL_H);
      }
    }
  }
}

/***********************************************************************************************
 * TemplateTypeClass::Prep_For_Add -- Prepares to add template to scenario. *
 *                                                                                             *
 *    This routine prepares a list of template objects so that the * scenario
 *editor can use this list to display a dialog box. The * selection of a
 *template object will allow its placement upon the                         *
 *    map. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/23/1994 JLB : Created. * 05/28/1994 JLB : Only handles real
 *templates now.                                         * 06/04/1994 JLB : Uses
 *map editing interface functions.                                    *
 *=============================================================================================*/
void TemplateTypeClass::Prep_For_Add() {
  for (TemplateType index = TEMPLATE_CLEAR1; index < TEMPLATE_COUNT; index++) {
    if (As_Reference(index).Get_Image_Data()) {
      Map.Add_To_List(&As_Reference(index));
    }
  }
}

/***********************************************************************************************
 * TemplateTypeClass::Create_And_Place -- Creates and places a template object
 *on the map.     *
 *                                                                                             *
 *    This support routine is used by the scenario editor to add a template
 *object to the map  * and to the game. *
 *                                                                                             *
 * INPUT:   cell  -- The cell to place the template object. *
 *                                                                                             *
 * OUTPUT:  bool; Was the template object placed successfully? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/28/1994 JLB : Created. *
 *=============================================================================================*/
bool TemplateTypeClass::Create_And_Place(CELL cell,
                                         HousesType /*unused*/) const {
  return new TemplateClass(Type, cell) != nullptr;
}

/***********************************************************************************************
 * TemplateTypeClass::Create_One_Of -- Creates an object of this template type.
 **
 *                                                                                             *
 *    This routine will create an object of this type. For certain template
 *objects, such      * as walls, it is actually created as a building. The
 *"building" wall is converted into    * a template at the moment of placing
 *down on the map.                                     *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the appropriate object for this template
 *type.           *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/18/1994 JLB : Created. *
 *=============================================================================================*/
ObjectClass* TemplateTypeClass::Create_One_Of(HouseClass* /*unused*/) const {
  return new TemplateClass(Type, -1);
}

/***********************************************************************************************
 * TemplateTypeClass::One_Time -- Performs one-time initialization *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/12/1994 JLB : Created. *
 *=============================================================================================*/
void TemplateTypeClass::One_Time() {}
