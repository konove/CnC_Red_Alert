/*
**	Command & Conquer Red Alert(tm)
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

/* $Header: /CounterStrike/CDATA.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
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
 *                  Last Update : July 6, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * TemplateTypeClass::As_Reference -- Fetches a reference to the
 *template specified.         * TemplateTypeClass::Create_And_Place -- Creates
 *and places a template object on the map.   * TemplateTypeClass::Create_One_Of
 *-- Creates an object of this template type.              *
 *   TemplateTypeClass::Display -- Displays a generic representation of
 *template.              * TemplateTypeClass::From_Name -- Determine template
 *from ASCII name.                       * TemplateTypeClass::Init -- Loads
 *graphic data for templates.                              *
 *   TemplateTypeClass::Land_Type -- Determines land type from template and icon
 *number.       * TemplateTypeClass::Occupy_List -- Determines occupation list.
 ** TemplateTypeClass::One_Time -- Performs one-time initialization *
 *   TemplateTypeClass::Prep_For_Add -- Prepares to add template to scenario. *
 *   TemplateTypeClass::TemplateTypeClass -- Constructor for template type
 *objects.            * TemplateTypeClass::operator delete -- Deletes a template
 *type object.                     * TemplateTypeClass::operator new --
 *Allocates a template type from special heap.           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>

#include "magic_enum/magic_enum.hpp"
#include "port/ex_string.h"
#include "ra/compat.h"
#include "ra/conquer.h"
#include "ra/const.h"
#include "ra/defines.h"
#include "ra/display.h"
#include "ra/externs.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/mapedit.h"
#include "ra/object.h"
#include "ra/template.h"
#include "ra/type.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/iconcach.h"
#include "sdllib/ww_win.h"

static const TemplateTypeClass Empty(TEMPLATE_CLEAR1,
                                     kTheaterFlagTemperate | kTheaterFlagSnow |
                                         kTheaterFlagInterior,
                                     "CLEAR1", TXT_CLEAR);
static const TemplateTypeClass Clear(TEMPLATE_CLEAR1,
                                     kTheaterFlagTemperate | kTheaterFlagSnow |
                                         kTheaterFlagInterior,
                                     "CLEAR1", TXT_CLEAR);
static const TemplateTypeClass Road01(TEMPLATE_ROAD01,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D01", TXT_ROAD);
static const TemplateTypeClass Road02(TEMPLATE_ROAD02,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D02", TXT_ROAD);
static const TemplateTypeClass Road03(TEMPLATE_ROAD03,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D03", TXT_ROAD);
static const TemplateTypeClass Road04(TEMPLATE_ROAD04,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D04", TXT_ROAD);
static const TemplateTypeClass Road05(TEMPLATE_ROAD05,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D05", TXT_ROAD);
static const TemplateTypeClass Road06(TEMPLATE_ROAD06,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D06", TXT_ROAD);
static const TemplateTypeClass Road07(TEMPLATE_ROAD07,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D07", TXT_ROAD);
static const TemplateTypeClass Road08(TEMPLATE_ROAD08,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D08", TXT_ROAD);
static const TemplateTypeClass Road09(TEMPLATE_ROAD09,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D09", TXT_ROAD);
static const TemplateTypeClass Road10(TEMPLATE_ROAD10,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D10", TXT_ROAD);
static const TemplateTypeClass Road11(TEMPLATE_ROAD11,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D11", TXT_ROAD);
static const TemplateTypeClass Road12(TEMPLATE_ROAD12,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D12", TXT_ROAD);
static const TemplateTypeClass Road13(TEMPLATE_ROAD13,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D13", TXT_ROAD);
static const TemplateTypeClass Road14(TEMPLATE_ROAD14,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D14", TXT_ROAD);
static const TemplateTypeClass Road15(TEMPLATE_ROAD15,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D15", TXT_ROAD);
static const TemplateTypeClass Road16(TEMPLATE_ROAD16,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D16", TXT_ROAD);
static const TemplateTypeClass Road17(TEMPLATE_ROAD17,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D17", TXT_ROAD);
static const TemplateTypeClass Road18(TEMPLATE_ROAD18,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D18", TXT_ROAD);
static const TemplateTypeClass Road19(TEMPLATE_ROAD19,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D19", TXT_ROAD);
static const TemplateTypeClass Road20(TEMPLATE_ROAD20,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D20", TXT_ROAD);
static const TemplateTypeClass Road21(TEMPLATE_ROAD21,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D21", TXT_ROAD);
static const TemplateTypeClass Road22(TEMPLATE_ROAD22,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D22", TXT_ROAD);
static const TemplateTypeClass Road23(TEMPLATE_ROAD23,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D23", TXT_ROAD);
static const TemplateTypeClass Road24(TEMPLATE_ROAD24,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D24", TXT_ROAD);
static const TemplateTypeClass Road25(TEMPLATE_ROAD25,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D25", TXT_ROAD);
static const TemplateTypeClass Road26(TEMPLATE_ROAD26,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D26", TXT_ROAD);
static const TemplateTypeClass Road27(TEMPLATE_ROAD27,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D27", TXT_ROAD);
static const TemplateTypeClass Road28(TEMPLATE_ROAD28,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D28", TXT_ROAD);
static const TemplateTypeClass Road29(TEMPLATE_ROAD29,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D29", TXT_ROAD);
static const TemplateTypeClass Road30(TEMPLATE_ROAD30,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D30", TXT_ROAD);
static const TemplateTypeClass Road31(TEMPLATE_ROAD31,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D31", TXT_ROAD);
static const TemplateTypeClass Road32(TEMPLATE_ROAD32,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D32", TXT_ROAD);
static const TemplateTypeClass Road33(TEMPLATE_ROAD33,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D33", TXT_ROAD);
static const TemplateTypeClass Road34(TEMPLATE_ROAD34,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D34", TXT_ROAD);
static const TemplateTypeClass Road35(TEMPLATE_ROAD35,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D35", TXT_ROAD);
static const TemplateTypeClass Road36(TEMPLATE_ROAD36,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D36", TXT_ROAD);
static const TemplateTypeClass Road37(TEMPLATE_ROAD37,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D37", TXT_ROAD);
static const TemplateTypeClass Road38(TEMPLATE_ROAD38,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D38", TXT_ROAD);
static const TemplateTypeClass Road39(TEMPLATE_ROAD39,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D39", TXT_ROAD);
static const TemplateTypeClass Road40(TEMPLATE_ROAD40,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D40", TXT_ROAD);
static const TemplateTypeClass Road41(TEMPLATE_ROAD41,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D41", TXT_ROAD);
static const TemplateTypeClass Road42(TEMPLATE_ROAD42,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D42", TXT_ROAD);
static const TemplateTypeClass Road43(TEMPLATE_ROAD43,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D43", TXT_ROAD);
static const TemplateTypeClass Road44(TEMPLATE_ROAD44,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D44", TXT_ROAD);
static const TemplateTypeClass Road45(TEMPLATE_ROAD45,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "D45", TXT_ROAD);
static const TemplateTypeClass Water(TEMPLATE_WATER,
                                     kTheaterFlagTemperate | kTheaterFlagSnow,
                                     "W1", TXT_WATER);
static const TemplateTypeClass Water2(TEMPLATE_WATER2,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "W2", TXT_WATER);
static const TemplateTypeClass Shore01(TEMPLATE_SHORE01,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH01", TXT_SHORE);
static const TemplateTypeClass Shore02(TEMPLATE_SHORE02,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH02", TXT_SHORE);
static const TemplateTypeClass Shore03(TEMPLATE_SHORE03,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH03", TXT_SHORE);
static const TemplateTypeClass Shore04(TEMPLATE_SHORE04,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH04", TXT_SHORE);
static const TemplateTypeClass Shore05(TEMPLATE_SHORE05,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH05", TXT_SHORE);
static const TemplateTypeClass Shore06(TEMPLATE_SHORE06,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH06", TXT_SHORE);
static const TemplateTypeClass Shore07(TEMPLATE_SHORE07,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH07", TXT_SHORE);
static const TemplateTypeClass Shore08(TEMPLATE_SHORE08,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH08", TXT_SHORE);
static const TemplateTypeClass Shore09(TEMPLATE_SHORE09,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH09", TXT_SHORE);
static const TemplateTypeClass Shore10(TEMPLATE_SHORE10,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH10", TXT_SHORE);
static const TemplateTypeClass Shore11(TEMPLATE_SHORE11,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH11", TXT_SHORE);
static const TemplateTypeClass Shore12(TEMPLATE_SHORE12,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH12", TXT_SHORE);
static const TemplateTypeClass Shore13(TEMPLATE_SHORE13,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH13", TXT_SHORE);
static const TemplateTypeClass Shore14(TEMPLATE_SHORE14,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH14", TXT_SHORE);
static const TemplateTypeClass Shore15(TEMPLATE_SHORE15,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH15", TXT_SHORE);
static const TemplateTypeClass Shore16(TEMPLATE_SHORE16,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH16", TXT_SHORE);
static const TemplateTypeClass Shore17(TEMPLATE_SHORE17,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH17", TXT_SHORE);
static const TemplateTypeClass Shore18(TEMPLATE_SHORE18,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH18", TXT_SHORE);
static const TemplateTypeClass Shore19(TEMPLATE_SHORE19,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH19", TXT_SHORE);
static const TemplateTypeClass Shore20(TEMPLATE_SHORE20,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH20", TXT_SHORE);
static const TemplateTypeClass Shore21(TEMPLATE_SHORE21,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH21", TXT_SHORE);
static const TemplateTypeClass Shore22(TEMPLATE_SHORE22,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH22", TXT_SHORE);
static const TemplateTypeClass Shore23(TEMPLATE_SHORE23,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH23", TXT_SHORE);
static const TemplateTypeClass Shore24(TEMPLATE_SHORE24,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH24", TXT_SHORE);
static const TemplateTypeClass Shore25(TEMPLATE_SHORE25,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH25", TXT_SHORE);
static const TemplateTypeClass Shore26(TEMPLATE_SHORE26,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH26", TXT_SHORE);
static const TemplateTypeClass Shore27(TEMPLATE_SHORE27,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH27", TXT_SHORE);
static const TemplateTypeClass Shore28(TEMPLATE_SHORE28,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH28", TXT_SHORE);
static const TemplateTypeClass Shore29(TEMPLATE_SHORE29,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH29", TXT_SHORE);
static const TemplateTypeClass Shore30(TEMPLATE_SHORE30,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH30", TXT_SHORE);
static const TemplateTypeClass Shore31(TEMPLATE_SHORE31,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH31", TXT_SHORE);
static const TemplateTypeClass Shore32(TEMPLATE_SHORE32,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH32", TXT_SHORE);
static const TemplateTypeClass Shore33(TEMPLATE_SHORE33,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH33", TXT_SHORE);
static const TemplateTypeClass Shore34(TEMPLATE_SHORE34,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH34", TXT_SHORE);
static const TemplateTypeClass Shore35(TEMPLATE_SHORE35,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH35", TXT_SHORE);
static const TemplateTypeClass Shore36(TEMPLATE_SHORE36,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH36", TXT_SHORE);
static const TemplateTypeClass Shore37(TEMPLATE_SHORE37,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH37", TXT_SHORE);
static const TemplateTypeClass Shore38(TEMPLATE_SHORE38,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH38", TXT_SHORE);
static const TemplateTypeClass Shore39(TEMPLATE_SHORE39,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH39", TXT_SHORE);
static const TemplateTypeClass Shore40(TEMPLATE_SHORE40,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH40", TXT_SHORE);
static const TemplateTypeClass Shore41(TEMPLATE_SHORE41,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH41", TXT_SHORE);
static const TemplateTypeClass Shore42(TEMPLATE_SHORE42,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH42", TXT_SHORE);
static const TemplateTypeClass Shore43(TEMPLATE_SHORE43,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH43", TXT_SHORE);
static const TemplateTypeClass Shore44(TEMPLATE_SHORE44,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH44", TXT_SHORE);
static const TemplateTypeClass Shore45(TEMPLATE_SHORE45,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH45", TXT_SHORE);
static const TemplateTypeClass Shore46(TEMPLATE_SHORE46,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH46", TXT_SHORE);
static const TemplateTypeClass Shore47(TEMPLATE_SHORE47,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH47", TXT_SHORE);
static const TemplateTypeClass Shore48(TEMPLATE_SHORE48,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH48", TXT_SHORE);
static const TemplateTypeClass Shore49(TEMPLATE_SHORE49,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH49", TXT_SHORE);
static const TemplateTypeClass Shore50(TEMPLATE_SHORE50,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH50", TXT_SHORE);
static const TemplateTypeClass Shore51(TEMPLATE_SHORE51,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH51", TXT_SHORE);
static const TemplateTypeClass Shore52(TEMPLATE_SHORE52,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH52", TXT_SHORE);
static const TemplateTypeClass Shore53(TEMPLATE_SHORE53,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH53", TXT_SHORE);
static const TemplateTypeClass Shore54(TEMPLATE_SHORE54,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH54", TXT_SHORE);
static const TemplateTypeClass Shore55(TEMPLATE_SHORE55,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH55", TXT_SHORE);
static const TemplateTypeClass Shore56(TEMPLATE_SHORE56,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "SH56", TXT_SHORE);
static const TemplateTypeClass Boulder1(TEMPLATE_BOULDER1,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "B1", TXT_SLOPE);
static const TemplateTypeClass Boulder2(TEMPLATE_BOULDER2,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "B2", TXT_SLOPE);
static const TemplateTypeClass Boulder3(TEMPLATE_BOULDER3,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "B3", TXT_SLOPE);
static const TemplateTypeClass Boulder4(TEMPLATE_BOULDER4,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "B4", TXT_SLOPE);
static const TemplateTypeClass Boulder5(TEMPLATE_BOULDER5,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "B5", TXT_SLOPE);
static const TemplateTypeClass Boulder6(TEMPLATE_BOULDER6,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "B6", TXT_SLOPE);
static const TemplateTypeClass Slope01(TEMPLATE_SLOPE01,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S01", TXT_SLOPE);
static const TemplateTypeClass Slope02(TEMPLATE_SLOPE02,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S02", TXT_SLOPE);
static const TemplateTypeClass Slope03(TEMPLATE_SLOPE03,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S03", TXT_SLOPE);
static const TemplateTypeClass Slope04(TEMPLATE_SLOPE04,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S04", TXT_SLOPE);
static const TemplateTypeClass Slope05(TEMPLATE_SLOPE05,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S05", TXT_SLOPE);
static const TemplateTypeClass Slope06(TEMPLATE_SLOPE06,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S06", TXT_SLOPE);
static const TemplateTypeClass Slope07(TEMPLATE_SLOPE07,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S07", TXT_SLOPE);
static const TemplateTypeClass Slope08(TEMPLATE_SLOPE08,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S08", TXT_SLOPE);
static const TemplateTypeClass Slope09(TEMPLATE_SLOPE09,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S09", TXT_SLOPE);
static const TemplateTypeClass Slope10(TEMPLATE_SLOPE10,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S10", TXT_SLOPE);
static const TemplateTypeClass Slope11(TEMPLATE_SLOPE11,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S11", TXT_SLOPE);
static const TemplateTypeClass Slope12(TEMPLATE_SLOPE12,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S12", TXT_SLOPE);
static const TemplateTypeClass Slope13(TEMPLATE_SLOPE13,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S13", TXT_SLOPE);
static const TemplateTypeClass Slope14(TEMPLATE_SLOPE14,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S14", TXT_SLOPE);
static const TemplateTypeClass Slope15(TEMPLATE_SLOPE15,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S15", TXT_SLOPE);
static const TemplateTypeClass Slope16(TEMPLATE_SLOPE16,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S16", TXT_SLOPE);
static const TemplateTypeClass Slope17(TEMPLATE_SLOPE17,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S17", TXT_SLOPE);
static const TemplateTypeClass Slope18(TEMPLATE_SLOPE18,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S18", TXT_SLOPE);
static const TemplateTypeClass Slope19(TEMPLATE_SLOPE19,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S19", TXT_SLOPE);
static const TemplateTypeClass Slope20(TEMPLATE_SLOPE20,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S20", TXT_SLOPE);
static const TemplateTypeClass Slope21(TEMPLATE_SLOPE21,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S21", TXT_SLOPE);
static const TemplateTypeClass Slope22(TEMPLATE_SLOPE22,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S22", TXT_SLOPE);
static const TemplateTypeClass Slope23(TEMPLATE_SLOPE23,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S23", TXT_SLOPE);
static const TemplateTypeClass Slope24(TEMPLATE_SLOPE24,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S24", TXT_SLOPE);
static const TemplateTypeClass Slope25(TEMPLATE_SLOPE25,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S25", TXT_SLOPE);
static const TemplateTypeClass Slope26(TEMPLATE_SLOPE26,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S26", TXT_SLOPE);
static const TemplateTypeClass Slope27(TEMPLATE_SLOPE27,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S27", TXT_SLOPE);
static const TemplateTypeClass Slope28(TEMPLATE_SLOPE28,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S28", TXT_SLOPE);
static const TemplateTypeClass Slope29(TEMPLATE_SLOPE29,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S29", TXT_SLOPE);
static const TemplateTypeClass Slope30(TEMPLATE_SLOPE30,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S30", TXT_SLOPE);
static const TemplateTypeClass Slope31(TEMPLATE_SLOPE31,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S31", TXT_SLOPE);
static const TemplateTypeClass Slope32(TEMPLATE_SLOPE32,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S32", TXT_SLOPE);
static const TemplateTypeClass Slope33(TEMPLATE_SLOPE33,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S33", TXT_SLOPE);
static const TemplateTypeClass Slope34(TEMPLATE_SLOPE34,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S34", TXT_SLOPE);
static const TemplateTypeClass Slope35(TEMPLATE_SLOPE35,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S35", TXT_SLOPE);
static const TemplateTypeClass Slope36(TEMPLATE_SLOPE36,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S36", TXT_SLOPE);
static const TemplateTypeClass Slope37(TEMPLATE_SLOPE37,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S37", TXT_SLOPE);
static const TemplateTypeClass Slope38(TEMPLATE_SLOPE38,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "S38", TXT_SLOPE);
static const TemplateTypeClass Patch01(TEMPLATE_PATCH01,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "P01", TXT_PATCH);
static const TemplateTypeClass Patch02(TEMPLATE_PATCH02,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "P02", TXT_PATCH);
static const TemplateTypeClass Patch03(TEMPLATE_PATCH03,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "P03", TXT_PATCH);
static const TemplateTypeClass Patch04(TEMPLATE_PATCH04,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "P04", TXT_PATCH);
static const TemplateTypeClass Patch07(TEMPLATE_PATCH07,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "P07", TXT_PATCH);
static const TemplateTypeClass Patch08(TEMPLATE_PATCH08,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "P08", TXT_PATCH);
static const TemplateTypeClass Patch13(TEMPLATE_PATCH13,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "P13", TXT_PATCH);
static const TemplateTypeClass Patch14(TEMPLATE_PATCH14,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "P14", TXT_PATCH);
static const TemplateTypeClass Patch15(TEMPLATE_PATCH15,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "P15", TXT_PATCH);
static const TemplateTypeClass River01(TEMPLATE_RIVER01,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV01", TXT_RIVER);
static const TemplateTypeClass River02(TEMPLATE_RIVER02,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV02", TXT_RIVER);
static const TemplateTypeClass River03(TEMPLATE_RIVER03,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV03", TXT_RIVER);
static const TemplateTypeClass River04(TEMPLATE_RIVER04,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV04", TXT_RIVER);
static const TemplateTypeClass River05(TEMPLATE_RIVER05,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV05", TXT_RIVER);
static const TemplateTypeClass River06(TEMPLATE_RIVER06,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV06", TXT_RIVER);
static const TemplateTypeClass River07(TEMPLATE_RIVER07,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV07", TXT_RIVER);
static const TemplateTypeClass River08(TEMPLATE_RIVER08,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV08", TXT_RIVER);
static const TemplateTypeClass River09(TEMPLATE_RIVER09,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV09", TXT_RIVER);
static const TemplateTypeClass River10(TEMPLATE_RIVER10,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV10", TXT_RIVER);
static const TemplateTypeClass River11(TEMPLATE_RIVER11,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV11", TXT_RIVER);
static const TemplateTypeClass River12(TEMPLATE_RIVER12,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV12", TXT_RIVER);
static const TemplateTypeClass River13(TEMPLATE_RIVER13,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV13", TXT_RIVER);
static const TemplateTypeClass River14(TEMPLATE_RIVER14,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV14", TXT_RIVER);
static const TemplateTypeClass River15(TEMPLATE_RIVER15,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RV15", TXT_RIVER);
static const TemplateTypeClass Ford1(TEMPLATE_FORD1,
                                     kTheaterFlagTemperate | kTheaterFlagSnow,
                                     "FORD1", TXT_RIVER);
static const TemplateTypeClass Ford2(TEMPLATE_FORD2,
                                     kTheaterFlagTemperate | kTheaterFlagSnow,
                                     "FORD2", TXT_RIVER);
static const TemplateTypeClass Falls1(TEMPLATE_FALLS1,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "FALLS1", TXT_RIVER);
static const TemplateTypeClass Falls1a(TEMPLATE_FALLS1A,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "FALLS1A", TXT_RIVER);
static const TemplateTypeClass Falls2(TEMPLATE_FALLS2,
                                      kTheaterFlagTemperate | kTheaterFlagSnow,
                                      "FALLS2", TXT_RIVER);
static const TemplateTypeClass Falls2a(TEMPLATE_FALLS2A,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "FALLS2A", TXT_RIVER);
static const TemplateTypeClass Bridge1x(TEMPLATE_BRIDGE1X,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BRIDGE1X", TXT_BRIDGE);
static const TemplateTypeClass Bridge1(TEMPLATE_BRIDGE1,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "BRIDGE1", TXT_BRIDGE);
static const TemplateTypeClass Bridge1h(TEMPLATE_BRIDGE1H,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BRIDGE1H", TXT_BRIDGE);
static const TemplateTypeClass Bridge1d(TEMPLATE_BRIDGE1D,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BRIDGE1D", TXT_BRIDGE);
static const TemplateTypeClass Bridge2x(TEMPLATE_BRIDGE2X,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BRIDGE2X", TXT_BRIDGE);
static const TemplateTypeClass Bridge2(TEMPLATE_BRIDGE2,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "BRIDGE2", TXT_BRIDGE);
static const TemplateTypeClass Bridge2h(TEMPLATE_BRIDGE2H,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BRIDGE2H", TXT_BRIDGE);
static const TemplateTypeClass Bridge2d(TEMPLATE_BRIDGE2D,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BRIDGE2D", TXT_BRIDGE);
static const TemplateTypeClass Bridge1ax(TEMPLATE_BRIDGE_1AX,
                                         kTheaterFlagTemperate |
                                             kTheaterFlagSnow,
                                         "BR1X", TXT_BRIDGE);
static const TemplateTypeClass Bridge1a(TEMPLATE_BRIDGE_1A,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BR1A", TXT_BRIDGE);
static const TemplateTypeClass Bridge1b(TEMPLATE_BRIDGE_1B,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BR1B", TXT_BRIDGE);
static const TemplateTypeClass Bridge1c(TEMPLATE_BRIDGE_1C,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BR1C", TXT_BRIDGE);
static const TemplateTypeClass Bridge2ax(TEMPLATE_BRIDGE_2AX,
                                         kTheaterFlagTemperate |
                                             kTheaterFlagSnow,
                                         "BR2X", TXT_BRIDGE);
static const TemplateTypeClass Bridge2a(TEMPLATE_BRIDGE_2A,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BR2A", TXT_BRIDGE);
static const TemplateTypeClass Bridge2b(TEMPLATE_BRIDGE_2B,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BR2B", TXT_BRIDGE);
static const TemplateTypeClass Bridge2c(TEMPLATE_BRIDGE_2C,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BR2C", TXT_BRIDGE);
static const TemplateTypeClass Bridge3a(TEMPLATE_BRIDGE_3A,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BR3A", TXT_BRIDGE);
static const TemplateTypeClass Bridge3b(TEMPLATE_BRIDGE_3B,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BR3B", TXT_BRIDGE);
static const TemplateTypeClass Bridge3c(TEMPLATE_BRIDGE_3C,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BR3C", TXT_BRIDGE);
static const TemplateTypeClass Bridge3d(TEMPLATE_BRIDGE_3D,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BR3D", TXT_BRIDGE);
static const TemplateTypeClass Bridge3e(TEMPLATE_BRIDGE_3E,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BR3E", TXT_BRIDGE);
static const TemplateTypeClass Bridge3f(TEMPLATE_BRIDGE_3F,
                                        kTheaterFlagTemperate |
                                            kTheaterFlagSnow,
                                        "BR3F", TXT_BRIDGE);
static const TemplateTypeClass ShoreCliff01(TEMPLATE_SHORECLIFF01,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC01", TXT_SHORE);
static const TemplateTypeClass ShoreCliff02(TEMPLATE_SHORECLIFF02,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC02", TXT_SHORE);
static const TemplateTypeClass ShoreCliff03(TEMPLATE_SHORECLIFF03,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC03", TXT_SHORE);
static const TemplateTypeClass ShoreCliff04(TEMPLATE_SHORECLIFF04,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC04", TXT_SHORE);
static const TemplateTypeClass ShoreCliff05(TEMPLATE_SHORECLIFF05,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC05", TXT_SHORE);
static const TemplateTypeClass ShoreCliff06(TEMPLATE_SHORECLIFF06,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC06", TXT_SHORE);
static const TemplateTypeClass ShoreCliff07(TEMPLATE_SHORECLIFF07,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC07", TXT_SHORE);
static const TemplateTypeClass ShoreCliff08(TEMPLATE_SHORECLIFF08,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC08", TXT_SHORE);
static const TemplateTypeClass ShoreCliff09(TEMPLATE_SHORECLIFF09,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC09", TXT_SHORE);
static const TemplateTypeClass ShoreCliff10(TEMPLATE_SHORECLIFF10,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC10", TXT_SHORE);
static const TemplateTypeClass ShoreCliff11(TEMPLATE_SHORECLIFF11,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC11", TXT_SHORE);
static const TemplateTypeClass ShoreCliff12(TEMPLATE_SHORECLIFF12,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC12", TXT_SHORE);
static const TemplateTypeClass ShoreCliff13(TEMPLATE_SHORECLIFF13,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC13", TXT_SHORE);
static const TemplateTypeClass ShoreCliff14(TEMPLATE_SHORECLIFF14,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC14", TXT_SHORE);
static const TemplateTypeClass ShoreCliff15(TEMPLATE_SHORECLIFF15,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC15", TXT_SHORE);
static const TemplateTypeClass ShoreCliff16(TEMPLATE_SHORECLIFF16,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC16", TXT_SHORE);
static const TemplateTypeClass ShoreCliff17(TEMPLATE_SHORECLIFF17,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC17", TXT_SHORE);
static const TemplateTypeClass ShoreCliff18(TEMPLATE_SHORECLIFF18,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC18", TXT_SHORE);
static const TemplateTypeClass ShoreCliff19(TEMPLATE_SHORECLIFF19,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC19", TXT_SHORE);
static const TemplateTypeClass ShoreCliff20(TEMPLATE_SHORECLIFF20,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC20", TXT_SHORE);
static const TemplateTypeClass ShoreCliff21(TEMPLATE_SHORECLIFF21,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC21", TXT_SHORE);
static const TemplateTypeClass ShoreCliff22(TEMPLATE_SHORECLIFF22,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC22", TXT_SHORE);
static const TemplateTypeClass ShoreCliff23(TEMPLATE_SHORECLIFF23,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC23", TXT_SHORE);
static const TemplateTypeClass ShoreCliff24(TEMPLATE_SHORECLIFF24,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC24", TXT_SHORE);
static const TemplateTypeClass ShoreCliff25(TEMPLATE_SHORECLIFF25,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC25", TXT_SHORE);
static const TemplateTypeClass ShoreCliff26(TEMPLATE_SHORECLIFF26,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC26", TXT_SHORE);
static const TemplateTypeClass ShoreCliff27(TEMPLATE_SHORECLIFF27,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC27", TXT_SHORE);
static const TemplateTypeClass ShoreCliff28(TEMPLATE_SHORECLIFF28,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC28", TXT_SHORE);
static const TemplateTypeClass ShoreCliff29(TEMPLATE_SHORECLIFF29,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC29", TXT_SHORE);
static const TemplateTypeClass ShoreCliff30(TEMPLATE_SHORECLIFF30,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC30", TXT_SHORE);
static const TemplateTypeClass ShoreCliff31(TEMPLATE_SHORECLIFF31,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC31", TXT_SHORE);
static const TemplateTypeClass ShoreCliff32(TEMPLATE_SHORECLIFF32,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC32", TXT_SHORE);
static const TemplateTypeClass ShoreCliff33(TEMPLATE_SHORECLIFF33,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC33", TXT_SHORE);
static const TemplateTypeClass ShoreCliff34(TEMPLATE_SHORECLIFF34,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC34", TXT_SHORE);
static const TemplateTypeClass ShoreCliff35(TEMPLATE_SHORECLIFF35,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC35", TXT_SHORE);
static const TemplateTypeClass ShoreCliff36(TEMPLATE_SHORECLIFF36,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC36", TXT_SHORE);
static const TemplateTypeClass ShoreCliff37(TEMPLATE_SHORECLIFF37,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC37", TXT_SHORE);
static const TemplateTypeClass ShoreCliff38(TEMPLATE_SHORECLIFF38,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "WC38", TXT_SHORE);
static const TemplateTypeClass Rough01(TEMPLATE_ROUGH01,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RF01", TXT_ROCK);
static const TemplateTypeClass Rough02(TEMPLATE_ROUGH02,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RF02", TXT_ROCK);
static const TemplateTypeClass Rough03(TEMPLATE_ROUGH03,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RF03", TXT_ROCK);
static const TemplateTypeClass Rough04(TEMPLATE_ROUGH04,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RF04", TXT_ROCK);
static const TemplateTypeClass Rough05(TEMPLATE_ROUGH05,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RF05", TXT_ROCK);
static const TemplateTypeClass Rough06(TEMPLATE_ROUGH06,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RF06", TXT_ROCK);
static const TemplateTypeClass Rough07(TEMPLATE_ROUGH07,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RF07", TXT_ROCK);
static const TemplateTypeClass Rough08(TEMPLATE_ROUGH08,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RF08", TXT_ROCK);
static const TemplateTypeClass Rough09(TEMPLATE_ROUGH09,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RF09", TXT_ROCK);
static const TemplateTypeClass Rough10(TEMPLATE_ROUGH10,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RF10", TXT_ROCK);
static const TemplateTypeClass Rough11(TEMPLATE_ROUGH11,
                                       kTheaterFlagTemperate | kTheaterFlagSnow,
                                       "RF11", TXT_ROCK);
static const TemplateTypeClass RiverCliff01(TEMPLATE_RIVERCLIFF01,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "RC01", TXT_RIVER);
static const TemplateTypeClass RiverCliff02(TEMPLATE_RIVERCLIFF02,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "RC02", TXT_RIVER);
static const TemplateTypeClass RiverCliff03(TEMPLATE_RIVERCLIFF03,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "RC03", TXT_RIVER);
static const TemplateTypeClass RiverCliff04(TEMPLATE_RIVERCLIFF04,
                                            kTheaterFlagTemperate |
                                                kTheaterFlagSnow,
                                            "RC04", TXT_RIVER);

static const TemplateTypeClass F01(TEMPLATE_F01,
                                   kTheaterFlagTemperate | kTheaterFlagSnow,
                                   "F01", TXT_RIVER);
static const TemplateTypeClass F02(TEMPLATE_F02,
                                   kTheaterFlagTemperate | kTheaterFlagSnow,
                                   "F02", TXT_RIVER);
static const TemplateTypeClass F03(TEMPLATE_F03,
                                   kTheaterFlagTemperate | kTheaterFlagSnow,
                                   "F03", TXT_RIVER);
static const TemplateTypeClass F04(TEMPLATE_F04,
                                   kTheaterFlagTemperate | kTheaterFlagSnow,
                                   "F04", TXT_RIVER);
static const TemplateTypeClass F05(TEMPLATE_F05,
                                   kTheaterFlagTemperate | kTheaterFlagSnow,
                                   "F05", TXT_RIVER);
static const TemplateTypeClass F06(TEMPLATE_F06,
                                   kTheaterFlagTemperate | kTheaterFlagSnow,
                                   "F06", TXT_RIVER);

static const TemplateTypeClass ARRO0001(TEMPLATE_ARRO0001, kTheaterFlagInterior,
                                        "ARRO0001", TXT_INTERIOR);
static const TemplateTypeClass ARRO0002(TEMPLATE_ARRO0002, kTheaterFlagInterior,
                                        "ARRO0002", TXT_INTERIOR);
static const TemplateTypeClass ARRO0003(TEMPLATE_ARRO0003, kTheaterFlagInterior,
                                        "ARRO0003", TXT_INTERIOR);
static const TemplateTypeClass ARRO0004(TEMPLATE_ARRO0004, kTheaterFlagInterior,
                                        "ARRO0004", TXT_INTERIOR);
static const TemplateTypeClass ARRO0005(TEMPLATE_ARRO0005, kTheaterFlagInterior,
                                        "ARRO0005", TXT_INTERIOR);
static const TemplateTypeClass ARRO0006(TEMPLATE_ARRO0006, kTheaterFlagInterior,
                                        "ARRO0006", TXT_INTERIOR);
static const TemplateTypeClass ARRO0007(TEMPLATE_ARRO0007, kTheaterFlagInterior,
                                        "ARRO0007", TXT_INTERIOR);
static const TemplateTypeClass ARRO0008(TEMPLATE_ARRO0008, kTheaterFlagInterior,
                                        "ARRO0008", TXT_INTERIOR);
static const TemplateTypeClass ARRO0009(TEMPLATE_ARRO0009, kTheaterFlagInterior,
                                        "ARRO0009", TXT_INTERIOR);
static const TemplateTypeClass ARRO0010(TEMPLATE_ARRO0010, kTheaterFlagInterior,
                                        "ARRO0010", TXT_INTERIOR);
static const TemplateTypeClass ARRO0011(TEMPLATE_ARRO0011, kTheaterFlagInterior,
                                        "ARRO0011", TXT_INTERIOR);
static const TemplateTypeClass ARRO0012(TEMPLATE_ARRO0012, kTheaterFlagInterior,
                                        "ARRO0012", TXT_INTERIOR);
static const TemplateTypeClass ARRO0013(TEMPLATE_ARRO0013, kTheaterFlagInterior,
                                        "ARRO0013", TXT_INTERIOR);
static const TemplateTypeClass ARRO0014(TEMPLATE_ARRO0014, kTheaterFlagInterior,
                                        "ARRO0014", TXT_INTERIOR);
static const TemplateTypeClass ARRO0015(TEMPLATE_ARRO0015, kTheaterFlagInterior,
                                        "ARRO0015", TXT_INTERIOR);
static const TemplateTypeClass FLOR0001(TEMPLATE_FLOR0001, kTheaterFlagInterior,
                                        "FLOR0001", TXT_INTERIOR);
static const TemplateTypeClass FLOR0002(TEMPLATE_FLOR0002, kTheaterFlagInterior,
                                        "FLOR0002", TXT_INTERIOR);
static const TemplateTypeClass FLOR0003(TEMPLATE_FLOR0003, kTheaterFlagInterior,
                                        "FLOR0003", TXT_INTERIOR);
static const TemplateTypeClass FLOR0004(TEMPLATE_FLOR0004, kTheaterFlagInterior,
                                        "FLOR0004", TXT_INTERIOR);
static const TemplateTypeClass FLOR0005(TEMPLATE_FLOR0005, kTheaterFlagInterior,
                                        "FLOR0005", TXT_INTERIOR);
static const TemplateTypeClass FLOR0006(TEMPLATE_FLOR0006, kTheaterFlagInterior,
                                        "FLOR0006", TXT_INTERIOR);
static const TemplateTypeClass FLOR0007(TEMPLATE_FLOR0007, kTheaterFlagInterior,
                                        "FLOR0007", TXT_INTERIOR);
static const TemplateTypeClass GFLR0001(TEMPLATE_GFLR0001, kTheaterFlagInterior,
                                        "GFLR0001", TXT_INTERIOR);
static const TemplateTypeClass GFLR0002(TEMPLATE_GFLR0002, kTheaterFlagInterior,
                                        "GFLR0002", TXT_INTERIOR);
static const TemplateTypeClass GFLR0003(TEMPLATE_GFLR0003, kTheaterFlagInterior,
                                        "GFLR0003", TXT_INTERIOR);
static const TemplateTypeClass GFLR0004(TEMPLATE_GFLR0004, kTheaterFlagInterior,
                                        "GFLR0004", TXT_INTERIOR);
static const TemplateTypeClass GFLR0005(TEMPLATE_GFLR0005, kTheaterFlagInterior,
                                        "GFLR0005", TXT_INTERIOR);
static const TemplateTypeClass GSTR0001(TEMPLATE_GSTR0001, kTheaterFlagInterior,
                                        "GSTR0001", TXT_INTERIOR);
static const TemplateTypeClass GSTR0002(TEMPLATE_GSTR0002, kTheaterFlagInterior,
                                        "GSTR0002", TXT_INTERIOR);
static const TemplateTypeClass GSTR0003(TEMPLATE_GSTR0003, kTheaterFlagInterior,
                                        "GSTR0003", TXT_INTERIOR);
static const TemplateTypeClass GSTR0004(TEMPLATE_GSTR0004, kTheaterFlagInterior,
                                        "GSTR0004", TXT_INTERIOR);
static const TemplateTypeClass GSTR0005(TEMPLATE_GSTR0005, kTheaterFlagInterior,
                                        "GSTR0005", TXT_INTERIOR);
static const TemplateTypeClass GSTR0006(TEMPLATE_GSTR0006, kTheaterFlagInterior,
                                        "GSTR0006", TXT_INTERIOR);
static const TemplateTypeClass GSTR0007(TEMPLATE_GSTR0007, kTheaterFlagInterior,
                                        "GSTR0007", TXT_INTERIOR);
static const TemplateTypeClass GSTR0008(TEMPLATE_GSTR0008, kTheaterFlagInterior,
                                        "GSTR0008", TXT_INTERIOR);
static const TemplateTypeClass GSTR0009(TEMPLATE_GSTR0009, kTheaterFlagInterior,
                                        "GSTR0009", TXT_INTERIOR);
static const TemplateTypeClass GSTR0010(TEMPLATE_GSTR0010, kTheaterFlagInterior,
                                        "GSTR0010", TXT_INTERIOR);
static const TemplateTypeClass GSTR0011(TEMPLATE_GSTR0011, kTheaterFlagInterior,
                                        "GSTR0011", TXT_INTERIOR);
static const TemplateTypeClass LWAL0001(TEMPLATE_LWAL0001, kTheaterFlagInterior,
                                        "LWAL0001", TXT_INTERIOR);
static const TemplateTypeClass LWAL0002(TEMPLATE_LWAL0002, kTheaterFlagInterior,
                                        "LWAL0002", TXT_INTERIOR);
static const TemplateTypeClass LWAL0003(TEMPLATE_LWAL0003, kTheaterFlagInterior,
                                        "LWAL0003", TXT_INTERIOR);
static const TemplateTypeClass LWAL0004(TEMPLATE_LWAL0004, kTheaterFlagInterior,
                                        "LWAL0004", TXT_INTERIOR);
static const TemplateTypeClass LWAL0005(TEMPLATE_LWAL0005, kTheaterFlagInterior,
                                        "LWAL0005", TXT_INTERIOR);
static const TemplateTypeClass LWAL0006(TEMPLATE_LWAL0006, kTheaterFlagInterior,
                                        "LWAL0006", TXT_INTERIOR);
static const TemplateTypeClass LWAL0007(TEMPLATE_LWAL0007, kTheaterFlagInterior,
                                        "LWAL0007", TXT_INTERIOR);
static const TemplateTypeClass LWAL0008(TEMPLATE_LWAL0008, kTheaterFlagInterior,
                                        "LWAL0008", TXT_INTERIOR);
static const TemplateTypeClass LWAL0009(TEMPLATE_LWAL0009, kTheaterFlagInterior,
                                        "LWAL0009", TXT_INTERIOR);
static const TemplateTypeClass LWAL0010(TEMPLATE_LWAL0010, kTheaterFlagInterior,
                                        "LWAL0010", TXT_INTERIOR);
static const TemplateTypeClass LWAL0011(TEMPLATE_LWAL0011, kTheaterFlagInterior,
                                        "LWAL0011", TXT_INTERIOR);
static const TemplateTypeClass LWAL0012(TEMPLATE_LWAL0012, kTheaterFlagInterior,
                                        "LWAL0012", TXT_INTERIOR);
static const TemplateTypeClass LWAL0013(TEMPLATE_LWAL0013, kTheaterFlagInterior,
                                        "LWAL0013", TXT_INTERIOR);
static const TemplateTypeClass LWAL0014(TEMPLATE_LWAL0014, kTheaterFlagInterior,
                                        "LWAL0014", TXT_INTERIOR);
static const TemplateTypeClass LWAL0015(TEMPLATE_LWAL0015, kTheaterFlagInterior,
                                        "LWAL0015", TXT_INTERIOR);
static const TemplateTypeClass LWAL0016(TEMPLATE_LWAL0016, kTheaterFlagInterior,
                                        "LWAL0016", TXT_INTERIOR);
static const TemplateTypeClass LWAL0017(TEMPLATE_LWAL0017, kTheaterFlagInterior,
                                        "LWAL0017", TXT_INTERIOR);
static const TemplateTypeClass LWAL0018(TEMPLATE_LWAL0018, kTheaterFlagInterior,
                                        "LWAL0018", TXT_INTERIOR);
static const TemplateTypeClass LWAL0019(TEMPLATE_LWAL0019, kTheaterFlagInterior,
                                        "LWAL0019", TXT_INTERIOR);
static const TemplateTypeClass LWAL0020(TEMPLATE_LWAL0020, kTheaterFlagInterior,
                                        "LWAL0020", TXT_INTERIOR);
static const TemplateTypeClass LWAL0021(TEMPLATE_LWAL0021, kTheaterFlagInterior,
                                        "LWAL0021", TXT_INTERIOR);
static const TemplateTypeClass LWAL0022(TEMPLATE_LWAL0022, kTheaterFlagInterior,
                                        "LWAL0022", TXT_INTERIOR);
static const TemplateTypeClass LWAL0023(TEMPLATE_LWAL0023, kTheaterFlagInterior,
                                        "LWAL0023", TXT_INTERIOR);
static const TemplateTypeClass LWAL0024(TEMPLATE_LWAL0024, kTheaterFlagInterior,
                                        "LWAL0024", TXT_INTERIOR);
static const TemplateTypeClass LWAL0025(TEMPLATE_LWAL0025, kTheaterFlagInterior,
                                        "LWAL0025", TXT_INTERIOR);
static const TemplateTypeClass LWAL0026(TEMPLATE_LWAL0026, kTheaterFlagInterior,
                                        "LWAL0026", TXT_INTERIOR);
static const TemplateTypeClass LWAL0027(TEMPLATE_LWAL0027, kTheaterFlagInterior,
                                        "LWAL0027", TXT_INTERIOR);
static const TemplateTypeClass STRP0001(TEMPLATE_STRP0001, kTheaterFlagInterior,
                                        "STRP0001", TXT_INTERIOR);
static const TemplateTypeClass STRP0002(TEMPLATE_STRP0002, kTheaterFlagInterior,
                                        "STRP0002", TXT_INTERIOR);
static const TemplateTypeClass STRP0003(TEMPLATE_STRP0003, kTheaterFlagInterior,
                                        "STRP0003", TXT_INTERIOR);
static const TemplateTypeClass STRP0004(TEMPLATE_STRP0004, kTheaterFlagInterior,
                                        "STRP0004", TXT_INTERIOR);
static const TemplateTypeClass STRP0005(TEMPLATE_STRP0005, kTheaterFlagInterior,
                                        "STRP0005", TXT_INTERIOR);
static const TemplateTypeClass STRP0006(TEMPLATE_STRP0006, kTheaterFlagInterior,
                                        "STRP0006", TXT_INTERIOR);
static const TemplateTypeClass STRP0007(TEMPLATE_STRP0007, kTheaterFlagInterior,
                                        "STRP0007", TXT_INTERIOR);
static const TemplateTypeClass STRP0008(TEMPLATE_STRP0008, kTheaterFlagInterior,
                                        "STRP0008", TXT_INTERIOR);
static const TemplateTypeClass STRP0009(TEMPLATE_STRP0009, kTheaterFlagInterior,
                                        "STRP0009", TXT_INTERIOR);
static const TemplateTypeClass STRP0010(TEMPLATE_STRP0010, kTheaterFlagInterior,
                                        "STRP0010", TXT_INTERIOR);
static const TemplateTypeClass STRP0011(TEMPLATE_STRP0011, kTheaterFlagInterior,
                                        "STRP0011", TXT_INTERIOR);
static const TemplateTypeClass WALL0001(TEMPLATE_WALL0001, kTheaterFlagInterior,
                                        "WALL0001", TXT_INTERIOR);
static const TemplateTypeClass WALL0002(TEMPLATE_WALL0002, kTheaterFlagInterior,
                                        "WALL0002", TXT_INTERIOR);
static const TemplateTypeClass WALL0003(TEMPLATE_WALL0003, kTheaterFlagInterior,
                                        "WALL0003", TXT_INTERIOR);
static const TemplateTypeClass WALL0004(TEMPLATE_WALL0004, kTheaterFlagInterior,
                                        "WALL0004", TXT_INTERIOR);
static const TemplateTypeClass WALL0005(TEMPLATE_WALL0005, kTheaterFlagInterior,
                                        "WALL0005", TXT_INTERIOR);
static const TemplateTypeClass WALL0006(TEMPLATE_WALL0006, kTheaterFlagInterior,
                                        "WALL0006", TXT_INTERIOR);
static const TemplateTypeClass WALL0007(TEMPLATE_WALL0007, kTheaterFlagInterior,
                                        "WALL0007", TXT_INTERIOR);
static const TemplateTypeClass WALL0008(TEMPLATE_WALL0008, kTheaterFlagInterior,
                                        "WALL0008", TXT_INTERIOR);
static const TemplateTypeClass WALL0009(TEMPLATE_WALL0009, kTheaterFlagInterior,
                                        "WALL0009", TXT_INTERIOR);
static const TemplateTypeClass WALL0010(TEMPLATE_WALL0010, kTheaterFlagInterior,
                                        "WALL0010", TXT_INTERIOR);
static const TemplateTypeClass WALL0011(TEMPLATE_WALL0011, kTheaterFlagInterior,
                                        "WALL0011", TXT_INTERIOR);
static const TemplateTypeClass WALL0012(TEMPLATE_WALL0012, kTheaterFlagInterior,
                                        "WALL0012", TXT_INTERIOR);
static const TemplateTypeClass WALL0013(TEMPLATE_WALL0013, kTheaterFlagInterior,
                                        "WALL0013", TXT_INTERIOR);
static const TemplateTypeClass WALL0014(TEMPLATE_WALL0014, kTheaterFlagInterior,
                                        "WALL0014", TXT_INTERIOR);
static const TemplateTypeClass WALL0015(TEMPLATE_WALL0015, kTheaterFlagInterior,
                                        "WALL0015", TXT_INTERIOR);
static const TemplateTypeClass WALL0016(TEMPLATE_WALL0016, kTheaterFlagInterior,
                                        "WALL0016", TXT_INTERIOR);
static const TemplateTypeClass WALL0017(TEMPLATE_WALL0017, kTheaterFlagInterior,
                                        "WALL0017", TXT_INTERIOR);
static const TemplateTypeClass WALL0018(TEMPLATE_WALL0018, kTheaterFlagInterior,
                                        "WALL0018", TXT_INTERIOR);
static const TemplateTypeClass WALL0019(TEMPLATE_WALL0019, kTheaterFlagInterior,
                                        "WALL0019", TXT_INTERIOR);
static const TemplateTypeClass WALL0020(TEMPLATE_WALL0020, kTheaterFlagInterior,
                                        "WALL0020", TXT_INTERIOR);
static const TemplateTypeClass WALL0021(TEMPLATE_WALL0021, kTheaterFlagInterior,
                                        "WALL0021", TXT_INTERIOR);
static const TemplateTypeClass WALL0022(TEMPLATE_WALL0022, kTheaterFlagInterior,
                                        "WALL0022", TXT_INTERIOR);
static const TemplateTypeClass WALL0023(TEMPLATE_WALL0023, kTheaterFlagInterior,
                                        "WALL0023", TXT_INTERIOR);
static const TemplateTypeClass WALL0024(TEMPLATE_WALL0024, kTheaterFlagInterior,
                                        "WALL0024", TXT_INTERIOR);
static const TemplateTypeClass WALL0025(TEMPLATE_WALL0025, kTheaterFlagInterior,
                                        "WALL0025", TXT_INTERIOR);
static const TemplateTypeClass WALL0026(TEMPLATE_WALL0026, kTheaterFlagInterior,
                                        "WALL0026", TXT_INTERIOR);
static const TemplateTypeClass WALL0027(TEMPLATE_WALL0027, kTheaterFlagInterior,
                                        "WALL0027", TXT_INTERIOR);
static const TemplateTypeClass WALL0028(TEMPLATE_WALL0028, kTheaterFlagInterior,
                                        "WALL0028", TXT_INTERIOR);
static const TemplateTypeClass WALL0029(TEMPLATE_WALL0029, kTheaterFlagInterior,
                                        "WALL0029", TXT_INTERIOR);
static const TemplateTypeClass WALL0030(TEMPLATE_WALL0030, kTheaterFlagInterior,
                                        "WALL0030", TXT_INTERIOR);
static const TemplateTypeClass WALL0031(TEMPLATE_WALL0031, kTheaterFlagInterior,
                                        "WALL0031", TXT_INTERIOR);
static const TemplateTypeClass WALL0032(TEMPLATE_WALL0032, kTheaterFlagInterior,
                                        "WALL0032", TXT_INTERIOR);
static const TemplateTypeClass WALL0033(TEMPLATE_WALL0033, kTheaterFlagInterior,
                                        "WALL0033", TXT_INTERIOR);
static const TemplateTypeClass WALL0034(TEMPLATE_WALL0034, kTheaterFlagInterior,
                                        "WALL0034", TXT_INTERIOR);
static const TemplateTypeClass WALL0035(TEMPLATE_WALL0035, kTheaterFlagInterior,
                                        "WALL0035", TXT_INTERIOR);
static const TemplateTypeClass WALL0036(TEMPLATE_WALL0036, kTheaterFlagInterior,
                                        "WALL0036", TXT_INTERIOR);
static const TemplateTypeClass WALL0037(TEMPLATE_WALL0037, kTheaterFlagInterior,
                                        "WALL0037", TXT_INTERIOR);
static const TemplateTypeClass WALL0038(TEMPLATE_WALL0038, kTheaterFlagInterior,
                                        "WALL0038", TXT_INTERIOR);
static const TemplateTypeClass WALL0039(TEMPLATE_WALL0039, kTheaterFlagInterior,
                                        "WALL0039", TXT_INTERIOR);
static const TemplateTypeClass WALL0040(TEMPLATE_WALL0040, kTheaterFlagInterior,
                                        "WALL0040", TXT_INTERIOR);
static const TemplateTypeClass WALL0041(TEMPLATE_WALL0041, kTheaterFlagInterior,
                                        "WALL0041", TXT_INTERIOR);
static const TemplateTypeClass WALL0042(TEMPLATE_WALL0042, kTheaterFlagInterior,
                                        "WALL0042", TXT_INTERIOR);
static const TemplateTypeClass WALL0043(TEMPLATE_WALL0043, kTheaterFlagInterior,
                                        "WALL0043", TXT_INTERIOR);
static const TemplateTypeClass WALL0044(TEMPLATE_WALL0044, kTheaterFlagInterior,
                                        "WALL0044", TXT_INTERIOR);
static const TemplateTypeClass WALL0045(TEMPLATE_WALL0045, kTheaterFlagInterior,
                                        "WALL0045", TXT_INTERIOR);
static const TemplateTypeClass WALL0046(TEMPLATE_WALL0046, kTheaterFlagInterior,
                                        "WALL0046", TXT_INTERIOR);
static const TemplateTypeClass WALL0047(TEMPLATE_WALL0047, kTheaterFlagInterior,
                                        "WALL0047", TXT_INTERIOR);
static const TemplateTypeClass WALL0048(TEMPLATE_WALL0048, kTheaterFlagInterior,
                                        "WALL0048", TXT_INTERIOR);
static const TemplateTypeClass WALL0049(TEMPLATE_WALL0049, kTheaterFlagInterior,
                                        "WALL0049", TXT_INTERIOR);

static const TemplateTypeClass Xtra0001(TEMPLATE_XTRA0001, kTheaterFlagInterior,
                                        "XTRA0001", TXT_INTERIOR);
static const TemplateTypeClass Xtra0002(TEMPLATE_XTRA0002, kTheaterFlagInterior,
                                        "XTRA0002", TXT_INTERIOR);
static const TemplateTypeClass Xtra0003(TEMPLATE_XTRA0003, kTheaterFlagInterior,
                                        "XTRA0003", TXT_INTERIOR);
static const TemplateTypeClass Xtra0004(TEMPLATE_XTRA0004, kTheaterFlagInterior,
                                        "XTRA0004", TXT_INTERIOR);
static const TemplateTypeClass Xtra0005(TEMPLATE_XTRA0005, kTheaterFlagInterior,
                                        "XTRA0005", TXT_INTERIOR);
static const TemplateTypeClass Xtra0006(TEMPLATE_XTRA0006, kTheaterFlagInterior,
                                        "XTRA0006", TXT_INTERIOR);
static const TemplateTypeClass Xtra0007(TEMPLATE_XTRA0007, kTheaterFlagInterior,
                                        "XTRA0007", TXT_INTERIOR);
static const TemplateTypeClass Xtra0008(TEMPLATE_XTRA0008, kTheaterFlagInterior,
                                        "XTRA0008", TXT_INTERIOR);
static const TemplateTypeClass Xtra0009(TEMPLATE_XTRA0009, kTheaterFlagInterior,
                                        "XTRA0009", TXT_INTERIOR);
static const TemplateTypeClass Xtra0010(TEMPLATE_XTRA0010, kTheaterFlagInterior,
                                        "XTRA0010", TXT_INTERIOR);
static const TemplateTypeClass Xtra0011(TEMPLATE_XTRA0011, kTheaterFlagInterior,
                                        "XTRA0011", TXT_INTERIOR);
static const TemplateTypeClass Xtra0012(TEMPLATE_XTRA0012, kTheaterFlagInterior,
                                        "XTRA0012", TXT_INTERIOR);
static const TemplateTypeClass Xtra0013(TEMPLATE_XTRA0013, kTheaterFlagInterior,
                                        "XTRA0013", TXT_INTERIOR);
static const TemplateTypeClass Xtra0014(TEMPLATE_XTRA0014, kTheaterFlagInterior,
                                        "XTRA0014", TXT_INTERIOR);
static const TemplateTypeClass Xtra0015(TEMPLATE_XTRA0015, kTheaterFlagInterior,
                                        "XTRA0015", TXT_INTERIOR);
static const TemplateTypeClass Xtra0016(TEMPLATE_XTRA0016, kTheaterFlagInterior,
                                        "XTRA0016", TXT_INTERIOR);
static const TemplateTypeClass AntHill(TEMPLATE_HILL01, kTheaterFlagTemperate,
                                       "HILL01", TXT_ROCK);

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
                                     const char* ininame, int fullname)
    : ObjectTypeClass(RTTI_TEMPLATETYPE, static_cast<int>(iconset), false, true,
                      false, false, true, true, false, fullname, ininame),
      Type(iconset),
      Theater(theater),
      Width(0),
      Height(0) {}

/***********************************************************************************************
 * TemplateTypeClass::operator new -- Allocates a template type from special
 *heap.             *
 *                                                                                             *
 *    This allocates a template type object from the special heap used for that
 *purpose.       *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the newly allocated template type object.
 *If no object   * could be allocated, then nullptr is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/06/1996 JLB : Created. *
 *=============================================================================================*/
void* TemplateTypeClass::operator new(size_t) noexcept {
  return TemplateTypes.Alloc();
}

/***********************************************************************************************
 * TemplateTypeClass::operator delete -- Deletes a template type object. *
 *                                                                                             *
 *    This routine will return a template type object back to the special heap
 *it was          * allocated from. *
 *                                                                                             *
 * INPUT:   ptr   -- Pointer to the template type object to free. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/06/1996 JLB : Created. *
 *=============================================================================================*/
void TemplateTypeClass::operator delete(void* ptr) {
  TemplateTypes.Free(static_cast<TemplateTypeClass*>(ptr));
}

static void _Watcom_Ugh_Hack() {
  (void)new TemplateTypeClass(Road37);        //	TEMPLATE_ROAD37
  (void)new TemplateTypeClass(Road38);        //	TEMPLATE_ROAD38
  (void)new TemplateTypeClass(Road39);        //	TEMPLATE_ROAD39
  (void)new TemplateTypeClass(Road40);        //	TEMPLATE_ROAD40
  (void)new TemplateTypeClass(Road41);        //	TEMPLATE_ROAD41
  (void)new TemplateTypeClass(Road42);        //	TEMPLATE_ROAD42
  (void)new TemplateTypeClass(Road43);        //	TEMPLATE_ROAD43
  (void)new TemplateTypeClass(Rough01);       // TEMPLATE_ROUGH01
  (void)new TemplateTypeClass(Rough02);       // TEMPLATE_ROUGH02
  (void)new TemplateTypeClass(Rough03);       // TEMPLATE_ROUGH03
  (void)new TemplateTypeClass(Rough04);       // TEMPLATE_ROUGH04
  (void)new TemplateTypeClass(Rough05);       // TEMPLATE_ROUGH05
  (void)new TemplateTypeClass(Rough06);       // TEMPLATE_ROUGH06
  (void)new TemplateTypeClass(Rough07);       // TEMPLATE_ROUGH07
  (void)new TemplateTypeClass(Rough08);       // TEMPLATE_ROUGH08
  (void)new TemplateTypeClass(Rough09);       // TEMPLATE_ROUGH09
  (void)new TemplateTypeClass(Rough10);       // TEMPLATE_ROUGH10
  (void)new TemplateTypeClass(Rough11);       // TEMPLATE_ROUGH11
  (void)new TemplateTypeClass(Road44);        //	TEMPLATE_ROAD44
  (void)new TemplateTypeClass(Road45);        //	TEMPLATE_ROAD45
  (void)new TemplateTypeClass(River14);       //	TEMPLATE_RIVER14
  (void)new TemplateTypeClass(River15);       //	TEMPLATE_RIVER15
  (void)new TemplateTypeClass(RiverCliff01);  //	TEMPLATE_RIVERCLIFF01
  (void)new TemplateTypeClass(RiverCliff02);  //	TEMPLATE_RIVERCLIFF02
  (void)new TemplateTypeClass(RiverCliff03);  //	TEMPLATE_RIVERCLIFF03
  (void)new TemplateTypeClass(RiverCliff04);  //	TEMPLATE_RIVERCLIFF04
  (void)new TemplateTypeClass(Bridge1a);      //	TEMPLATE_BRIDGE_1A
  (void)new TemplateTypeClass(Bridge1b);      //	TEMPLATE_BRIDGE_1B
  (void)new TemplateTypeClass(Bridge1c);      //	TEMPLATE_BRIDGE_1C
  (void)new TemplateTypeClass(Bridge2a);      //	TEMPLATE_BRIDGE_2A
  (void)new TemplateTypeClass(Bridge2b);      //	TEMPLATE_BRIDGE_2B
  (void)new TemplateTypeClass(Bridge2c);      //	TEMPLATE_BRIDGE_2C
  (void)new TemplateTypeClass(Bridge3a);      //	TEMPLATE_BRIDGE_3A
  (void)new TemplateTypeClass(Bridge3b);      //	TEMPLATE_BRIDGE_3B
  (void)new TemplateTypeClass(Bridge3c);      //	TEMPLATE_BRIDGE_3C
  (void)new TemplateTypeClass(Bridge3d);      //	TEMPLATE_BRIDGE_3D
  (void)new TemplateTypeClass(Bridge3e);      //	TEMPLATE_BRIDGE_3E
  (void)new TemplateTypeClass(Bridge3f);      //	TEMPLATE_BRIDGE_3F
  (void)new TemplateTypeClass(F01);           //	TEMPLATE_F01
  (void)new TemplateTypeClass(F02);           //	TEMPLATE_F02
  (void)new TemplateTypeClass(F03);           //	TEMPLATE_F03
  (void)new TemplateTypeClass(F04);           //	TEMPLATE_F04
  (void)new TemplateTypeClass(F05);           //	TEMPLATE_F05
  (void)new TemplateTypeClass(F06);           //	TEMPLATE_F06
  (void)new TemplateTypeClass(ARRO0001);      //	TEMPLATE_ARRO0001
  (void)new TemplateTypeClass(ARRO0002);      //	TEMPLATE_ARRO0002
  (void)new TemplateTypeClass(ARRO0003);      //	TEMPLATE_ARRO0003
  (void)new TemplateTypeClass(ARRO0004);      //	TEMPLATE_ARRO0004
  (void)new TemplateTypeClass(ARRO0005);      //	TEMPLATE_ARRO0005
  (void)new TemplateTypeClass(ARRO0006);      //	TEMPLATE_ARRO0006
  (void)new TemplateTypeClass(ARRO0007);      //	TEMPLATE_ARRO0007
  (void)new TemplateTypeClass(ARRO0008);      //	TEMPLATE_ARRO0008
  (void)new TemplateTypeClass(ARRO0009);      //	TEMPLATE_ARRO0009
  (void)new TemplateTypeClass(ARRO0010);      //	TEMPLATE_ARRO0010
  (void)new TemplateTypeClass(ARRO0011);      //	TEMPLATE_ARRO0011
  (void)new TemplateTypeClass(ARRO0012);      //	TEMPLATE_ARRO0012
  (void)new TemplateTypeClass(ARRO0013);      //	TEMPLATE_ARRO0013
  (void)new TemplateTypeClass(ARRO0014);      //	TEMPLATE_ARRO0014
  (void)new TemplateTypeClass(ARRO0015);      //	TEMPLATE_ARRO0015
  (void)new TemplateTypeClass(FLOR0001);      //	TEMPLATE_FLOR0001
  (void)new TemplateTypeClass(FLOR0002);      //	TEMPLATE_FLOR0002
  (void)new TemplateTypeClass(FLOR0003);      //	TEMPLATE_FLOR0003
  (void)new TemplateTypeClass(FLOR0004);      //	TEMPLATE_FLOR0004
  (void)new TemplateTypeClass(FLOR0005);      //	TEMPLATE_FLOR0005
  (void)new TemplateTypeClass(FLOR0006);      //	TEMPLATE_FLOR0006
  (void)new TemplateTypeClass(FLOR0007);      //	TEMPLATE_FLOR0007
  (void)new TemplateTypeClass(GFLR0001);      //	TEMPLATE_GFLR0001
  (void)new TemplateTypeClass(GFLR0002);      //	TEMPLATE_GFLR0002
  (void)new TemplateTypeClass(GFLR0003);      //	TEMPLATE_GFLR0003
  (void)new TemplateTypeClass(GFLR0004);      //	TEMPLATE_GFLR0004
  (void)new TemplateTypeClass(GFLR0005);      //	TEMPLATE_GFLR0005
  (void)new TemplateTypeClass(GSTR0001);      //	TEMPLATE_GSTR0001
  (void)new TemplateTypeClass(GSTR0002);      //	TEMPLATE_GSTR0002
  (void)new TemplateTypeClass(GSTR0003);      //	TEMPLATE_GSTR0003
  (void)new TemplateTypeClass(GSTR0004);      //	TEMPLATE_GSTR0004
  (void)new TemplateTypeClass(GSTR0005);      //	TEMPLATE_GSTR0005
  (void)new TemplateTypeClass(GSTR0006);      //	TEMPLATE_GSTR0006
  (void)new TemplateTypeClass(GSTR0007);      //	TEMPLATE_GSTR0007
  (void)new TemplateTypeClass(GSTR0008);      //	TEMPLATE_GSTR0008
  (void)new TemplateTypeClass(GSTR0009);      //	TEMPLATE_GSTR0009
  (void)new TemplateTypeClass(GSTR0010);      //	TEMPLATE_GSTR0010
  (void)new TemplateTypeClass(GSTR0011);      //	TEMPLATE_GSTR0011
  (void)new TemplateTypeClass(LWAL0001);      //	TEMPLATE_LWAL0001
  (void)new TemplateTypeClass(LWAL0002);      //	TEMPLATE_LWAL0002
  (void)new TemplateTypeClass(LWAL0003);      //	TEMPLATE_LWAL0003
  (void)new TemplateTypeClass(LWAL0004);      //	TEMPLATE_LWAL0004
  (void)new TemplateTypeClass(LWAL0005);      //	TEMPLATE_LWAL0005
  (void)new TemplateTypeClass(LWAL0006);      //	TEMPLATE_LWAL0006
  (void)new TemplateTypeClass(LWAL0007);      //	TEMPLATE_LWAL0007
  (void)new TemplateTypeClass(LWAL0008);      //	TEMPLATE_LWAL0008
  (void)new TemplateTypeClass(LWAL0009);      //	TEMPLATE_LWAL0009
  (void)new TemplateTypeClass(LWAL0010);      //	TEMPLATE_LWAL0010
  (void)new TemplateTypeClass(LWAL0011);      //	TEMPLATE_LWAL0011
  (void)new TemplateTypeClass(LWAL0012);      //	TEMPLATE_LWAL0012
  (void)new TemplateTypeClass(LWAL0013);      //	TEMPLATE_LWAL0013
  (void)new TemplateTypeClass(LWAL0014);      //	TEMPLATE_LWAL0014
  (void)new TemplateTypeClass(LWAL0015);      //	TEMPLATE_LWAL0015
  (void)new TemplateTypeClass(LWAL0016);      //	TEMPLATE_LWAL0016
  (void)new TemplateTypeClass(LWAL0017);      //	TEMPLATE_LWAL0017
  (void)new TemplateTypeClass(LWAL0018);      //	TEMPLATE_LWAL0018
  (void)new TemplateTypeClass(LWAL0019);      //	TEMPLATE_LWAL0019
  (void)new TemplateTypeClass(LWAL0020);      //	TEMPLATE_LWAL0020
  (void)new TemplateTypeClass(LWAL0021);      //	TEMPLATE_LWAL0021
  (void)new TemplateTypeClass(LWAL0022);      //	TEMPLATE_LWAL0022
  (void)new TemplateTypeClass(LWAL0023);      //	TEMPLATE_LWAL0023
  (void)new TemplateTypeClass(LWAL0024);      //	TEMPLATE_LWAL0024
  (void)new TemplateTypeClass(LWAL0025);      //	TEMPLATE_LWAL0025
  (void)new TemplateTypeClass(LWAL0026);      //	TEMPLATE_LWAL0026
  (void)new TemplateTypeClass(LWAL0027);      //	TEMPLATE_LWAL0027
  (void)new TemplateTypeClass(STRP0001);      //	TEMPLATE_STRP0001
  (void)new TemplateTypeClass(STRP0002);      //	TEMPLATE_STRP0002
  (void)new TemplateTypeClass(STRP0003);      //	TEMPLATE_STRP0003
  (void)new TemplateTypeClass(STRP0004);      //	TEMPLATE_STRP0004
  (void)new TemplateTypeClass(STRP0005);      //	TEMPLATE_STRP0005
  (void)new TemplateTypeClass(STRP0006);      //	TEMPLATE_STRP0006
  (void)new TemplateTypeClass(STRP0007);      //	TEMPLATE_STRP0007
  (void)new TemplateTypeClass(STRP0008);      //	TEMPLATE_STRP0008
  (void)new TemplateTypeClass(STRP0009);      //	TEMPLATE_STRP0009
  (void)new TemplateTypeClass(STRP0010);      //	TEMPLATE_STRP0010
  (void)new TemplateTypeClass(STRP0011);      //	TEMPLATE_STRP0011
  (void)new TemplateTypeClass(WALL0001);      //	TEMPLATE_WALL0001
  (void)new TemplateTypeClass(WALL0002);      //	TEMPLATE_WALL0002
  (void)new TemplateTypeClass(WALL0003);      //	TEMPLATE_WALL0003
  (void)new TemplateTypeClass(WALL0004);      //	TEMPLATE_WALL0004
  (void)new TemplateTypeClass(WALL0005);      //	TEMPLATE_WALL0005
  (void)new TemplateTypeClass(WALL0006);      //	TEMPLATE_WALL0006
  (void)new TemplateTypeClass(WALL0007);      //	TEMPLATE_WALL0007
  (void)new TemplateTypeClass(WALL0008);      //	TEMPLATE_WALL0008
  (void)new TemplateTypeClass(WALL0009);      //	TEMPLATE_WALL0009
  (void)new TemplateTypeClass(WALL0010);      //	TEMPLATE_WALL0010
  (void)new TemplateTypeClass(WALL0011);      //	TEMPLATE_WALL0011
  (void)new TemplateTypeClass(WALL0012);      //	TEMPLATE_WALL0012
  (void)new TemplateTypeClass(WALL0013);      //	TEMPLATE_WALL0013
  (void)new TemplateTypeClass(WALL0014);      //	TEMPLATE_WALL0014
  (void)new TemplateTypeClass(WALL0015);      //	TEMPLATE_WALL0015
  (void)new TemplateTypeClass(WALL0016);      //	TEMPLATE_WALL0016
  (void)new TemplateTypeClass(WALL0017);      //	TEMPLATE_WALL0017
  (void)new TemplateTypeClass(WALL0018);      //	TEMPLATE_WALL0018
  (void)new TemplateTypeClass(WALL0019);      //	TEMPLATE_WALL0019
  (void)new TemplateTypeClass(WALL0020);      //	TEMPLATE_WALL0020
  (void)new TemplateTypeClass(WALL0021);      //	TEMPLATE_WALL0021
  (void)new TemplateTypeClass(WALL0022);      //	TEMPLATE_WALL0022
  (void)new TemplateTypeClass(WALL0023);      //	TEMPLATE_WALL0023
  (void)new TemplateTypeClass(WALL0024);      //	TEMPLATE_WALL0024
  (void)new TemplateTypeClass(WALL0025);      //	TEMPLATE_WALL0025
  (void)new TemplateTypeClass(WALL0026);      //	TEMPLATE_WALL0026
  (void)new TemplateTypeClass(WALL0027);      //	TEMPLATE_WALL0027
  (void)new TemplateTypeClass(WALL0028);      //	TEMPLATE_WALL0028
  (void)new TemplateTypeClass(WALL0029);      //	TEMPLATE_WALL0029
  (void)new TemplateTypeClass(WALL0030);      //	TEMPLATE_WALL0030
  (void)new TemplateTypeClass(WALL0031);      //	TEMPLATE_WALL0031
  (void)new TemplateTypeClass(WALL0032);      //	TEMPLATE_WALL0032
  (void)new TemplateTypeClass(WALL0033);      //	TEMPLATE_WALL0033
  (void)new TemplateTypeClass(WALL0034);      //	TEMPLATE_WALL0034
  (void)new TemplateTypeClass(WALL0035);      //	TEMPLATE_WALL0035
  (void)new TemplateTypeClass(WALL0036);      //	TEMPLATE_WALL0036
  (void)new TemplateTypeClass(WALL0037);      //	TEMPLATE_WALL0037
  (void)new TemplateTypeClass(WALL0038);      //	TEMPLATE_WALL0038
  (void)new TemplateTypeClass(WALL0039);      //	TEMPLATE_WALL0039
  (void)new TemplateTypeClass(WALL0040);      //	TEMPLATE_WALL0040
  (void)new TemplateTypeClass(WALL0041);      //	TEMPLATE_WALL0041
  (void)new TemplateTypeClass(WALL0042);      //	TEMPLATE_WALL0042
  (void)new TemplateTypeClass(WALL0043);      //	TEMPLATE_WALL0043
  (void)new TemplateTypeClass(WALL0044);      //	TEMPLATE_WALL0044
  (void)new TemplateTypeClass(WALL0045);      //	TEMPLATE_WALL0045
  (void)new TemplateTypeClass(WALL0046);      //	TEMPLATE_WALL0046
  (void)new TemplateTypeClass(WALL0047);      //	TEMPLATE_WALL0047
  (void)new TemplateTypeClass(WALL0048);      //	TEMPLATE_WALL0048
  (void)new TemplateTypeClass(WALL0049);      //	TEMPLATE_WALL0049
  (void)new TemplateTypeClass(Bridge1h);      //	TEMPLATE_BRIDGE1H
  (void)new TemplateTypeClass(Bridge2h);      //	TEMPLATE_BRIDGE2H

  (void)new TemplateTypeClass(Bridge1ax);  //	TEMPLATE_BRIDGE_1AX
  (void)new TemplateTypeClass(Bridge2ax);  //	TEMPLATE_BRIDGE_2AX
  (void)new TemplateTypeClass(Bridge1x);   //	TEMPLATE_BRIDGE1X
  (void)new TemplateTypeClass(Bridge2x);   //	TEMPLATE_BRIDGE2X

  (void)new TemplateTypeClass(Xtra0001);  //	TEMPLATE_XTRA0001
  (void)new TemplateTypeClass(Xtra0002);  //	TEMPLATE_XTRA0002
  (void)new TemplateTypeClass(Xtra0003);  //	TEMPLATE_XTRA0003
  (void)new TemplateTypeClass(Xtra0004);  //	TEMPLATE_XTRA0004
  (void)new TemplateTypeClass(Xtra0005);  //	TEMPLATE_XTRA0005
  (void)new TemplateTypeClass(Xtra0006);  //	TEMPLATE_XTRA0006
  (void)new TemplateTypeClass(Xtra0007);  //	TEMPLATE_XTRA0007
  (void)new TemplateTypeClass(Xtra0008);  //	TEMPLATE_XTRA0008
  (void)new TemplateTypeClass(Xtra0009);  //	TEMPLATE_XTRA0009
  (void)new TemplateTypeClass(Xtra0010);  //	TEMPLATE_XTRA0010
  (void)new TemplateTypeClass(Xtra0011);  //	TEMPLATE_XTRA0011
  (void)new TemplateTypeClass(Xtra0012);  //	TEMPLATE_XTRA0012
  (void)new TemplateTypeClass(Xtra0013);  //	TEMPLATE_XTRA0013
  (void)new TemplateTypeClass(Xtra0014);  //	TEMPLATE_XTRA0014
  (void)new TemplateTypeClass(Xtra0015);  //	TEMPLATE_XTRA0015
  (void)new TemplateTypeClass(Xtra0016);  //	TEMPLATE_XTRA0016
  (void)new TemplateTypeClass(AntHill);   //	TEMPLATE_ROAD36
}

void TemplateTypeClass::Init_Heap() {
  /*
  **	These template type class objects must be allocated in the exact order
  *that they *	are specified in the TemplateType enumeration. This is necessary
  *because the heap *	allocation block index serves double duty as the type
  *number index.
  */
  (void)new TemplateTypeClass(Clear);         // TEMPLATE_CLEAR1
  (void)new TemplateTypeClass(Water);         // TEMPLATE_WATER
  (void)new TemplateTypeClass(Water2);        // TEMPLATE_WATER2
  (void)new TemplateTypeClass(Shore01);       // TEMPLATE_SHORE1
  (void)new TemplateTypeClass(Shore02);       // TEMPLATE_SHORE2
  (void)new TemplateTypeClass(Shore03);       // TEMPLATE_SHORE3
  (void)new TemplateTypeClass(Shore04);       // TEMPLATE_SHORE4
  (void)new TemplateTypeClass(Shore05);       // TEMPLATE_SHORE5
  (void)new TemplateTypeClass(Shore06);       // TEMPLATE_SHORE6
  (void)new TemplateTypeClass(Shore07);       // TEMPLATE_SHORE7
  (void)new TemplateTypeClass(Shore08);       // TEMPLATE_SHORE8
  (void)new TemplateTypeClass(Shore09);       // TEMPLATE_SHORE9
  (void)new TemplateTypeClass(Shore10);       // TEMPLATE_SHORE10
  (void)new TemplateTypeClass(Shore11);       //	TEMPLATE_SHORE11
  (void)new TemplateTypeClass(Shore12);       // TEMPLATE_SHORE12
  (void)new TemplateTypeClass(Shore13);       // TEMPLATE_SHORE13
  (void)new TemplateTypeClass(Shore14);       // TEMPLATE_SHORE14
  (void)new TemplateTypeClass(Shore15);       // TEMPLATE_SHORE15
  (void)new TemplateTypeClass(Shore16);       //	TEMPLATE_SHORE16
  (void)new TemplateTypeClass(Shore17);       //	TEMPLATE_SHORE17
  (void)new TemplateTypeClass(Shore18);       //	TEMPLATE_SHORE18
  (void)new TemplateTypeClass(Shore19);       // TEMPLATE_SHORE19
  (void)new TemplateTypeClass(Shore20);       // TEMPLATE_SHORE20
  (void)new TemplateTypeClass(Shore21);       // TEMPLATE_SHORE21
  (void)new TemplateTypeClass(Shore22);       //	TEMPLATE_SHORE22
  (void)new TemplateTypeClass(Shore23);       // TEMPLATE_SHORE23
  (void)new TemplateTypeClass(Shore24);       //	TEMPLATE_SHORE24
  (void)new TemplateTypeClass(Shore25);       //	TEMPLATE_SHORE25
  (void)new TemplateTypeClass(Shore26);       //	TEMPLATE_SHORE26
  (void)new TemplateTypeClass(Shore27);       //	TEMPLATE_SHORE27
  (void)new TemplateTypeClass(Shore28);       //	TEMPLATE_SHORE28
  (void)new TemplateTypeClass(Shore29);       //	TEMPLATE_SHORE29
  (void)new TemplateTypeClass(Shore30);       //	TEMPLATE_SHORE30
  (void)new TemplateTypeClass(Shore31);       //	TEMPLATE_SHORE31
  (void)new TemplateTypeClass(Shore32);       // TEMPLATE_SHORE32
  (void)new TemplateTypeClass(Shore33);       // TEMPLATE_SHORE33
  (void)new TemplateTypeClass(Shore34);       //	TEMPLATE_SHORE34
  (void)new TemplateTypeClass(Shore35);       //	TEMPLATE_SHORE35
  (void)new TemplateTypeClass(Shore36);       //	TEMPLATE_SHORE36
  (void)new TemplateTypeClass(Shore37);       //	TEMPLATE_SHORE37
  (void)new TemplateTypeClass(Shore38);       //	TEMPLATE_SHORE38
  (void)new TemplateTypeClass(Shore39);       //	TEMPLATE_SHORE39
  (void)new TemplateTypeClass(Shore40);       //	TEMPLATE_SHORE40
  (void)new TemplateTypeClass(Shore41);       //	TEMPLATE_SHORE41
  (void)new TemplateTypeClass(Shore42);       //	TEMPLATE_SHORE42
  (void)new TemplateTypeClass(Shore43);       //	TEMPLATE_SHORE43
  (void)new TemplateTypeClass(Shore44);       //	TEMPLATE_SHORE44
  (void)new TemplateTypeClass(Shore45);       //	TEMPLATE_SHORE45
  (void)new TemplateTypeClass(Shore46);       //	TEMPLATE_SHORE46
  (void)new TemplateTypeClass(Shore47);       //	TEMPLATE_SHORE47
  (void)new TemplateTypeClass(Shore48);       //	TEMPLATE_SHORE48
  (void)new TemplateTypeClass(Shore49);       //	TEMPLATE_SHORE49
  (void)new TemplateTypeClass(Shore50);       //	TEMPLATE_SHORE50
  (void)new TemplateTypeClass(Shore51);       //	TEMPLATE_SHORE51
  (void)new TemplateTypeClass(Shore52);       //	TEMPLATE_SHORE52
  (void)new TemplateTypeClass(Shore53);       //	TEMPLATE_SHORE53
  (void)new TemplateTypeClass(Shore54);       //	TEMPLATE_SHORE54
  (void)new TemplateTypeClass(Shore55);       //	TEMPLATE_SHORE55
  (void)new TemplateTypeClass(Shore56);       //	TEMPLATE_SHORE56
  (void)new TemplateTypeClass(ShoreCliff01);  //	TEMPLATE_SHORECLIFF01
  (void)new TemplateTypeClass(ShoreCliff02);  //	TEMPLATE_SHORECLIFF02
  (void)new TemplateTypeClass(ShoreCliff03);  //	TEMPLATE_SHORECLIFF03
  (void)new TemplateTypeClass(ShoreCliff04);  //	TEMPLATE_SHORECLIFF04
  (void)new TemplateTypeClass(ShoreCliff05);  //	TEMPLATE_SHORECLIFF05
  (void)new TemplateTypeClass(ShoreCliff06);  //	TEMPLATE_SHORECLIFF06
  (void)new TemplateTypeClass(ShoreCliff07);  //	TEMPLATE_SHORECLIFF07
  (void)new TemplateTypeClass(ShoreCliff08);  //	TEMPLATE_SHORECLIFF08
  (void)new TemplateTypeClass(ShoreCliff09);  //	TEMPLATE_SHORECLIFF09
  (void)new TemplateTypeClass(ShoreCliff10);  //	TEMPLATE_SHORECLIFF10
  (void)new TemplateTypeClass(ShoreCliff11);  //	TEMPLATE_SHORECLIFF11
  (void)new TemplateTypeClass(ShoreCliff12);  //	TEMPLATE_SHORECLIFF12
  (void)new TemplateTypeClass(ShoreCliff13);  //	TEMPLATE_SHORECLIFF13
  (void)new TemplateTypeClass(ShoreCliff14);  //	TEMPLATE_SHORECLIFF14
  (void)new TemplateTypeClass(ShoreCliff15);  //	TEMPLATE_SHORECLIFF15
  (void)new TemplateTypeClass(ShoreCliff16);  //	TEMPLATE_SHORECLIFF16
  (void)new TemplateTypeClass(ShoreCliff17);  //	TEMPLATE_SHORECLIFF17
  (void)new TemplateTypeClass(ShoreCliff18);  //	TEMPLATE_SHORECLIFF18
  (void)new TemplateTypeClass(ShoreCliff19);  //	TEMPLATE_SHORECLIFF19
  (void)new TemplateTypeClass(ShoreCliff20);  //	TEMPLATE_SHORECLIFF20
  (void)new TemplateTypeClass(ShoreCliff21);  //	TEMPLATE_SHORECLIFF21
  (void)new TemplateTypeClass(ShoreCliff22);  //	TEMPLATE_SHORECLIFF22
  (void)new TemplateTypeClass(ShoreCliff23);  //	TEMPLATE_SHORECLIFF23
  (void)new TemplateTypeClass(ShoreCliff24);  //	TEMPLATE_SHORECLIFF24
  (void)new TemplateTypeClass(ShoreCliff25);  //	TEMPLATE_SHORECLIFF25
  (void)new TemplateTypeClass(ShoreCliff26);  //	TEMPLATE_SHORECLIFF26
  (void)new TemplateTypeClass(ShoreCliff27);  //	TEMPLATE_SHORECLIFF27
  (void)new TemplateTypeClass(ShoreCliff28);  //	TEMPLATE_SHORECLIFF28
  (void)new TemplateTypeClass(ShoreCliff29);  //	TEMPLATE_SHORECLIFF29
  (void)new TemplateTypeClass(ShoreCliff30);  //	TEMPLATE_SHORECLIFF30
  (void)new TemplateTypeClass(ShoreCliff31);  //	TEMPLATE_SHORECLIFF31
  (void)new TemplateTypeClass(ShoreCliff32);  //	TEMPLATE_SHORECLIFF32
  (void)new TemplateTypeClass(ShoreCliff33);  //	TEMPLATE_SHORECLIFF33
  (void)new TemplateTypeClass(ShoreCliff34);  //	TEMPLATE_SHORECLIFF34
  (void)new TemplateTypeClass(ShoreCliff35);  //	TEMPLATE_SHORECLIFF35
  (void)new TemplateTypeClass(ShoreCliff36);  //	TEMPLATE_SHORECLIFF36
  (void)new TemplateTypeClass(ShoreCliff37);  //	TEMPLATE_SHORECLIFF37
  (void)new TemplateTypeClass(ShoreCliff38);  //	TEMPLATE_SHORECLIFF38
  (void)new TemplateTypeClass(Boulder1);      //	TEMPLATE_BOULDER1
  (void)new TemplateTypeClass(Boulder2);      //	TEMPLATE_BOULDER2
  (void)new TemplateTypeClass(Boulder3);      //	TEMPLATE_BOULDER3
  (void)new TemplateTypeClass(Boulder4);      // TEMPLATE_BOULDER4
  (void)new TemplateTypeClass(Boulder5);      //	TEMPLATE_BOULDER5
  (void)new TemplateTypeClass(Boulder6);      //	TEMPLATE_BOULDER6
  (void)new TemplateTypeClass(Patch01);       //	TEMPLATE_PATCH1
  (void)new TemplateTypeClass(Patch02);       //	TEMPLATE_PATCH2
  (void)new TemplateTypeClass(Patch03);       //	TEMPLATE_PATCH3
  (void)new TemplateTypeClass(Patch04);       //	TEMPLATE_PATCH4
  (void)new TemplateTypeClass(Patch07);       //	TEMPLATE_PATCH7
  (void)new TemplateTypeClass(Patch08);       //	TEMPLATE_PATCH8
  (void)new TemplateTypeClass(Patch13);       //	TEMPLATE_PATCH13
  (void)new TemplateTypeClass(Patch14);       //	TEMPLATE_PATCH14
  (void)new TemplateTypeClass(Patch15);       //	TEMPLATE_PATCH15
  (void)new TemplateTypeClass(River01);       //	TEMPLATE_RIVER1
  (void)new TemplateTypeClass(River02);       //	TEMPLATE_RIVER2
  (void)new TemplateTypeClass(River03);       //	TEMPLATE_RIVER3
  (void)new TemplateTypeClass(River04);       //	TEMPLATE_RIVER4
  (void)new TemplateTypeClass(River05);       //	TEMPLATE_RIVER5
  (void)new TemplateTypeClass(River06);       //	TEMPLATE_RIVER6
  (void)new TemplateTypeClass(River07);       //	TEMPLATE_RIVER7
  (void)new TemplateTypeClass(River08);       //	TEMPLATE_RIVER8
  (void)new TemplateTypeClass(River09);       //	TEMPLATE_RIVER9
  (void)new TemplateTypeClass(River10);       //	TEMPLATE_RIVER10
  (void)new TemplateTypeClass(River11);       //	TEMPLATE_RIVER11
  (void)new TemplateTypeClass(River12);       //	TEMPLATE_RIVER12
  (void)new TemplateTypeClass(River13);       //	TEMPLATE_RIVER13
  (void)new TemplateTypeClass(Falls1);        //	TEMPLATE_FALLS1
  (void)new TemplateTypeClass(Falls1a);       //	TEMPLATE_FALLS1A
  (void)new TemplateTypeClass(Falls2);        //	TEMPLATE_FALLS2
  (void)new TemplateTypeClass(Falls2a);       //	TEMPLATE_FALLS2A
  (void)new TemplateTypeClass(Ford1);         //	TEMPLATE_FORD1
  (void)new TemplateTypeClass(Ford2);         //	TEMPLATE_FORD2
  (void)new TemplateTypeClass(Bridge1);       //	TEMPLATE_BRIDGE1
  (void)new TemplateTypeClass(Bridge1d);      //	TEMPLATE_BRIDGE1D
  (void)new TemplateTypeClass(Bridge2);       //	TEMPLATE_BRIDGE2
  (void)new TemplateTypeClass(Bridge2d);      //	TEMPLATE_BRIDGE2D
  (void)new TemplateTypeClass(Slope01);       //	TEMPLATE_SLOPE1
  (void)new TemplateTypeClass(Slope02);       //	TEMPLATE_SLOPE2
  (void)new TemplateTypeClass(Slope03);       //	TEMPLATE_SLOPE3
  (void)new TemplateTypeClass(Slope04);       //	TEMPLATE_SLOPE4
  (void)new TemplateTypeClass(Slope05);       //	TEMPLATE_SLOPE5
  (void)new TemplateTypeClass(Slope06);       //	TEMPLATE_SLOPE6
  (void)new TemplateTypeClass(Slope07);       //	TEMPLATE_SLOPE7
  (void)new TemplateTypeClass(Slope08);       //	TEMPLATE_SLOPE8
  (void)new TemplateTypeClass(Slope09);       //	TEMPLATE_SLOPE9
  (void)new TemplateTypeClass(Slope10);       //	TEMPLATE_SLOPE10
  (void)new TemplateTypeClass(Slope11);       //	TEMPLATE_SLOPE11
  (void)new TemplateTypeClass(Slope12);       //	TEMPLATE_SLOPE12
  (void)new TemplateTypeClass(Slope13);       //	TEMPLATE_SLOPE13
  (void)new TemplateTypeClass(Slope14);       //	TEMPLATE_SLOPE14
  (void)new TemplateTypeClass(Slope15);       //	TEMPLATE_SLOPE15
  (void)new TemplateTypeClass(Slope16);       //	TEMPLATE_SLOPE16
  (void)new TemplateTypeClass(Slope17);       //	TEMPLATE_SLOPE17
  (void)new TemplateTypeClass(Slope18);       //	TEMPLATE_SLOPE18
  (void)new TemplateTypeClass(Slope19);       //	TEMPLATE_SLOPE19
  (void)new TemplateTypeClass(Slope20);       //	TEMPLATE_SLOPE20
  (void)new TemplateTypeClass(Slope21);       //	TEMPLATE_SLOPE21
  (void)new TemplateTypeClass(Slope22);       //	TEMPLATE_SLOPE22
  (void)new TemplateTypeClass(Slope23);       //	TEMPLATE_SLOPE23
  (void)new TemplateTypeClass(Slope24);       //	TEMPLATE_SLOPE24
  (void)new TemplateTypeClass(Slope25);       //	TEMPLATE_SLOPE25
  (void)new TemplateTypeClass(Slope26);       //	TEMPLATE_SLOPE26
  (void)new TemplateTypeClass(Slope27);       //	TEMPLATE_SLOPE27
  (void)new TemplateTypeClass(Slope28);       //	TEMPLATE_SLOPE28
  (void)new TemplateTypeClass(Slope29);       //	TEMPLATE_SLOPE29
  (void)new TemplateTypeClass(Slope30);       //	TEMPLATE_SLOPE30
  (void)new TemplateTypeClass(Slope31);       //	TEMPLATE_SLOPE31
  (void)new TemplateTypeClass(Slope32);       //	TEMPLATE_SLOPE32
  (void)new TemplateTypeClass(Slope33);       //	TEMPLATE_SLOPE33
  (void)new TemplateTypeClass(Slope34);       //	TEMPLATE_SLOPE34
  (void)new TemplateTypeClass(Slope35);       //	TEMPLATE_SLOPE35
  (void)new TemplateTypeClass(Slope36);       //	TEMPLATE_SLOPE36
  (void)new TemplateTypeClass(Slope37);       //	TEMPLATE_SLOPE37
  (void)new TemplateTypeClass(Slope38);       //	TEMPLATE_SLOPE38
  (void)new TemplateTypeClass(Road01);        //	TEMPLATE_ROAD1
  (void)new TemplateTypeClass(Road02);        //	TEMPLATE_ROAD2
  (void)new TemplateTypeClass(Road03);        //	TEMPLATE_ROAD3
  (void)new TemplateTypeClass(Road04);        //	TEMPLATE_ROAD4
  (void)new TemplateTypeClass(Road05);        //	TEMPLATE_ROAD5
  (void)new TemplateTypeClass(Road06);        //	TEMPLATE_ROAD6
  (void)new TemplateTypeClass(Road07);        //	TEMPLATE_ROAD7
  (void)new TemplateTypeClass(Road08);        //	TEMPLATE_ROAD8
  (void)new TemplateTypeClass(Road09);        //	TEMPLATE_ROAD9
  (void)new TemplateTypeClass(Road10);        //	TEMPLATE_ROAD10
  (void)new TemplateTypeClass(Road11);        //	TEMPLATE_ROAD11
  (void)new TemplateTypeClass(Road12);        //	TEMPLATE_ROAD12
  (void)new TemplateTypeClass(Road13);        //	TEMPLATE_ROAD13
  (void)new TemplateTypeClass(Road14);        //	TEMPLATE_ROAD14
  (void)new TemplateTypeClass(Road15);        //	TEMPLATE_ROAD15
  (void)new TemplateTypeClass(Road16);        //	TEMPLATE_ROAD16
  (void)new TemplateTypeClass(Road17);        //	TEMPLATE_ROAD17
  (void)new TemplateTypeClass(Road18);        //	TEMPLATE_ROAD18
  (void)new TemplateTypeClass(Road19);        //	TEMPLATE_ROAD19
  (void)new TemplateTypeClass(Road20);        //	TEMPLATE_ROAD20
  (void)new TemplateTypeClass(Road21);        //	TEMPLATE_ROAD21
  (void)new TemplateTypeClass(Road22);        //	TEMPLATE_ROAD22
  (void)new TemplateTypeClass(Road23);        //	TEMPLATE_ROAD23
  (void)new TemplateTypeClass(Road24);        //	TEMPLATE_ROAD24
  (void)new TemplateTypeClass(Road25);        //	TEMPLATE_ROAD25
  (void)new TemplateTypeClass(Road26);        //	TEMPLATE_ROAD26
  (void)new TemplateTypeClass(Road27);        //	TEMPLATE_ROAD27
  (void)new TemplateTypeClass(Road28);        //	TEMPLATE_ROAD28
  (void)new TemplateTypeClass(Road29);        //	TEMPLATE_ROAD29
  (void)new TemplateTypeClass(Road30);        //	TEMPLATE_ROAD30
  (void)new TemplateTypeClass(Road31);        //	TEMPLATE_ROAD31
  (void)new TemplateTypeClass(Road32);        //	TEMPLATE_ROAD32
  (void)new TemplateTypeClass(Road33);        //	TEMPLATE_ROAD33
  (void)new TemplateTypeClass(Road34);        //	TEMPLATE_ROAD34
  (void)new TemplateTypeClass(Road35);        //	TEMPLATE_ROAD35
  (void)new TemplateTypeClass(Road36);        //	TEMPLATE_ROAD36

  /*
  **	Separate out the list of new operator calls. Watcom bombs
  **	if they are kept together.
  */
  _Watcom_Ugh_Hack();
}

/***********************************************************************************************
 * TemplateTypeClass::Land_Type -- Determines land type from template and icon
 *number.         *
 *                                                                                             *
 *    This routine will convert the specified icon number into the appropriate
 *land type. The  * land type can be determined from the embedded colors in the
 *"control template" section   * of the original art file. This control
 *information is encoded into the icon data file    * to be retrieved and
 *interpreted as the program sees fit. The engine only recognizes      * the
 *first 16 colors as control colors, so the control map color value serves as an
 ** index into a simple lookup table. *
 *                                                                                             *
 * INPUT:   icon  -- The icon number within this template that is to be examined
 *and used      * to determine the land type. *
 *                                                                                             *
 * OUTPUT:  Returns with the land type that corresponds to the icon number
 *specified.          *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/12/1995 JLB : Created. *
 *=============================================================================================*/
LandType TemplateTypeClass::Land_Type(int icon) const {
  const IconsetClass* icontrol =
      static_cast<const IconsetClass*>(Get_Image_Data());

  if (icontrol != nullptr) {
    const unsigned char* map = icontrol->Control_Map();
    if (map != nullptr) {
      static LandType _land[16] = {
          LAND_CLEAR, LAND_CLEAR, LAND_CLEAR,
          LAND_CLEAR,  // Clear
          LAND_CLEAR, LAND_CLEAR,
          LAND_BEACH,  // Beach
          LAND_CLEAR,
          LAND_ROCK,   // Rock
          LAND_ROAD,   // Road
          LAND_WATER,  // Water
          LAND_RIVER,  //	River
          LAND_CLEAR, LAND_CLEAR,
          LAND_ROUGH,  // Rough
          LAND_CLEAR,
      };

      return _land[map[icon %
                       (icontrol->Map_Width() * icontrol->Map_Height())]];
    }
  }
  return LAND_CLEAR;
}

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
  if (name != nullptr) {
    for (TemplateType index : magic_enum::enum_values<TemplateType>()) {
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
 * HISTORY: * 05/23/1994 JLB : Created. * 12/12/1995 JLB : Optimized for direct
 *access to iconset data.                             *
 *=============================================================================================*/
const short* TemplateTypeClass::Occupy_List(bool) const {
  static short _occupy[13 * 8 + 5];
  short* ptr;

  const IconsetClass* iconset =
      static_cast<const IconsetClass*>(Get_Image_Data());
  const unsigned char* map = iconset->Map_Data();

  ptr = &_occupy[0];
  for (int index = 0; index < Width * Height; index++) {
    if (*map++ != 0xFF) {
      *ptr++ = static_cast<short>(index % Width + index / Width * MAP_CELL_W);
    }
  }
  *ptr = kRefreshEol;

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
  for (TemplateType index : magic_enum::enum_values<TemplateType>()) {
    TemplateTypeClass& tplate = As_Reference(index);

    tplate.ClearImage();
    if (tplate.Theater & 1 << theater) {
      auto fullname = std::filesystem::path(tplate.IniName)
                          .replace_extension(Theaters[theater].Suffix)
                          .string();

      // Working loaded iconset pointer.
      auto data = MFCD::RetrieveData(fullname);
      tplate.SetBorrowedImage(data);
      const void* ptr = data.data();

      // Register icon set for video memory caching
      Register_Icon_Set(ptr, true);

      tplate.Width = Get_IconSet_MapWidth(ptr);
      tplate.Height = Get_IconSet_MapHeight(ptr);
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
                                HousesType) const {
  int w, h;
  int index;
  bool scale;  // Should the template be half sized?

  w = Bound(static_cast<int>(Width), 1, 13);
  h = Bound(static_cast<int>(Height), 1, 8);
  scale = (w > 3 || h > 3);
  if (scale) {
    x -= (w * ICON_PIXEL_W) / 4;
    y -= (h * ICON_PIXEL_H) / 4;
  } else {
    x -= (w * ICON_PIXEL_W) / 2;
    y -= (h * ICON_PIXEL_H) / 2;
  }
  x += WindowList[window][WINDOWX];
  y += WindowList[window][WINDOWY];

  const IconsetClass* iconset = (const IconsetClass*)Get_Image_Data();
  const unsigned char* map = iconset->Map_Data();

  for (index = 0; index < w * h; index++) {
    if (map[index] != 0xFF) {
      HidPage.Draw_Stamp(iconset, index, 0, 0, nullptr, WINDOW_MAIN);
      if (scale) {
        HidPage.Scale(
            (*LogicPage), 0, 0, x + ((index % w) * (ICON_PIXEL_W / 2)),
            y + ((index / w) * (ICON_PIXEL_H / 2)), ICON_PIXEL_W, ICON_PIXEL_H,
            ICON_PIXEL_W / 2, ICON_PIXEL_H / 2, (char*)nullptr);

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
  for (TemplateType index : magic_enum::enum_values<TemplateType>()) {
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
bool TemplateTypeClass::Create_And_Place(CELL cell, HousesType) const {
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
ObjectClass* TemplateTypeClass::Create_One_Of(HouseClass*) const {
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

/***********************************************************************************************
 * TemplateTypeClass::As_Reference -- Fetches a reference to the template
 *specified.           *
 *                                                                                             *
 *    This will return a reference to the TemplateTypeClass requested. *
 *                                                                                             *
 * INPUT:   type  -- The template type to fetch a reference to. *
 *                                                                                             *
 * OUTPUT:  Returns with a reference to the template type class specified. *
 *                                                                                             *
 * WARNINGS:   Be sure to pass a valid type parameter. This routine doesn't
 *check it for       * legality. *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
TemplateTypeClass& TemplateTypeClass::As_Reference(TemplateType type) {
  return *TemplateTypes.Ptr(type);
}

COORDINATE TemplateTypeClass::Coord_Fixup(COORDINATE coord) const {
  return Coord_Whole(coord);
}
