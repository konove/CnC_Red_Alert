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

/* $Header: /CounterStrike/INI.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : INI.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/15/96 *
 *                                                                                             *
 *                  Last Update : May 15, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_INI_H_
#define CNC_RED_ALERT_RA_INI_H_

#include <cstdlib>
#include <string>

#include "ra/defines.h"
#include "ra/object.h"
#include "ra/search.h"
#include "tech/byte_sink.h"
#include "tech/byte_source.h"
#include "tech/crc.h"
#include "tech/file.h"
#include "tech/fixed.h"
#include "tech/listnode.h"
#include "tech/pk.h"

/*
**	This is an INI database handler class. It handles a database with a disk
*format identical *	to the INI files commonly used by Windows.
*/
class INIClass {
 public:
  INIClass() = default;
  ~INIClass();
  INIClass(const INIClass&) = delete;
  INIClass& operator=(const INIClass&) = delete;
  INIClass(INIClass&&) = delete;
  INIClass& operator=(INIClass&&) = delete;

  /*
  **	Fetch and store INI data.
  */
  bool Load(File& file);
  bool Load(ByteSource& file);
  bool Save(File& file) const;
  bool Save(ByteSink& pipe) const;

  /*
  **	Erase all data within this INI file manager.
  */
  bool Clear(const char* section = nullptr, const char* entry = nullptr);

  int Line_Count(const char* section) const;
  [[nodiscard]] bool Is_Loaded() const { return !SectionList.Is_Empty(); }
  [[nodiscard]] int Size() const;
  bool Is_Present(const char* section, const char* entry = nullptr) const {
    if (entry == nullptr) {
      return Find_Section(section) != nullptr;
    }
    return Find_Entry(section, entry) != nullptr;
  }

  /*
  **	Fetch the number of sections in the INI file or verify if a specific
  **	section is present.
  */
  [[nodiscard]] int Section_Count() const;
  bool Section_Present(const char* section) const {
    return Find_Section(section) != nullptr;
  }

  /*
  **	Fetch the number of entries in a section or get a particular entry in a
  *section.
  */
  int Entry_Count(const char* section) const;
  const char* Get_Entry(const char* section, int index) const;

  /*
  **	Get the various data types from the section and entry specified.
  */
  int Get_String(const char* section, const char* entry, const char* defvalue,
                 char* buffer, int size) const;
  int Get_Int(const char* section, const char* entry, int defvalue = 0) const;
  int Get_Hex(const char* section, const char* entry, int defvalue = 0) const;
  bool Get_Bool(const char* section, const char* entry,
                bool defvalue = false) const;
  int Get_TextBlock(const char* section, char* buffer, int len) const;
  int Get_UUBlock(const char* section, void* block, int len) const;
  [[nodiscard]] PKey Get_PKey(bool fast) const;
  fixed Get_Fixed(const char* section, const char* entry, fixed defvalue) const;

  /*
  **	Put a data type to the section and entry specified.
  */
  bool Put_Fixed(const char* section, const char* entry, fixed value);
  bool Put_String(const char* section, const char* entry, const char* string);
  bool Put_Hex(const char* section, const char* entry, int number);
  bool Put_Int(const char* section, const char* entry, int number,
               int format = 0);
  bool Put_Bool(const char* section, const char* entry, bool value);
  bool Put_TextBlock(const char* section, const char* text);
  bool Put_UUBlock(const char* section, const void* block, int len);
  bool Put_PKey(const PKey& key);

 protected:
  enum { MAX_LINE_LENGTH = 128 };

  /*
  **	The value entries for the INI file are stored as objects of this type.
  **	The entry identifier and value string are combined into this object.
  */
  struct INIEntry : Node<INIEntry> {
    explicit INIEntry(const char* entry = "", const char* value = "")
        : Entry(entry), Value(value) {}
    ~INIEntry() override = default;
    INIEntry(const INIEntry&) = delete;
    INIEntry& operator=(const INIEntry&) = delete;
    INIEntry(INIEntry&&) = delete;
    INIEntry& operator=(INIEntry&&) = delete;
    [[nodiscard]] int Index_ID() const {
      return static_cast<int>(CrcEngine::Compute(Entry));
    }

    std::string Entry;
    std::string Value;
  };

  /*
  **	Each section (bracketed) is represented by an object of this type. All
  *entries *	subordinate to this section are attached.
  */
  struct INISection : Node<INISection> {
    explicit INISection(const char* section) : Section(section) {}
    ~INISection() override { EntryList.Delete(); }
    INISection(const INISection&) = delete;
    INISection& operator=(const INISection&) = delete;
    INISection(INISection&&) = delete;
    INISection& operator=(INISection&&) = delete;
    INIEntry* Find_Entry(const char* entry) const;
    [[nodiscard]] int Index_ID() const {
      return static_cast<int>(CrcEngine::Compute(Section));
    }

    std::string Section;
    List<INIEntry> EntryList;
    IndexClass<INIEntry*> EntryIndex;
  };

  /*
  **	Utility routines to help find the appropriate section and entry objects.
  */
  INISection* Find_Section(const char* section) const;
  INIEntry* Find_Entry(const char* section, const char* entry) const;
  static void Strip_Comments(char* buffer);

  /*
  **	This is the list of all sections within this INI file.
  */
  List<INISection> SectionList;

  IndexClass<INISection*> SectionIndex;
};

void Write_Scenario_INI(const char* fname);
bool Read_Scenario_INI(const char* fname, bool fresh = true);
bool Scan_Place_Object(ObjectClass* obj, CELL cell);
void Assign_Houses();

#endif  // CNC_RED_ALERT_RA_INI_H_
