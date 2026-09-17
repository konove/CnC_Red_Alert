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

/* $Header:   F:\projects\c&c\vcs\code\base.cpv   1.9   16 Oct 1995 16:48:56
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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
 *                  Last Update : March 27, 1995 *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * BaseClass::Get_Building -- Returns ptr to the built building for
 *the given node           * BaseClass::Get_Node -- Returns ptr to the node
 *corresponding to given object              * BaseClass::Is_Built -- Tells if
 *given item in the list has been built yet                 * BaseClass::Is_Node
 *-- Tells if the given building is part of our base list                *
 *   BaseClass::Load -- loads from a saved game file * BaseClass::Next_Buildable
 *-- returns ptr to the next node that needs to be built          *
 *   BaseClass::Read_INI -- INI reading routine * BaseClass::Save -- saves to a
 *saved game file                                             *
 *   BaseClass::Write_INI -- INI writing routine * BaseNodeClass::operator != --
 *inequality operator                                         *
 *   BaseNodeClass::operator == -- equality operator * BaseNodeClass::operator >
 *-- greater-than operator                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

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
#include "td/base.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "port/tokenizer.h"
#include "td/building.h"
#include "td/cell.h"
#include "td/defines.h"
#include "td/externs.h"
#include "td/inline.h"
#include "td/object.h"
#include "td/profile.h"
#include "td/type.h"
#include "td/vector.h"
#include "tech/number_parse.h"

bool BaseNodeClass::operator==(const BaseNodeClass& node) const {
  return Type == node.Type && Coord == node.Coord;
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
bool BaseNodeClass::operator!=(const BaseNodeClass& node) const {
  return Type != node.Type || Coord != node.Coord;
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
bool BaseNodeClass::operator>(const BaseNodeClass& /*unused*/) { return true; }

/***********************************************************************************************
 * BaseClass::Read_INI -- INI reading routine *
 *                                                                                             *
 * INI entry format: * BLDG=COORD * BLDG=COORD *
 *        ... *
 *                                                                                             *
 * INPUT: * buffer      pointer to loaded INI file *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * This routines assumes there is only one base defined for the
 *scenario.                 *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. *
 *=============================================================================================*/
void BaseClass::Read_INI(char* buffer) {
  char buf[128];
  char uname[10];
  BaseNodeClass node;  // node to add to list

  /*
  **	First, determine the house of the human player, and set the Base's house
  **	accordingly.
  */
  WWGetPrivateProfileString("BASIC", "Player", "GoodGuy",
                            std::span(buf).first(static_cast<std::size_t>(20)),
                            buffer);
  if (HouseTypeClass::From_Name(buf) == HOUSE_GOOD) {
    House = HOUSE_BAD;
  } else {
    House = HOUSE_GOOD;
  }

  /*
  **	Read the number of buildings that will go into the base node list
  */
  const int count = WWGetPrivateProfileInt(INI_Name(), "Count", 0, buffer);

  /*
  **	Read each entry in turn, in the same order they were written out.
  */
  for (int i = 0; i < count; i++) {
    /*
    ** Get an INI entry
    */
    absl::SNPrintF(uname, sizeof(uname), "%03d", i);
    WWGetPrivateProfileString(
        INI_Name(), uname, nullptr,
        std::span(buf).first(static_cast<std::size_t>(sizeof(buf) - 1)),
        buffer);

    /*
    ** Set the node's building type
    */
    port::Tokenizer tokens(buf, ",");
    node.Type = BuildingTypeClass::From_Name(tokens.Next());

    /*
    ** Read & set the node's coordinate
    */
    const char* coordinate_text = tokens.Next();
    if (coordinate_text == nullptr) {
      continue;
    }
    const auto coordinate = tech::ParseDecimalBits(coordinate_text);
    if (!coordinate) {
      continue;
    }
    node.Coord = *coordinate;

    /*
    ** Add this node to the Base's list
    */
    Nodes.Add(node);
  }
}

/***********************************************************************************************
 * BaseClass::Write_INI -- INI writing routine *
 *                                                                                             *
 * INI entry format: * BLDG=COORD * BLDG=COORD *
 *        ... *
 *                                                                                             *
 * INPUT: * buffer      pointer to loaded INI file staging area *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * This routines assumes there is only one base defined for the
 *scenario.                 *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. *
 *=============================================================================================*/
void BaseClass::Write_INI(std::span<char> buffer) {
  char buf[128];
  char uname[10];

  /*
  **	Clear out all existing teamtype data from the INI file.
  */
  WWWritePrivateProfileString(INI_Name(), nullptr, nullptr, buffer);

  /*
  **	Save the # of buildings in the Nodes list.  This is essential because
  **	they must be read in the same order they were created, so "000" must be
  **	read first, etc.
  */
  WWWritePrivateProfileInt(INI_Name(), "Count", static_cast<int>(Nodes.Count()),
                           buffer);

  /*
  **	Write each entry into the INI
  */
  for (int i = 0; i < Nodes.Count(); i++) {
    absl::SNPrintF(uname, sizeof(uname), "%03d", i);
    absl::SNPrintF(buf, sizeof(buf), "%s,%d",
                   BuildingTypeClass::As_Reference(Nodes.at(i).Type).IniName,
                   static_cast<int>(Nodes.at(i).Coord));

    WWWritePrivateProfileString(INI_Name(), uname, buf, buffer);
  }
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
bool BaseClass::Is_Built(int index) { return Get_Building(index) != nullptr; }

/***********************************************************************************************
 * BaseClass::Get_Building -- Returns ptr to the built building for the given
 *node             *
 *                                                                                             *
 * INPUT: * obj      pointer to building to test *
 *                                                                                             *
 * OUTPUT: * ptr to already-built building, NULL if none *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/24/1995 BRR : Created. *
 *=============================================================================================*/
BuildingClass* BaseClass::Get_Building(int index) {
  ObjectClass* obj[4];

  /*
  ** Check the location on the map where this building should be; if it's
  ** there, return a pointer to it.
  */
  const CELL cell = Coord_Cell(Nodes.at(index).Coord);

  base::At(obj, 0) = Map.at(cell).Cell_Building();
  base::At(obj, 1) = base::At(Map.at(cell).Overlappers, 0);
  base::At(obj, 2) = base::At(Map.at(cell).Overlappers, 1);
  base::At(obj, 3) = base::At(Map.at(cell).Overlappers, 2);

  BuildingClass* bldg = nullptr;
  for (auto& i : obj) {
    if (i && i->Coord == Nodes.at(index).Coord &&
        i->What_Am_I() == RTTI_BUILDING &&
        dynamic_cast<BuildingClass*>(i)->Class->Type == Nodes.at(index).Type) {
      bldg = dynamic_cast<BuildingClass*>(i);
      break;
    }
  }

  return bldg;
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
bool BaseClass::Is_Node(BuildingClass* obj) { return Get_Node(obj) != nullptr; }

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
BaseNodeClass* BaseClass::Get_Node(BuildingClass* obj) {
  for (int i = 0; i < Nodes.Count(); i++) {
    if (obj->Class->Type == Nodes.at(i).Type &&
        obj->Coord == Nodes.at(i).Coord) {
      return &Nodes.at(i);
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
 * OUTPUT: * ptr to a BaseNodeClass, NULL if none *
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
        return &Nodes.at(i);
      }

    } else {
      /*
      ** For a "real" building type, return the first hold for that type
      */
      if (Nodes.at(i).Type == type && !Is_Built(i)) {
        return &Nodes.at(i);
      }
    }
  }

  // If no entry could be found, then create a fake one that will allow
  // placement of the building. Make it static and reuse the next time this
  // routine is called.

  return nullptr;
}
