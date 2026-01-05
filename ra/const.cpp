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

/* $Header: /CounterStrike/CONST.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CONST.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 20, 1993 *
 *                                                                                             *
 *                  Last Update : September 20, 1993   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/const.h"

#include "ra/defines.h"

// TODO(konove): Move to globals. These are not constants.

/***************************************************************************
**	This specifies the odds of receiving the various random crate power
**	ups. The odds are expressed as "shares" of 100 percent.
*/
int CrateShares[CRATE_COUNT] = {
    50,  //	CRATE_MONEY
    20,  //	CRATE_UNIT
    3,   //	CRATE_PARA_BOMB
    1,   //	CRATE_HEAL_BASE
    3,   //	CRATE_CLOAK
    5,   //	CRATE_EXPLOSION
    5,   //	CRATE_NAPALM
    20,  //	CRATE_SQUAD
    1,   //	CRATE_DARKNESS
    1,   //	CRATE_REVEAL
    3,   //	CRATE_SONAR
    10,  //	CRATE_ARMOR
    10,  //	CRATE_SPEED
    10,  //	CRATE_FIREPOWER
    1,   //	CRATE_ICBM
    1,   //	CRATE_TIMEQUAKE
    3,   //	CRATE_INVULN
    5    // CRATE_VORTEX
};

AnimType CrateAnims[CRATE_COUNT] = {
    ANIM_NONE,  //	CRATE_MONEY
    ANIM_NONE,  //	CRATE_UNIT
    ANIM_NONE,  //	CRATE_PARA_BOMB
    ANIM_NONE,  //	CRATE_HEAL_BASE
    ANIM_NONE,  //	CRATE_CLOAK
    ANIM_NONE,  //	CRATE_EXPLOSION
    ANIM_NONE,  //	CRATE_NAPALM
    ANIM_NONE,  //	CRATE_SQUAD
    ANIM_NONE,  //	CRATE_DARKNESS
    ANIM_NONE,  //	CRATE_REVEAL
    ANIM_NONE,  //	CRATE_SONAR
    ANIM_NONE,  //	CRATE_ARMOR
    ANIM_NONE,  //	CRATE_SPEED
    ANIM_NONE,  //	CRATE_FIREPOWER
    ANIM_NONE,  //	CRATE_ICBM
    ANIM_NONE,  //	CRATE_TIMEQUAKE
    ANIM_NONE,  //	CRATE_INVULN
    ANIM_NONE   // CRATE_VORTEX
};

int CrateData[CRATE_COUNT] = {
    0,  //	CRATE_MONEY
    0,  //	CRATE_UNIT
    0,  //	CRATE_PARA_BOMB
    0,  //	CRATE_HEAL_BASE
    0,  //	CRATE_CLOAK
    0,  //	CRATE_EXPLOSION
    0,  //	CRATE_NAPALM
    0,  //	CRATE_SQUAD
    0,  //	CRATE_DARKNESS
    0,  //	CRATE_REVEAL
    0,  //	CRATE_SONAR
    0,  //	CRATE_ARMOR
    0,  //	CRATE_SPEED
    0,  //	CRATE_FIREPOWER
    0,  //	CRATE_ICBM
    0,  //	CRATE_TIMEQUAKE
    0,  //	CRATE_INVULN
    0   //	CRATE_VORTEX
};

GroundType Ground[LAND_COUNT];
