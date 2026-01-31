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

/* $Header: /CounterStrike/BASE.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : BASE.CPP *
 *                                                                                             *
 *                   Programmer : Bill Randolph *
 *                                                                                             *
 *                   Start Date : 03/27/95 *
 *                                                                                             *
 *                  Last Update : July 30, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * BaseClass::Get_Building -- Returns ptr to the built building for
 *the given node           * BaseClass::Get_Node -- Finds the node that matches
 *the cell specified.                    * BaseClass::Get_Node -- Returns ptr to
 *the node corresponding to given object              * BaseClass::Is_Built --
 *Tells if given item in the list has been built yet                 *
 *   BaseClass::Is_Node -- Tells if the given building is part of our base list
 ** BaseClass::Load -- loads from a saved game file * BaseClass::Next_Buildable
 *-- returns ptr to the next node that needs to be built          *
 *   BaseClass::Read_INI -- INI reading routine * BaseClass::Save -- saves to a
 *saved game file                                             *
 *   BaseClass::Write_INI -- Writes all the base information to the INI
 *database.              * BaseNodeClass::operator != -- inequality operator *
 *   BaseNodeClass::operator == -- equality operator * BaseNodeClass::operator >
 *-- greater-than operator                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */
#include "ra/base.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ra/building.h"
#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/cell.h"
#include "ra/coord.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/house.h"
#include "ra/map.h"
#include "ra/mapedit.h"
#include "ra/monoc.h"
#include "ra/object.h"
#include "ra/type.h"
#include "ra/vector.h"
#include "tech/pipe.h"
#include "tech/straw.h"

/***********************************************************************************************
 * BaseNodeClass::operator == -- equality operator *
 *                                                                                             *
 * INPUT: * node      node to test against *
 *                                                                                             *
 * OUTPUT: * true = equal, false = not equal *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. *
 *=============================================================================================*/
int BaseNodeClass::operator==(const BaseNodeClass& node) {
  return Type == node.Type && Cell == node.Cell;
}

/***********************************************************************************************
 * BaseNodeClass::operator != -- inequality operator *
 *                                                                                             *
 * INPUT: * node      node to test against *
 *                                                                                             *
 * OUTPUT: * comparison result *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. *
 *=============================================================================================*/
int BaseNodeClass::operator!=(const BaseNodeClass& node) {
  return !(*this == node);
}

/***********************************************************************************************
 * BaseNodeClass::operator > -- greater-than operator *
 *                                                                                             *
 * INPUT: * node      node to test against *
 *                                                                                             *
 * OUTPUT: * comparison result *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. *
 *=============================================================================================*/
int BaseNodeClass::operator>(const BaseNodeClass&) { return true; }

/***********************************************************************************************
 * BaseClass::Load -- loads from a saved game file *
 *                                                                                             *
 * INPUT: * file      open file *
 *                                                                                             *
 * OUTPUT: * true = success, false = failure *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. * 07/04/1996 JLB : Converted to demand
 *driven data source.                                  *
 *=============================================================================================*/
bool BaseClass::Load(Straw& file) {
  int num_struct;
  int i;
  BaseNodeClass node;

  /*
  ** Read in & check the size of this class
  */
  if (file.Get(&i, sizeof(i)) != sizeof(i)) {
    return false;
  }

  if (i != sizeof(*this)) {
    return false;
  }

  /*
  ** Read in the House & the number of structures in the base
  */
  if (file.Get(&House, sizeof(House)) != sizeof(House)) {
    return false;
  }

  if (file.Get(&num_struct, sizeof(num_struct)) != sizeof(num_struct)) {
    return false;
  }

  /*
  ** Read each node entry & add it to the list
  */
  for (i = 0; i < num_struct; i++) {
    if (file.Get(&node, sizeof(node)) != sizeof(node)) {
      return false;
    }
    Nodes.Add(node);
  }

  return true;
}

/***********************************************************************************************
 * BaseClass::Save -- saves to a saved game file *
 *                                                                                             *
 * INPUT: * file      open file *
 *                                                                                             *
 * OUTPUT: * true = success, false = failure *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. * 07/04/1996 JLB : Converted to supply
 *driven data output.                                  *
 *=============================================================================================*/
bool BaseClass::Save(Pipe& file) const {
  int num_struct;
  int i;
  BaseNodeClass node;

  /*
  ** Write the size of this class
  */
  i = sizeof(*this);
  file.Put(&i, sizeof(i));

  /*
  ** Write the House & the number of structures in the base
  */
  file.Put(&House, sizeof(House));

  num_struct = Nodes.Count();
  file.Put(&num_struct, sizeof(num_struct));

  /*
  ** Write each node entry
  */
  for (i = 0; i < num_struct; i++) {
    node = Nodes[i];
    file.Put(&node, sizeof(node));
  }

  return true;
}

/***********************************************************************************************
 * BaseClass::Is_Built -- Tells if given item in the list has been built yet *
 *                                                                                             *
 * INPUT: * index      index into base list *
 *                                                                                             *
 * OUTPUT: * true = yes, false = no *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. *
 *=============================================================================================*/
bool BaseClass::Is_Built(int index) const {
  return Get_Building(index) != nullptr;
}

/***********************************************************************************************
 * BaseClass::Get_Building -- Returns ptr to the built building for the given
 *node             *
 *                                                                                             *
 * INPUT: * obj      pointer to building to test *
 *                                                                                             *
 * OUTPUT: * ptr to already-built building, nullptr if none *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. * 07/30/1996 JLB : Handle arbitrary
 *overlapper list length.                                 *
 *=============================================================================================*/
BuildingClass* BaseClass::Get_Building(int index) const {
  /*
  ** Check the location on the map where this building should be; if it's
  ** there, return a pointer to it.
  */
  const auto& node = Nodes[index];
  const CELL target_cell = node.Cell;

  // Create a reference to the map cell to avoid repeated lookups.
  const auto& map_cell = Map[target_cell];

  // Helper lambda to check if a candidate object matches our criteria.
  // Returns the cast pointer if successful, nullptr otherwise.
  auto check_candidate = [&](ObjectClass* candidate) -> BuildingClass* {
    if (!candidate) return nullptr;

    // 1. Fast RTTI Check: Is it actually a building?
    if (candidate->What_Am_I() != RTTI_BUILDING) {
      return nullptr;
    }

    // 2. Safe Cast: We verified the type above, so static_cast is safe and
    // fast.
    auto* building = dynamic_cast<BuildingClass*>(candidate);

    // 3. Logic Check: Does the Type matches?
    if (building->Class->Type != node.Type) {
      return nullptr;
    }

    // 4. Location Check: Ensure the building is actually AT this cell
    // coordinate.
    if (Coord_Cell(building->Coord) != target_cell) {
      return nullptr;
    }

    return building;
  };

  // Check the primary occupant.
  if (auto* match = check_candidate(map_cell.Cell_Building())) {
    return match;
  }

  // Check the overlappers.
  for (auto* overlapper : map_cell.Overlappers) {
    if (auto* match = check_candidate(overlapper)) {
      return match;
    }
  }

  return nullptr;
}

/***********************************************************************************************
 * BaseClass::Is_Node -- Tells if the given building is part of our base list *
 *                                                                                             *
 * INPUT: * obj      pointer to building to test *
 *                                                                                             *
 * OUTPUT: * true = building is a node in the list, false = isn't *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. *
 *=============================================================================================*/
bool BaseClass::Is_Node(const BuildingClass* obj) {
  return Get_Node(obj) != nullptr;
}

/***********************************************************************************************
 * BaseClass::Get_Node -- Returns ptr to the node corresponding to given object
 **
 *                                                                                             *
 * INPUT: * obj      pointer to building to test *
 *                                                                                             *
 * OUTPUT: * ptr to node *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. *
 *=============================================================================================*/
BaseNodeClass* BaseClass::Get_Node(const BuildingClass* obj) {
  for (int i = 0; i < Nodes.Count(); i++) {
    if (obj->Class->Type == Nodes[i].Type &&
        Coord_Cell(obj->Coord) == Nodes[i].Cell) {
      return &Nodes[i];
    }
  }
  return nullptr;
}

/***********************************************************************************************
 * BaseClass::Get_Node -- Finds the node that matches the cell specified. *
 *                                                                                             *
 *    This routine is used to find a matching node the corresponds to the cell
 *specified.      *
 *                                                                                             *
 * INPUT:   cell  -- The cell to use in finding a match. *
 *                                                                                             *
 * OUTPUT:  Returns a pointer to the matching node if found. If not found, then
 *nullptr is        * returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/12/1996 JLB : Created. *
 *=============================================================================================*/
BaseNodeClass* BaseClass::Get_Node(CELL cell) {
  for (int index = 0; index < Nodes.Count(); index++) {
    if (cell == Nodes[index].Cell) {
      return &Nodes[index];
    }
  }
  return nullptr;
}

/***********************************************************************************************
 * BaseClass::Next_Buildable -- returns ptr to the next node that needs to be
 *built            *
 *                                                                                             *
 * If 'type' is not NONE, returns ptr to the next "hole" in the list of the
 *given type.        * Otherwise, returns ptr to the next hole in the list of
 *any type.                            *
 *                                                                                             *
 * INPUT: * type      type of building to check for *
 *                                                                                             *
 * OUTPUT: * ptr to a BaseNodeClass, nullptr if none *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. *
 *=============================================================================================*/
BaseNodeClass* BaseClass::Next_Buildable(StructType type) {
  /*
  ** Loop through all node entries, returning a pointer to the first
  ** un-built one that matches the requested type.
  */
  for (int i = 0; i < Nodes.Count(); i++) {
    /*
    ** For STRUCT_NONE, return the first hole found
    */
    if (type == STRUCT_NONE) {
      if (!Is_Built(i)) {
        return &Nodes[i];
      }

    } else {
      /*
      ** For a "real" building type, return the first hold for that type
      */
      if (Nodes[i].Type == type && !Is_Built(i)) {
        return &Nodes[i];
      }
    }
  }

  // If no entry could be found, then create a fake one that will allow
  // placement of the building. Make it static and reuse the next time this
  // routine is called.

  return nullptr;
}

/***********************************************************************************************
 * BaseClass::Read_INI -- INI reading routine *
 *                                                                                             *
 * INI entry format: * BLDG=COORDINATE * BLDG=COORDINATE *
 *        ... *
 *                                                                                             *
 * INPUT: * buffer      pointer to loaded INI file *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * This routines assumes there is only one base defined for the
 *scenario.                 *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. * 02/20/1996 JLB : Fixed to know what
 *house to build base from.                             *
 *=============================================================================================*/
void BaseClass::Read_INI(CCINIClass& ini) {
  char buf[128];
  char uname[10];
  BaseNodeClass node;  // node to add to list

  Mono_Clear_Screen();
  /*
  **	First, determine the house of the human player, and set the Base's house
  **	accordingly.
  */
  House = ini.Get_HousesType(INI_Name(), "Player", PlayerPtr->Class->House);

  /*
  **	Read the number of buildings that will go into the base node list
  */
  int count = ini.Get_Int(INI_Name(), "Count", 0);

  /*
  **	Read each entry in turn, in the same order they were written out.
  */
  for (int i = 0; i < count; i++) {
    /*
    ** Get an INI entry
    */
    sprintf(uname, "%03d", i);
    ini.Get_String(INI_Name(), uname, nullptr, buf, sizeof(buf));

    /*
    ** Set the node's building type
    */
    node.Type = BuildingTypeClass::From_Name(strtok(buf, ","));

    /*
    ** Read & set the node's coordinate
    */
    node.Cell = atoi(strtok(nullptr, ","));

    /*
    ** Add this node to the Base's list
    */
    Nodes.Add(node);
  }
}

/***********************************************************************************************
 * BaseClass::Write_INI -- Writes all the base information to the INI database.
 **
 *                                                                                             *
 *    Use this routine to write all prebuild base information to the INI
 *database specified.   *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database to store the data to. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   If there was any preexisting prebuild base data in the database,
 *it will be     * be erased by this routine. *
 *                                                                                             *
 * HISTORY: * 07/30/1996 JLB : Created. *
 *=============================================================================================*/
void BaseClass::Write_INI(CCINIClass& ini) {
  /*
  **	Clear out all existing base data from the ini file.
  */
  ini.Clear(INI_Name());

  if (House != HOUSE_NONE) {
    /*
    **	Write out the owner of this buildable list.
    */
    ini.Put_HousesType(INI_Name(), "Player", House);

    /*
    **	Save the # of buildings in the Nodes list.  This is essential because
    **	they must be read in the same order they were created, so "000" must be
    **	read first, etc.
    */
    ini.Put_Int(INI_Name(), "Count", Nodes.Count());

    // Write each entry into the INI
    for (size_t i = 0; i < Nodes.Count(); i++) {
      char buf[128];
      char uname[10];

      snprintf(uname, sizeof(uname), "%03lu", i);

      snprintf(buf, sizeof(buf), "%s,%d",
               BuildingTypeClass::As_Reference(Nodes[i].Type).IniName,
               Nodes[i].Cell);

      ini.Put_String(INI_Name(), uname, buf);
    }
  }
}
