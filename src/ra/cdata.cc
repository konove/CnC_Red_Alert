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
                                     THEATERF_TEMPERATE | THEATERF_SNOW |
                                         THEATERF_INTERIOR,
                                     "CLEAR1", TXT_CLEAR);
static const TemplateTypeClass Clear(TEMPLATE_CLEAR1,
                                     THEATERF_TEMPERATE | THEATERF_SNOW |
                                         THEATERF_INTERIOR,
                                     "CLEAR1", TXT_CLEAR);
static const TemplateTypeClass Road01(TEMPLATE_ROAD01,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D01",
                                      TXT_ROAD);
static const TemplateTypeClass Road02(TEMPLATE_ROAD02,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D02",
                                      TXT_ROAD);
static const TemplateTypeClass Road03(TEMPLATE_ROAD03,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D03",
                                      TXT_ROAD);
static const TemplateTypeClass Road04(TEMPLATE_ROAD04,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D04",
                                      TXT_ROAD);
static const TemplateTypeClass Road05(TEMPLATE_ROAD05,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D05",
                                      TXT_ROAD);
static const TemplateTypeClass Road06(TEMPLATE_ROAD06,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D06",
                                      TXT_ROAD);
static const TemplateTypeClass Road07(TEMPLATE_ROAD07,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D07",
                                      TXT_ROAD);
static const TemplateTypeClass Road08(TEMPLATE_ROAD08,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D08",
                                      TXT_ROAD);
static const TemplateTypeClass Road09(TEMPLATE_ROAD09,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D09",
                                      TXT_ROAD);
static const TemplateTypeClass Road10(TEMPLATE_ROAD10,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D10",
                                      TXT_ROAD);
static const TemplateTypeClass Road11(TEMPLATE_ROAD11,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D11",
                                      TXT_ROAD);
static const TemplateTypeClass Road12(TEMPLATE_ROAD12,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D12",
                                      TXT_ROAD);
static const TemplateTypeClass Road13(TEMPLATE_ROAD13,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D13",
                                      TXT_ROAD);
static const TemplateTypeClass Road14(TEMPLATE_ROAD14,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D14",
                                      TXT_ROAD);
static const TemplateTypeClass Road15(TEMPLATE_ROAD15,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D15",
                                      TXT_ROAD);
static const TemplateTypeClass Road16(TEMPLATE_ROAD16,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D16",
                                      TXT_ROAD);
static const TemplateTypeClass Road17(TEMPLATE_ROAD17,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D17",
                                      TXT_ROAD);
static const TemplateTypeClass Road18(TEMPLATE_ROAD18,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D18",
                                      TXT_ROAD);
static const TemplateTypeClass Road19(TEMPLATE_ROAD19,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D19",
                                      TXT_ROAD);
static const TemplateTypeClass Road20(TEMPLATE_ROAD20,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D20",
                                      TXT_ROAD);
static const TemplateTypeClass Road21(TEMPLATE_ROAD21,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D21",
                                      TXT_ROAD);
static const TemplateTypeClass Road22(TEMPLATE_ROAD22,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D22",
                                      TXT_ROAD);
static const TemplateTypeClass Road23(TEMPLATE_ROAD23,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D23",
                                      TXT_ROAD);
static const TemplateTypeClass Road24(TEMPLATE_ROAD24,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D24",
                                      TXT_ROAD);
static const TemplateTypeClass Road25(TEMPLATE_ROAD25,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D25",
                                      TXT_ROAD);
static const TemplateTypeClass Road26(TEMPLATE_ROAD26,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D26",
                                      TXT_ROAD);
static const TemplateTypeClass Road27(TEMPLATE_ROAD27,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D27",
                                      TXT_ROAD);
static const TemplateTypeClass Road28(TEMPLATE_ROAD28,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D28",
                                      TXT_ROAD);
static const TemplateTypeClass Road29(TEMPLATE_ROAD29,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D29",
                                      TXT_ROAD);
static const TemplateTypeClass Road30(TEMPLATE_ROAD30,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D30",
                                      TXT_ROAD);
static const TemplateTypeClass Road31(TEMPLATE_ROAD31,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D31",
                                      TXT_ROAD);
static const TemplateTypeClass Road32(TEMPLATE_ROAD32,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D32",
                                      TXT_ROAD);
static const TemplateTypeClass Road33(TEMPLATE_ROAD33,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D33",
                                      TXT_ROAD);
static const TemplateTypeClass Road34(TEMPLATE_ROAD34,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D34",
                                      TXT_ROAD);
static const TemplateTypeClass Road35(TEMPLATE_ROAD35,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D35",
                                      TXT_ROAD);
static const TemplateTypeClass Road36(TEMPLATE_ROAD36,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D36",
                                      TXT_ROAD);
static const TemplateTypeClass Road37(TEMPLATE_ROAD37,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D37",
                                      TXT_ROAD);
static const TemplateTypeClass Road38(TEMPLATE_ROAD38,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D38",
                                      TXT_ROAD);
static const TemplateTypeClass Road39(TEMPLATE_ROAD39,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D39",
                                      TXT_ROAD);
static const TemplateTypeClass Road40(TEMPLATE_ROAD40,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D40",
                                      TXT_ROAD);
static const TemplateTypeClass Road41(TEMPLATE_ROAD41,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D41",
                                      TXT_ROAD);
static const TemplateTypeClass Road42(TEMPLATE_ROAD42,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D42",
                                      TXT_ROAD);
static const TemplateTypeClass Road43(TEMPLATE_ROAD43,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D43",
                                      TXT_ROAD);
static const TemplateTypeClass Road44(TEMPLATE_ROAD44,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D44",
                                      TXT_ROAD);
static const TemplateTypeClass Road45(TEMPLATE_ROAD45,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "D45",
                                      TXT_ROAD);
static const TemplateTypeClass Water(TEMPLATE_WATER,
                                     THEATERF_TEMPERATE | THEATERF_SNOW, "W1",
                                     TXT_WATER);
static const TemplateTypeClass Water2(TEMPLATE_WATER2,
                                      THEATERF_TEMPERATE | THEATERF_SNOW, "W2",
                                      TXT_WATER);
static const TemplateTypeClass Shore01(TEMPLATE_SHORE01,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH01", TXT_SHORE);
static const TemplateTypeClass Shore02(TEMPLATE_SHORE02,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH02", TXT_SHORE);
static const TemplateTypeClass Shore03(TEMPLATE_SHORE03,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH03", TXT_SHORE);
static const TemplateTypeClass Shore04(TEMPLATE_SHORE04,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH04", TXT_SHORE);
static const TemplateTypeClass Shore05(TEMPLATE_SHORE05,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH05", TXT_SHORE);
static const TemplateTypeClass Shore06(TEMPLATE_SHORE06,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH06", TXT_SHORE);
static const TemplateTypeClass Shore07(TEMPLATE_SHORE07,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH07", TXT_SHORE);
static const TemplateTypeClass Shore08(TEMPLATE_SHORE08,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH08", TXT_SHORE);
static const TemplateTypeClass Shore09(TEMPLATE_SHORE09,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH09", TXT_SHORE);
static const TemplateTypeClass Shore10(TEMPLATE_SHORE10,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH10", TXT_SHORE);
static const TemplateTypeClass Shore11(TEMPLATE_SHORE11,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH11", TXT_SHORE);
static const TemplateTypeClass Shore12(TEMPLATE_SHORE12,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH12", TXT_SHORE);
static const TemplateTypeClass Shore13(TEMPLATE_SHORE13,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH13", TXT_SHORE);
static const TemplateTypeClass Shore14(TEMPLATE_SHORE14,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH14", TXT_SHORE);
static const TemplateTypeClass Shore15(TEMPLATE_SHORE15,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH15", TXT_SHORE);
static const TemplateTypeClass Shore16(TEMPLATE_SHORE16,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH16", TXT_SHORE);
static const TemplateTypeClass Shore17(TEMPLATE_SHORE17,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH17", TXT_SHORE);
static const TemplateTypeClass Shore18(TEMPLATE_SHORE18,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH18", TXT_SHORE);
static const TemplateTypeClass Shore19(TEMPLATE_SHORE19,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH19", TXT_SHORE);
static const TemplateTypeClass Shore20(TEMPLATE_SHORE20,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH20", TXT_SHORE);
static const TemplateTypeClass Shore21(TEMPLATE_SHORE21,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH21", TXT_SHORE);
static const TemplateTypeClass Shore22(TEMPLATE_SHORE22,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH22", TXT_SHORE);
static const TemplateTypeClass Shore23(TEMPLATE_SHORE23,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH23", TXT_SHORE);
static const TemplateTypeClass Shore24(TEMPLATE_SHORE24,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH24", TXT_SHORE);
static const TemplateTypeClass Shore25(TEMPLATE_SHORE25,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH25", TXT_SHORE);
static const TemplateTypeClass Shore26(TEMPLATE_SHORE26,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH26", TXT_SHORE);
static const TemplateTypeClass Shore27(TEMPLATE_SHORE27,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH27", TXT_SHORE);
static const TemplateTypeClass Shore28(TEMPLATE_SHORE28,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH28", TXT_SHORE);
static const TemplateTypeClass Shore29(TEMPLATE_SHORE29,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH29", TXT_SHORE);
static const TemplateTypeClass Shore30(TEMPLATE_SHORE30,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH30", TXT_SHORE);
static const TemplateTypeClass Shore31(TEMPLATE_SHORE31,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH31", TXT_SHORE);
static const TemplateTypeClass Shore32(TEMPLATE_SHORE32,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH32", TXT_SHORE);
static const TemplateTypeClass Shore33(TEMPLATE_SHORE33,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH33", TXT_SHORE);
static const TemplateTypeClass Shore34(TEMPLATE_SHORE34,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH34", TXT_SHORE);
static const TemplateTypeClass Shore35(TEMPLATE_SHORE35,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH35", TXT_SHORE);
static const TemplateTypeClass Shore36(TEMPLATE_SHORE36,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH36", TXT_SHORE);
static const TemplateTypeClass Shore37(TEMPLATE_SHORE37,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH37", TXT_SHORE);
static const TemplateTypeClass Shore38(TEMPLATE_SHORE38,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH38", TXT_SHORE);
static const TemplateTypeClass Shore39(TEMPLATE_SHORE39,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH39", TXT_SHORE);
static const TemplateTypeClass Shore40(TEMPLATE_SHORE40,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH40", TXT_SHORE);
static const TemplateTypeClass Shore41(TEMPLATE_SHORE41,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH41", TXT_SHORE);
static const TemplateTypeClass Shore42(TEMPLATE_SHORE42,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH42", TXT_SHORE);
static const TemplateTypeClass Shore43(TEMPLATE_SHORE43,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH43", TXT_SHORE);
static const TemplateTypeClass Shore44(TEMPLATE_SHORE44,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH44", TXT_SHORE);
static const TemplateTypeClass Shore45(TEMPLATE_SHORE45,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH45", TXT_SHORE);
static const TemplateTypeClass Shore46(TEMPLATE_SHORE46,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH46", TXT_SHORE);
static const TemplateTypeClass Shore47(TEMPLATE_SHORE47,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH47", TXT_SHORE);
static const TemplateTypeClass Shore48(TEMPLATE_SHORE48,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH48", TXT_SHORE);
static const TemplateTypeClass Shore49(TEMPLATE_SHORE49,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH49", TXT_SHORE);
static const TemplateTypeClass Shore50(TEMPLATE_SHORE50,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH50", TXT_SHORE);
static const TemplateTypeClass Shore51(TEMPLATE_SHORE51,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH51", TXT_SHORE);
static const TemplateTypeClass Shore52(TEMPLATE_SHORE52,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH52", TXT_SHORE);
static const TemplateTypeClass Shore53(TEMPLATE_SHORE53,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH53", TXT_SHORE);
static const TemplateTypeClass Shore54(TEMPLATE_SHORE54,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH54", TXT_SHORE);
static const TemplateTypeClass Shore55(TEMPLATE_SHORE55,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH55", TXT_SHORE);
static const TemplateTypeClass Shore56(TEMPLATE_SHORE56,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "SH56", TXT_SHORE);
static const TemplateTypeClass Boulder1(TEMPLATE_BOULDER1,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "B1", TXT_SLOPE);
static const TemplateTypeClass Boulder2(TEMPLATE_BOULDER2,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "B2", TXT_SLOPE);
static const TemplateTypeClass Boulder3(TEMPLATE_BOULDER3,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "B3", TXT_SLOPE);
static const TemplateTypeClass Boulder4(TEMPLATE_BOULDER4,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "B4", TXT_SLOPE);
static const TemplateTypeClass Boulder5(TEMPLATE_BOULDER5,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "B5", TXT_SLOPE);
static const TemplateTypeClass Boulder6(TEMPLATE_BOULDER6,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "B6", TXT_SLOPE);
static const TemplateTypeClass Slope01(TEMPLATE_SLOPE01,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S01", TXT_SLOPE);
static const TemplateTypeClass Slope02(TEMPLATE_SLOPE02,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S02", TXT_SLOPE);
static const TemplateTypeClass Slope03(TEMPLATE_SLOPE03,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S03", TXT_SLOPE);
static const TemplateTypeClass Slope04(TEMPLATE_SLOPE04,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S04", TXT_SLOPE);
static const TemplateTypeClass Slope05(TEMPLATE_SLOPE05,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S05", TXT_SLOPE);
static const TemplateTypeClass Slope06(TEMPLATE_SLOPE06,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S06", TXT_SLOPE);
static const TemplateTypeClass Slope07(TEMPLATE_SLOPE07,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S07", TXT_SLOPE);
static const TemplateTypeClass Slope08(TEMPLATE_SLOPE08,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S08", TXT_SLOPE);
static const TemplateTypeClass Slope09(TEMPLATE_SLOPE09,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S09", TXT_SLOPE);
static const TemplateTypeClass Slope10(TEMPLATE_SLOPE10,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S10", TXT_SLOPE);
static const TemplateTypeClass Slope11(TEMPLATE_SLOPE11,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S11", TXT_SLOPE);
static const TemplateTypeClass Slope12(TEMPLATE_SLOPE12,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S12", TXT_SLOPE);
static const TemplateTypeClass Slope13(TEMPLATE_SLOPE13,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S13", TXT_SLOPE);
static const TemplateTypeClass Slope14(TEMPLATE_SLOPE14,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S14", TXT_SLOPE);
static const TemplateTypeClass Slope15(TEMPLATE_SLOPE15,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S15", TXT_SLOPE);
static const TemplateTypeClass Slope16(TEMPLATE_SLOPE16,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S16", TXT_SLOPE);
static const TemplateTypeClass Slope17(TEMPLATE_SLOPE17,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S17", TXT_SLOPE);
static const TemplateTypeClass Slope18(TEMPLATE_SLOPE18,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S18", TXT_SLOPE);
static const TemplateTypeClass Slope19(TEMPLATE_SLOPE19,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S19", TXT_SLOPE);
static const TemplateTypeClass Slope20(TEMPLATE_SLOPE20,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S20", TXT_SLOPE);
static const TemplateTypeClass Slope21(TEMPLATE_SLOPE21,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S21", TXT_SLOPE);
static const TemplateTypeClass Slope22(TEMPLATE_SLOPE22,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S22", TXT_SLOPE);
static const TemplateTypeClass Slope23(TEMPLATE_SLOPE23,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S23", TXT_SLOPE);
static const TemplateTypeClass Slope24(TEMPLATE_SLOPE24,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S24", TXT_SLOPE);
static const TemplateTypeClass Slope25(TEMPLATE_SLOPE25,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S25", TXT_SLOPE);
static const TemplateTypeClass Slope26(TEMPLATE_SLOPE26,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S26", TXT_SLOPE);
static const TemplateTypeClass Slope27(TEMPLATE_SLOPE27,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S27", TXT_SLOPE);
static const TemplateTypeClass Slope28(TEMPLATE_SLOPE28,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S28", TXT_SLOPE);
static const TemplateTypeClass Slope29(TEMPLATE_SLOPE29,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S29", TXT_SLOPE);
static const TemplateTypeClass Slope30(TEMPLATE_SLOPE30,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S30", TXT_SLOPE);
static const TemplateTypeClass Slope31(TEMPLATE_SLOPE31,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S31", TXT_SLOPE);
static const TemplateTypeClass Slope32(TEMPLATE_SLOPE32,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S32", TXT_SLOPE);
static const TemplateTypeClass Slope33(TEMPLATE_SLOPE33,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S33", TXT_SLOPE);
static const TemplateTypeClass Slope34(TEMPLATE_SLOPE34,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S34", TXT_SLOPE);
static const TemplateTypeClass Slope35(TEMPLATE_SLOPE35,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S35", TXT_SLOPE);
static const TemplateTypeClass Slope36(TEMPLATE_SLOPE36,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S36", TXT_SLOPE);
static const TemplateTypeClass Slope37(TEMPLATE_SLOPE37,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S37", TXT_SLOPE);
static const TemplateTypeClass Slope38(TEMPLATE_SLOPE38,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "S38", TXT_SLOPE);
static const TemplateTypeClass Patch01(TEMPLATE_PATCH01,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "P01", TXT_PATCH);
static const TemplateTypeClass Patch02(TEMPLATE_PATCH02,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "P02", TXT_PATCH);
static const TemplateTypeClass Patch03(TEMPLATE_PATCH03,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "P03", TXT_PATCH);
static const TemplateTypeClass Patch04(TEMPLATE_PATCH04,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "P04", TXT_PATCH);
static const TemplateTypeClass Patch07(TEMPLATE_PATCH07,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "P07", TXT_PATCH);
static const TemplateTypeClass Patch08(TEMPLATE_PATCH08,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "P08", TXT_PATCH);
static const TemplateTypeClass Patch13(TEMPLATE_PATCH13,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "P13", TXT_PATCH);
static const TemplateTypeClass Patch14(TEMPLATE_PATCH14,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "P14", TXT_PATCH);
static const TemplateTypeClass Patch15(TEMPLATE_PATCH15,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "P15", TXT_PATCH);
static const TemplateTypeClass River01(TEMPLATE_RIVER01,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV01", TXT_RIVER);
static const TemplateTypeClass River02(TEMPLATE_RIVER02,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV02", TXT_RIVER);
static const TemplateTypeClass River03(TEMPLATE_RIVER03,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV03", TXT_RIVER);
static const TemplateTypeClass River04(TEMPLATE_RIVER04,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV04", TXT_RIVER);
static const TemplateTypeClass River05(TEMPLATE_RIVER05,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV05", TXT_RIVER);
static const TemplateTypeClass River06(TEMPLATE_RIVER06,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV06", TXT_RIVER);
static const TemplateTypeClass River07(TEMPLATE_RIVER07,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV07", TXT_RIVER);
static const TemplateTypeClass River08(TEMPLATE_RIVER08,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV08", TXT_RIVER);
static const TemplateTypeClass River09(TEMPLATE_RIVER09,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV09", TXT_RIVER);
static const TemplateTypeClass River10(TEMPLATE_RIVER10,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV10", TXT_RIVER);
static const TemplateTypeClass River11(TEMPLATE_RIVER11,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV11", TXT_RIVER);
static const TemplateTypeClass River12(TEMPLATE_RIVER12,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV12", TXT_RIVER);
static const TemplateTypeClass River13(TEMPLATE_RIVER13,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV13", TXT_RIVER);
static const TemplateTypeClass River14(TEMPLATE_RIVER14,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV14", TXT_RIVER);
static const TemplateTypeClass River15(TEMPLATE_RIVER15,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RV15", TXT_RIVER);
static const TemplateTypeClass Ford1(TEMPLATE_FORD1,
                                     THEATERF_TEMPERATE | THEATERF_SNOW,
                                     "FORD1", TXT_RIVER);
static const TemplateTypeClass Ford2(TEMPLATE_FORD2,
                                     THEATERF_TEMPERATE | THEATERF_SNOW,
                                     "FORD2", TXT_RIVER);
static const TemplateTypeClass Falls1(TEMPLATE_FALLS1,
                                      THEATERF_TEMPERATE | THEATERF_SNOW,
                                      "FALLS1", TXT_RIVER);
static const TemplateTypeClass Falls1a(TEMPLATE_FALLS1A,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "FALLS1A", TXT_RIVER);
static const TemplateTypeClass Falls2(TEMPLATE_FALLS2,
                                      THEATERF_TEMPERATE | THEATERF_SNOW,
                                      "FALLS2", TXT_RIVER);
static const TemplateTypeClass Falls2a(TEMPLATE_FALLS2A,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "FALLS2A", TXT_RIVER);
static const TemplateTypeClass Bridge1x(TEMPLATE_BRIDGE1X,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BRIDGE1X", TXT_BRIDGE);
static const TemplateTypeClass Bridge1(TEMPLATE_BRIDGE1,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "BRIDGE1", TXT_BRIDGE);
static const TemplateTypeClass Bridge1h(TEMPLATE_BRIDGE1H,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BRIDGE1H", TXT_BRIDGE);
static const TemplateTypeClass Bridge1d(TEMPLATE_BRIDGE1D,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BRIDGE1D", TXT_BRIDGE);
static const TemplateTypeClass Bridge2x(TEMPLATE_BRIDGE2X,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BRIDGE2X", TXT_BRIDGE);
static const TemplateTypeClass Bridge2(TEMPLATE_BRIDGE2,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "BRIDGE2", TXT_BRIDGE);
static const TemplateTypeClass Bridge2h(TEMPLATE_BRIDGE2H,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BRIDGE2H", TXT_BRIDGE);
static const TemplateTypeClass Bridge2d(TEMPLATE_BRIDGE2D,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BRIDGE2D", TXT_BRIDGE);
static const TemplateTypeClass Bridge1ax(TEMPLATE_BRIDGE_1AX,
                                         THEATERF_TEMPERATE | THEATERF_SNOW,
                                         "BR1X", TXT_BRIDGE);
static const TemplateTypeClass Bridge1a(TEMPLATE_BRIDGE_1A,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BR1A", TXT_BRIDGE);
static const TemplateTypeClass Bridge1b(TEMPLATE_BRIDGE_1B,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BR1B", TXT_BRIDGE);
static const TemplateTypeClass Bridge1c(TEMPLATE_BRIDGE_1C,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BR1C", TXT_BRIDGE);
static const TemplateTypeClass Bridge2ax(TEMPLATE_BRIDGE_2AX,
                                         THEATERF_TEMPERATE | THEATERF_SNOW,
                                         "BR2X", TXT_BRIDGE);
static const TemplateTypeClass Bridge2a(TEMPLATE_BRIDGE_2A,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BR2A", TXT_BRIDGE);
static const TemplateTypeClass Bridge2b(TEMPLATE_BRIDGE_2B,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BR2B", TXT_BRIDGE);
static const TemplateTypeClass Bridge2c(TEMPLATE_BRIDGE_2C,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BR2C", TXT_BRIDGE);
static const TemplateTypeClass Bridge3a(TEMPLATE_BRIDGE_3A,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BR3A", TXT_BRIDGE);
static const TemplateTypeClass Bridge3b(TEMPLATE_BRIDGE_3B,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BR3B", TXT_BRIDGE);
static const TemplateTypeClass Bridge3c(TEMPLATE_BRIDGE_3C,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BR3C", TXT_BRIDGE);
static const TemplateTypeClass Bridge3d(TEMPLATE_BRIDGE_3D,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BR3D", TXT_BRIDGE);
static const TemplateTypeClass Bridge3e(TEMPLATE_BRIDGE_3E,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BR3E", TXT_BRIDGE);
static const TemplateTypeClass Bridge3f(TEMPLATE_BRIDGE_3F,
                                        THEATERF_TEMPERATE | THEATERF_SNOW,
                                        "BR3F", TXT_BRIDGE);
static const TemplateTypeClass ShoreCliff01(TEMPLATE_SHORECLIFF01,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC01", TXT_SHORE);
static const TemplateTypeClass ShoreCliff02(TEMPLATE_SHORECLIFF02,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC02", TXT_SHORE);
static const TemplateTypeClass ShoreCliff03(TEMPLATE_SHORECLIFF03,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC03", TXT_SHORE);
static const TemplateTypeClass ShoreCliff04(TEMPLATE_SHORECLIFF04,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC04", TXT_SHORE);
static const TemplateTypeClass ShoreCliff05(TEMPLATE_SHORECLIFF05,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC05", TXT_SHORE);
static const TemplateTypeClass ShoreCliff06(TEMPLATE_SHORECLIFF06,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC06", TXT_SHORE);
static const TemplateTypeClass ShoreCliff07(TEMPLATE_SHORECLIFF07,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC07", TXT_SHORE);
static const TemplateTypeClass ShoreCliff08(TEMPLATE_SHORECLIFF08,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC08", TXT_SHORE);
static const TemplateTypeClass ShoreCliff09(TEMPLATE_SHORECLIFF09,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC09", TXT_SHORE);
static const TemplateTypeClass ShoreCliff10(TEMPLATE_SHORECLIFF10,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC10", TXT_SHORE);
static const TemplateTypeClass ShoreCliff11(TEMPLATE_SHORECLIFF11,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC11", TXT_SHORE);
static const TemplateTypeClass ShoreCliff12(TEMPLATE_SHORECLIFF12,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC12", TXT_SHORE);
static const TemplateTypeClass ShoreCliff13(TEMPLATE_SHORECLIFF13,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC13", TXT_SHORE);
static const TemplateTypeClass ShoreCliff14(TEMPLATE_SHORECLIFF14,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC14", TXT_SHORE);
static const TemplateTypeClass ShoreCliff15(TEMPLATE_SHORECLIFF15,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC15", TXT_SHORE);
static const TemplateTypeClass ShoreCliff16(TEMPLATE_SHORECLIFF16,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC16", TXT_SHORE);
static const TemplateTypeClass ShoreCliff17(TEMPLATE_SHORECLIFF17,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC17", TXT_SHORE);
static const TemplateTypeClass ShoreCliff18(TEMPLATE_SHORECLIFF18,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC18", TXT_SHORE);
static const TemplateTypeClass ShoreCliff19(TEMPLATE_SHORECLIFF19,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC19", TXT_SHORE);
static const TemplateTypeClass ShoreCliff20(TEMPLATE_SHORECLIFF20,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC20", TXT_SHORE);
static const TemplateTypeClass ShoreCliff21(TEMPLATE_SHORECLIFF21,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC21", TXT_SHORE);
static const TemplateTypeClass ShoreCliff22(TEMPLATE_SHORECLIFF22,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC22", TXT_SHORE);
static const TemplateTypeClass ShoreCliff23(TEMPLATE_SHORECLIFF23,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC23", TXT_SHORE);
static const TemplateTypeClass ShoreCliff24(TEMPLATE_SHORECLIFF24,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC24", TXT_SHORE);
static const TemplateTypeClass ShoreCliff25(TEMPLATE_SHORECLIFF25,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC25", TXT_SHORE);
static const TemplateTypeClass ShoreCliff26(TEMPLATE_SHORECLIFF26,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC26", TXT_SHORE);
static const TemplateTypeClass ShoreCliff27(TEMPLATE_SHORECLIFF27,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC27", TXT_SHORE);
static const TemplateTypeClass ShoreCliff28(TEMPLATE_SHORECLIFF28,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC28", TXT_SHORE);
static const TemplateTypeClass ShoreCliff29(TEMPLATE_SHORECLIFF29,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC29", TXT_SHORE);
static const TemplateTypeClass ShoreCliff30(TEMPLATE_SHORECLIFF30,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC30", TXT_SHORE);
static const TemplateTypeClass ShoreCliff31(TEMPLATE_SHORECLIFF31,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC31", TXT_SHORE);
static const TemplateTypeClass ShoreCliff32(TEMPLATE_SHORECLIFF32,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC32", TXT_SHORE);
static const TemplateTypeClass ShoreCliff33(TEMPLATE_SHORECLIFF33,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC33", TXT_SHORE);
static const TemplateTypeClass ShoreCliff34(TEMPLATE_SHORECLIFF34,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC34", TXT_SHORE);
static const TemplateTypeClass ShoreCliff35(TEMPLATE_SHORECLIFF35,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC35", TXT_SHORE);
static const TemplateTypeClass ShoreCliff36(TEMPLATE_SHORECLIFF36,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC36", TXT_SHORE);
static const TemplateTypeClass ShoreCliff37(TEMPLATE_SHORECLIFF37,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC37", TXT_SHORE);
static const TemplateTypeClass ShoreCliff38(TEMPLATE_SHORECLIFF38,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "WC38", TXT_SHORE);
static const TemplateTypeClass Rough01(TEMPLATE_ROUGH01,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RF01", TXT_ROCK);
static const TemplateTypeClass Rough02(TEMPLATE_ROUGH02,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RF02", TXT_ROCK);
static const TemplateTypeClass Rough03(TEMPLATE_ROUGH03,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RF03", TXT_ROCK);
static const TemplateTypeClass Rough04(TEMPLATE_ROUGH04,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RF04", TXT_ROCK);
static const TemplateTypeClass Rough05(TEMPLATE_ROUGH05,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RF05", TXT_ROCK);
static const TemplateTypeClass Rough06(TEMPLATE_ROUGH06,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RF06", TXT_ROCK);
static const TemplateTypeClass Rough07(TEMPLATE_ROUGH07,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RF07", TXT_ROCK);
static const TemplateTypeClass Rough08(TEMPLATE_ROUGH08,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RF08", TXT_ROCK);
static const TemplateTypeClass Rough09(TEMPLATE_ROUGH09,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RF09", TXT_ROCK);
static const TemplateTypeClass Rough10(TEMPLATE_ROUGH10,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RF10", TXT_ROCK);
static const TemplateTypeClass Rough11(TEMPLATE_ROUGH11,
                                       THEATERF_TEMPERATE | THEATERF_SNOW,
                                       "RF11", TXT_ROCK);
static const TemplateTypeClass RiverCliff01(TEMPLATE_RIVERCLIFF01,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "RC01", TXT_RIVER);
static const TemplateTypeClass RiverCliff02(TEMPLATE_RIVERCLIFF02,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "RC02", TXT_RIVER);
static const TemplateTypeClass RiverCliff03(TEMPLATE_RIVERCLIFF03,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "RC03", TXT_RIVER);
static const TemplateTypeClass RiverCliff04(TEMPLATE_RIVERCLIFF04,
                                            THEATERF_TEMPERATE | THEATERF_SNOW,
                                            "RC04", TXT_RIVER);

static const TemplateTypeClass F01(TEMPLATE_F01,
                                   THEATERF_TEMPERATE | THEATERF_SNOW, "F01",
                                   TXT_RIVER);
static const TemplateTypeClass F02(TEMPLATE_F02,
                                   THEATERF_TEMPERATE | THEATERF_SNOW, "F02",
                                   TXT_RIVER);
static const TemplateTypeClass F03(TEMPLATE_F03,
                                   THEATERF_TEMPERATE | THEATERF_SNOW, "F03",
                                   TXT_RIVER);
static const TemplateTypeClass F04(TEMPLATE_F04,
                                   THEATERF_TEMPERATE | THEATERF_SNOW, "F04",
                                   TXT_RIVER);
static const TemplateTypeClass F05(TEMPLATE_F05,
                                   THEATERF_TEMPERATE | THEATERF_SNOW, "F05",
                                   TXT_RIVER);
static const TemplateTypeClass F06(TEMPLATE_F06,
                                   THEATERF_TEMPERATE | THEATERF_SNOW, "F06",
                                   TXT_RIVER);

static const TemplateTypeClass ARRO0001(TEMPLATE_ARRO0001, THEATERF_INTERIOR,
                                        "ARRO0001", TXT_INTERIOR);
static const TemplateTypeClass ARRO0002(TEMPLATE_ARRO0002, THEATERF_INTERIOR,
                                        "ARRO0002", TXT_INTERIOR);
static const TemplateTypeClass ARRO0003(TEMPLATE_ARRO0003, THEATERF_INTERIOR,
                                        "ARRO0003", TXT_INTERIOR);
static const TemplateTypeClass ARRO0004(TEMPLATE_ARRO0004, THEATERF_INTERIOR,
                                        "ARRO0004", TXT_INTERIOR);
static const TemplateTypeClass ARRO0005(TEMPLATE_ARRO0005, THEATERF_INTERIOR,
                                        "ARRO0005", TXT_INTERIOR);
static const TemplateTypeClass ARRO0006(TEMPLATE_ARRO0006, THEATERF_INTERIOR,
                                        "ARRO0006", TXT_INTERIOR);
static const TemplateTypeClass ARRO0007(TEMPLATE_ARRO0007, THEATERF_INTERIOR,
                                        "ARRO0007", TXT_INTERIOR);
static const TemplateTypeClass ARRO0008(TEMPLATE_ARRO0008, THEATERF_INTERIOR,
                                        "ARRO0008", TXT_INTERIOR);
static const TemplateTypeClass ARRO0009(TEMPLATE_ARRO0009, THEATERF_INTERIOR,
                                        "ARRO0009", TXT_INTERIOR);
static const TemplateTypeClass ARRO0010(TEMPLATE_ARRO0010, THEATERF_INTERIOR,
                                        "ARRO0010", TXT_INTERIOR);
static const TemplateTypeClass ARRO0011(TEMPLATE_ARRO0011, THEATERF_INTERIOR,
                                        "ARRO0011", TXT_INTERIOR);
static const TemplateTypeClass ARRO0012(TEMPLATE_ARRO0012, THEATERF_INTERIOR,
                                        "ARRO0012", TXT_INTERIOR);
static const TemplateTypeClass ARRO0013(TEMPLATE_ARRO0013, THEATERF_INTERIOR,
                                        "ARRO0013", TXT_INTERIOR);
static const TemplateTypeClass ARRO0014(TEMPLATE_ARRO0014, THEATERF_INTERIOR,
                                        "ARRO0014", TXT_INTERIOR);
static const TemplateTypeClass ARRO0015(TEMPLATE_ARRO0015, THEATERF_INTERIOR,
                                        "ARRO0015", TXT_INTERIOR);
static const TemplateTypeClass FLOR0001(TEMPLATE_FLOR0001, THEATERF_INTERIOR,
                                        "FLOR0001", TXT_INTERIOR);
static const TemplateTypeClass FLOR0002(TEMPLATE_FLOR0002, THEATERF_INTERIOR,
                                        "FLOR0002", TXT_INTERIOR);
static const TemplateTypeClass FLOR0003(TEMPLATE_FLOR0003, THEATERF_INTERIOR,
                                        "FLOR0003", TXT_INTERIOR);
static const TemplateTypeClass FLOR0004(TEMPLATE_FLOR0004, THEATERF_INTERIOR,
                                        "FLOR0004", TXT_INTERIOR);
static const TemplateTypeClass FLOR0005(TEMPLATE_FLOR0005, THEATERF_INTERIOR,
                                        "FLOR0005", TXT_INTERIOR);
static const TemplateTypeClass FLOR0006(TEMPLATE_FLOR0006, THEATERF_INTERIOR,
                                        "FLOR0006", TXT_INTERIOR);
static const TemplateTypeClass FLOR0007(TEMPLATE_FLOR0007, THEATERF_INTERIOR,
                                        "FLOR0007", TXT_INTERIOR);
static const TemplateTypeClass GFLR0001(TEMPLATE_GFLR0001, THEATERF_INTERIOR,
                                        "GFLR0001", TXT_INTERIOR);
static const TemplateTypeClass GFLR0002(TEMPLATE_GFLR0002, THEATERF_INTERIOR,
                                        "GFLR0002", TXT_INTERIOR);
static const TemplateTypeClass GFLR0003(TEMPLATE_GFLR0003, THEATERF_INTERIOR,
                                        "GFLR0003", TXT_INTERIOR);
static const TemplateTypeClass GFLR0004(TEMPLATE_GFLR0004, THEATERF_INTERIOR,
                                        "GFLR0004", TXT_INTERIOR);
static const TemplateTypeClass GFLR0005(TEMPLATE_GFLR0005, THEATERF_INTERIOR,
                                        "GFLR0005", TXT_INTERIOR);
static const TemplateTypeClass GSTR0001(TEMPLATE_GSTR0001, THEATERF_INTERIOR,
                                        "GSTR0001", TXT_INTERIOR);
static const TemplateTypeClass GSTR0002(TEMPLATE_GSTR0002, THEATERF_INTERIOR,
                                        "GSTR0002", TXT_INTERIOR);
static const TemplateTypeClass GSTR0003(TEMPLATE_GSTR0003, THEATERF_INTERIOR,
                                        "GSTR0003", TXT_INTERIOR);
static const TemplateTypeClass GSTR0004(TEMPLATE_GSTR0004, THEATERF_INTERIOR,
                                        "GSTR0004", TXT_INTERIOR);
static const TemplateTypeClass GSTR0005(TEMPLATE_GSTR0005, THEATERF_INTERIOR,
                                        "GSTR0005", TXT_INTERIOR);
static const TemplateTypeClass GSTR0006(TEMPLATE_GSTR0006, THEATERF_INTERIOR,
                                        "GSTR0006", TXT_INTERIOR);
static const TemplateTypeClass GSTR0007(TEMPLATE_GSTR0007, THEATERF_INTERIOR,
                                        "GSTR0007", TXT_INTERIOR);
static const TemplateTypeClass GSTR0008(TEMPLATE_GSTR0008, THEATERF_INTERIOR,
                                        "GSTR0008", TXT_INTERIOR);
static const TemplateTypeClass GSTR0009(TEMPLATE_GSTR0009, THEATERF_INTERIOR,
                                        "GSTR0009", TXT_INTERIOR);
static const TemplateTypeClass GSTR0010(TEMPLATE_GSTR0010, THEATERF_INTERIOR,
                                        "GSTR0010", TXT_INTERIOR);
static const TemplateTypeClass GSTR0011(TEMPLATE_GSTR0011, THEATERF_INTERIOR,
                                        "GSTR0011", TXT_INTERIOR);
static const TemplateTypeClass LWAL0001(TEMPLATE_LWAL0001, THEATERF_INTERIOR,
                                        "LWAL0001", TXT_INTERIOR);
static const TemplateTypeClass LWAL0002(TEMPLATE_LWAL0002, THEATERF_INTERIOR,
                                        "LWAL0002", TXT_INTERIOR);
static const TemplateTypeClass LWAL0003(TEMPLATE_LWAL0003, THEATERF_INTERIOR,
                                        "LWAL0003", TXT_INTERIOR);
static const TemplateTypeClass LWAL0004(TEMPLATE_LWAL0004, THEATERF_INTERIOR,
                                        "LWAL0004", TXT_INTERIOR);
static const TemplateTypeClass LWAL0005(TEMPLATE_LWAL0005, THEATERF_INTERIOR,
                                        "LWAL0005", TXT_INTERIOR);
static const TemplateTypeClass LWAL0006(TEMPLATE_LWAL0006, THEATERF_INTERIOR,
                                        "LWAL0006", TXT_INTERIOR);
static const TemplateTypeClass LWAL0007(TEMPLATE_LWAL0007, THEATERF_INTERIOR,
                                        "LWAL0007", TXT_INTERIOR);
static const TemplateTypeClass LWAL0008(TEMPLATE_LWAL0008, THEATERF_INTERIOR,
                                        "LWAL0008", TXT_INTERIOR);
static const TemplateTypeClass LWAL0009(TEMPLATE_LWAL0009, THEATERF_INTERIOR,
                                        "LWAL0009", TXT_INTERIOR);
static const TemplateTypeClass LWAL0010(TEMPLATE_LWAL0010, THEATERF_INTERIOR,
                                        "LWAL0010", TXT_INTERIOR);
static const TemplateTypeClass LWAL0011(TEMPLATE_LWAL0011, THEATERF_INTERIOR,
                                        "LWAL0011", TXT_INTERIOR);
static const TemplateTypeClass LWAL0012(TEMPLATE_LWAL0012, THEATERF_INTERIOR,
                                        "LWAL0012", TXT_INTERIOR);
static const TemplateTypeClass LWAL0013(TEMPLATE_LWAL0013, THEATERF_INTERIOR,
                                        "LWAL0013", TXT_INTERIOR);
static const TemplateTypeClass LWAL0014(TEMPLATE_LWAL0014, THEATERF_INTERIOR,
                                        "LWAL0014", TXT_INTERIOR);
static const TemplateTypeClass LWAL0015(TEMPLATE_LWAL0015, THEATERF_INTERIOR,
                                        "LWAL0015", TXT_INTERIOR);
static const TemplateTypeClass LWAL0016(TEMPLATE_LWAL0016, THEATERF_INTERIOR,
                                        "LWAL0016", TXT_INTERIOR);
static const TemplateTypeClass LWAL0017(TEMPLATE_LWAL0017, THEATERF_INTERIOR,
                                        "LWAL0017", TXT_INTERIOR);
static const TemplateTypeClass LWAL0018(TEMPLATE_LWAL0018, THEATERF_INTERIOR,
                                        "LWAL0018", TXT_INTERIOR);
static const TemplateTypeClass LWAL0019(TEMPLATE_LWAL0019, THEATERF_INTERIOR,
                                        "LWAL0019", TXT_INTERIOR);
static const TemplateTypeClass LWAL0020(TEMPLATE_LWAL0020, THEATERF_INTERIOR,
                                        "LWAL0020", TXT_INTERIOR);
static const TemplateTypeClass LWAL0021(TEMPLATE_LWAL0021, THEATERF_INTERIOR,
                                        "LWAL0021", TXT_INTERIOR);
static const TemplateTypeClass LWAL0022(TEMPLATE_LWAL0022, THEATERF_INTERIOR,
                                        "LWAL0022", TXT_INTERIOR);
static const TemplateTypeClass LWAL0023(TEMPLATE_LWAL0023, THEATERF_INTERIOR,
                                        "LWAL0023", TXT_INTERIOR);
static const TemplateTypeClass LWAL0024(TEMPLATE_LWAL0024, THEATERF_INTERIOR,
                                        "LWAL0024", TXT_INTERIOR);
static const TemplateTypeClass LWAL0025(TEMPLATE_LWAL0025, THEATERF_INTERIOR,
                                        "LWAL0025", TXT_INTERIOR);
static const TemplateTypeClass LWAL0026(TEMPLATE_LWAL0026, THEATERF_INTERIOR,
                                        "LWAL0026", TXT_INTERIOR);
static const TemplateTypeClass LWAL0027(TEMPLATE_LWAL0027, THEATERF_INTERIOR,
                                        "LWAL0027", TXT_INTERIOR);
static const TemplateTypeClass STRP0001(TEMPLATE_STRP0001, THEATERF_INTERIOR,
                                        "STRP0001", TXT_INTERIOR);
static const TemplateTypeClass STRP0002(TEMPLATE_STRP0002, THEATERF_INTERIOR,
                                        "STRP0002", TXT_INTERIOR);
static const TemplateTypeClass STRP0003(TEMPLATE_STRP0003, THEATERF_INTERIOR,
                                        "STRP0003", TXT_INTERIOR);
static const TemplateTypeClass STRP0004(TEMPLATE_STRP0004, THEATERF_INTERIOR,
                                        "STRP0004", TXT_INTERIOR);
static const TemplateTypeClass STRP0005(TEMPLATE_STRP0005, THEATERF_INTERIOR,
                                        "STRP0005", TXT_INTERIOR);
static const TemplateTypeClass STRP0006(TEMPLATE_STRP0006, THEATERF_INTERIOR,
                                        "STRP0006", TXT_INTERIOR);
static const TemplateTypeClass STRP0007(TEMPLATE_STRP0007, THEATERF_INTERIOR,
                                        "STRP0007", TXT_INTERIOR);
static const TemplateTypeClass STRP0008(TEMPLATE_STRP0008, THEATERF_INTERIOR,
                                        "STRP0008", TXT_INTERIOR);
static const TemplateTypeClass STRP0009(TEMPLATE_STRP0009, THEATERF_INTERIOR,
                                        "STRP0009", TXT_INTERIOR);
static const TemplateTypeClass STRP0010(TEMPLATE_STRP0010, THEATERF_INTERIOR,
                                        "STRP0010", TXT_INTERIOR);
static const TemplateTypeClass STRP0011(TEMPLATE_STRP0011, THEATERF_INTERIOR,
                                        "STRP0011", TXT_INTERIOR);
static const TemplateTypeClass WALL0001(TEMPLATE_WALL0001, THEATERF_INTERIOR,
                                        "WALL0001", TXT_INTERIOR);
static const TemplateTypeClass WALL0002(TEMPLATE_WALL0002, THEATERF_INTERIOR,
                                        "WALL0002", TXT_INTERIOR);
static const TemplateTypeClass WALL0003(TEMPLATE_WALL0003, THEATERF_INTERIOR,
                                        "WALL0003", TXT_INTERIOR);
static const TemplateTypeClass WALL0004(TEMPLATE_WALL0004, THEATERF_INTERIOR,
                                        "WALL0004", TXT_INTERIOR);
static const TemplateTypeClass WALL0005(TEMPLATE_WALL0005, THEATERF_INTERIOR,
                                        "WALL0005", TXT_INTERIOR);
static const TemplateTypeClass WALL0006(TEMPLATE_WALL0006, THEATERF_INTERIOR,
                                        "WALL0006", TXT_INTERIOR);
static const TemplateTypeClass WALL0007(TEMPLATE_WALL0007, THEATERF_INTERIOR,
                                        "WALL0007", TXT_INTERIOR);
static const TemplateTypeClass WALL0008(TEMPLATE_WALL0008, THEATERF_INTERIOR,
                                        "WALL0008", TXT_INTERIOR);
static const TemplateTypeClass WALL0009(TEMPLATE_WALL0009, THEATERF_INTERIOR,
                                        "WALL0009", TXT_INTERIOR);
static const TemplateTypeClass WALL0010(TEMPLATE_WALL0010, THEATERF_INTERIOR,
                                        "WALL0010", TXT_INTERIOR);
static const TemplateTypeClass WALL0011(TEMPLATE_WALL0011, THEATERF_INTERIOR,
                                        "WALL0011", TXT_INTERIOR);
static const TemplateTypeClass WALL0012(TEMPLATE_WALL0012, THEATERF_INTERIOR,
                                        "WALL0012", TXT_INTERIOR);
static const TemplateTypeClass WALL0013(TEMPLATE_WALL0013, THEATERF_INTERIOR,
                                        "WALL0013", TXT_INTERIOR);
static const TemplateTypeClass WALL0014(TEMPLATE_WALL0014, THEATERF_INTERIOR,
                                        "WALL0014", TXT_INTERIOR);
static const TemplateTypeClass WALL0015(TEMPLATE_WALL0015, THEATERF_INTERIOR,
                                        "WALL0015", TXT_INTERIOR);
static const TemplateTypeClass WALL0016(TEMPLATE_WALL0016, THEATERF_INTERIOR,
                                        "WALL0016", TXT_INTERIOR);
static const TemplateTypeClass WALL0017(TEMPLATE_WALL0017, THEATERF_INTERIOR,
                                        "WALL0017", TXT_INTERIOR);
static const TemplateTypeClass WALL0018(TEMPLATE_WALL0018, THEATERF_INTERIOR,
                                        "WALL0018", TXT_INTERIOR);
static const TemplateTypeClass WALL0019(TEMPLATE_WALL0019, THEATERF_INTERIOR,
                                        "WALL0019", TXT_INTERIOR);
static const TemplateTypeClass WALL0020(TEMPLATE_WALL0020, THEATERF_INTERIOR,
                                        "WALL0020", TXT_INTERIOR);
static const TemplateTypeClass WALL0021(TEMPLATE_WALL0021, THEATERF_INTERIOR,
                                        "WALL0021", TXT_INTERIOR);
static const TemplateTypeClass WALL0022(TEMPLATE_WALL0022, THEATERF_INTERIOR,
                                        "WALL0022", TXT_INTERIOR);
static const TemplateTypeClass WALL0023(TEMPLATE_WALL0023, THEATERF_INTERIOR,
                                        "WALL0023", TXT_INTERIOR);
static const TemplateTypeClass WALL0024(TEMPLATE_WALL0024, THEATERF_INTERIOR,
                                        "WALL0024", TXT_INTERIOR);
static const TemplateTypeClass WALL0025(TEMPLATE_WALL0025, THEATERF_INTERIOR,
                                        "WALL0025", TXT_INTERIOR);
static const TemplateTypeClass WALL0026(TEMPLATE_WALL0026, THEATERF_INTERIOR,
                                        "WALL0026", TXT_INTERIOR);
static const TemplateTypeClass WALL0027(TEMPLATE_WALL0027, THEATERF_INTERIOR,
                                        "WALL0027", TXT_INTERIOR);
static const TemplateTypeClass WALL0028(TEMPLATE_WALL0028, THEATERF_INTERIOR,
                                        "WALL0028", TXT_INTERIOR);
static const TemplateTypeClass WALL0029(TEMPLATE_WALL0029, THEATERF_INTERIOR,
                                        "WALL0029", TXT_INTERIOR);
static const TemplateTypeClass WALL0030(TEMPLATE_WALL0030, THEATERF_INTERIOR,
                                        "WALL0030", TXT_INTERIOR);
static const TemplateTypeClass WALL0031(TEMPLATE_WALL0031, THEATERF_INTERIOR,
                                        "WALL0031", TXT_INTERIOR);
static const TemplateTypeClass WALL0032(TEMPLATE_WALL0032, THEATERF_INTERIOR,
                                        "WALL0032", TXT_INTERIOR);
static const TemplateTypeClass WALL0033(TEMPLATE_WALL0033, THEATERF_INTERIOR,
                                        "WALL0033", TXT_INTERIOR);
static const TemplateTypeClass WALL0034(TEMPLATE_WALL0034, THEATERF_INTERIOR,
                                        "WALL0034", TXT_INTERIOR);
static const TemplateTypeClass WALL0035(TEMPLATE_WALL0035, THEATERF_INTERIOR,
                                        "WALL0035", TXT_INTERIOR);
static const TemplateTypeClass WALL0036(TEMPLATE_WALL0036, THEATERF_INTERIOR,
                                        "WALL0036", TXT_INTERIOR);
static const TemplateTypeClass WALL0037(TEMPLATE_WALL0037, THEATERF_INTERIOR,
                                        "WALL0037", TXT_INTERIOR);
static const TemplateTypeClass WALL0038(TEMPLATE_WALL0038, THEATERF_INTERIOR,
                                        "WALL0038", TXT_INTERIOR);
static const TemplateTypeClass WALL0039(TEMPLATE_WALL0039, THEATERF_INTERIOR,
                                        "WALL0039", TXT_INTERIOR);
static const TemplateTypeClass WALL0040(TEMPLATE_WALL0040, THEATERF_INTERIOR,
                                        "WALL0040", TXT_INTERIOR);
static const TemplateTypeClass WALL0041(TEMPLATE_WALL0041, THEATERF_INTERIOR,
                                        "WALL0041", TXT_INTERIOR);
static const TemplateTypeClass WALL0042(TEMPLATE_WALL0042, THEATERF_INTERIOR,
                                        "WALL0042", TXT_INTERIOR);
static const TemplateTypeClass WALL0043(TEMPLATE_WALL0043, THEATERF_INTERIOR,
                                        "WALL0043", TXT_INTERIOR);
static const TemplateTypeClass WALL0044(TEMPLATE_WALL0044, THEATERF_INTERIOR,
                                        "WALL0044", TXT_INTERIOR);
static const TemplateTypeClass WALL0045(TEMPLATE_WALL0045, THEATERF_INTERIOR,
                                        "WALL0045", TXT_INTERIOR);
static const TemplateTypeClass WALL0046(TEMPLATE_WALL0046, THEATERF_INTERIOR,
                                        "WALL0046", TXT_INTERIOR);
static const TemplateTypeClass WALL0047(TEMPLATE_WALL0047, THEATERF_INTERIOR,
                                        "WALL0047", TXT_INTERIOR);
static const TemplateTypeClass WALL0048(TEMPLATE_WALL0048, THEATERF_INTERIOR,
                                        "WALL0048", TXT_INTERIOR);
static const TemplateTypeClass WALL0049(TEMPLATE_WALL0049, THEATERF_INTERIOR,
                                        "WALL0049", TXT_INTERIOR);

static const TemplateTypeClass Xtra0001(TEMPLATE_XTRA0001, THEATERF_INTERIOR,
                                        "XTRA0001", TXT_INTERIOR);
static const TemplateTypeClass Xtra0002(TEMPLATE_XTRA0002, THEATERF_INTERIOR,
                                        "XTRA0002", TXT_INTERIOR);
static const TemplateTypeClass Xtra0003(TEMPLATE_XTRA0003, THEATERF_INTERIOR,
                                        "XTRA0003", TXT_INTERIOR);
static const TemplateTypeClass Xtra0004(TEMPLATE_XTRA0004, THEATERF_INTERIOR,
                                        "XTRA0004", TXT_INTERIOR);
static const TemplateTypeClass Xtra0005(TEMPLATE_XTRA0005, THEATERF_INTERIOR,
                                        "XTRA0005", TXT_INTERIOR);
static const TemplateTypeClass Xtra0006(TEMPLATE_XTRA0006, THEATERF_INTERIOR,
                                        "XTRA0006", TXT_INTERIOR);
static const TemplateTypeClass Xtra0007(TEMPLATE_XTRA0007, THEATERF_INTERIOR,
                                        "XTRA0007", TXT_INTERIOR);
static const TemplateTypeClass Xtra0008(TEMPLATE_XTRA0008, THEATERF_INTERIOR,
                                        "XTRA0008", TXT_INTERIOR);
static const TemplateTypeClass Xtra0009(TEMPLATE_XTRA0009, THEATERF_INTERIOR,
                                        "XTRA0009", TXT_INTERIOR);
static const TemplateTypeClass Xtra0010(TEMPLATE_XTRA0010, THEATERF_INTERIOR,
                                        "XTRA0010", TXT_INTERIOR);
static const TemplateTypeClass Xtra0011(TEMPLATE_XTRA0011, THEATERF_INTERIOR,
                                        "XTRA0011", TXT_INTERIOR);
static const TemplateTypeClass Xtra0012(TEMPLATE_XTRA0012, THEATERF_INTERIOR,
                                        "XTRA0012", TXT_INTERIOR);
static const TemplateTypeClass Xtra0013(TEMPLATE_XTRA0013, THEATERF_INTERIOR,
                                        "XTRA0013", TXT_INTERIOR);
static const TemplateTypeClass Xtra0014(TEMPLATE_XTRA0014, THEATERF_INTERIOR,
                                        "XTRA0014", TXT_INTERIOR);
static const TemplateTypeClass Xtra0015(TEMPLATE_XTRA0015, THEATERF_INTERIOR,
                                        "XTRA0015", TXT_INTERIOR);
static const TemplateTypeClass Xtra0016(TEMPLATE_XTRA0016, THEATERF_INTERIOR,
                                        "XTRA0016", TXT_INTERIOR);
static const TemplateTypeClass AntHill(TEMPLATE_HILL01, THEATERF_TEMPERATE,
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
    for (TemplateType index = TEMPLATE_FIRST; index < TEMPLATE_COUNT; index++) {
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
  for (TemplateType index = TEMPLATE_FIRST; index < TEMPLATE_COUNT; index++) {
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
