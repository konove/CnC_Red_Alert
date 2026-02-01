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

/***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood Auto Registration App           *
 *                                                                         *
 *                    File Name : PACKET.H                                 *
 *                                                                         *
 *                   Programmer : Philip W. Gorrow                         *
 *                                                                         *
 *                   Start Date : 04/19/96                                 *
 *                                                                         *
 *                  Last Update : April 19, 1996 [PWG]                     *
 *                                                                         *
 * This header defines the functions for the PacketClass.  The packet      *
 * class is used to create a linked list of field entries which can be     *
 * converted to a linear packet in a COMMS API compatible format.          *
 *																									*
 * Packets can be created empty and then have fields added to them or can  *
 * be created from an existing linear packet.
 **
 *																									*
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#ifndef CNC_RED_ALERT_TECH_PACKET_H_
#define CNC_RED_ALERT_TECH_PACKET_H_

#include <cstddef>
#include <cstdint>

#include "tech/field.h"

class PacketClass {
 public:
  PacketClass(short id = 0) : Size(0), ID(id), Head(nullptr) {}
  PacketClass(char* cur_buf);
  ~PacketClass();

  PacketClass(const PacketClass&) = delete;
  PacketClass& operator=(const PacketClass&) = delete;
  PacketClass(PacketClass&&) = delete;
  PacketClass& operator=(PacketClass&&) = delete;

  //
  // This function allows us to add a field to the start of the list.  As the
  // field is just
  //   a big linked list it makes no difference which end we add a member to.
  //
  void Add_Field(FieldClass* field);

  //
  // These convenience functions allow us to add a field directly to the list
  // without having to worry about newing one first.
  //
  void Add_Field(const char* field, char data) {
    Add_Field(new FieldClass(field, data));
  }
  void Add_Field(const char* field, unsigned char data) {
    Add_Field(new FieldClass(field, data));
  }
  void Add_Field(const char* field, short data) {
    Add_Field(new FieldClass(field, data));
  }
  void Add_Field(const char* field, unsigned short data) {
    Add_Field(new FieldClass(field, data));
  }
  void Add_Field(const char* field, long data) {
    Add_Field(new FieldClass(field, data));
  }
  void Add_Field(const char* field, unsigned long data) {
    Add_Field(new FieldClass(field, data));
  }
  void Add_Field(const char* field, const char* data) {
    Add_Field(new FieldClass(field, data));
  }
  void Add_Field(const char* field, void* data, int length) {
    Add_Field(new FieldClass(field, data, length));
  }

  //
  // These functions search for a field of a given name in the list and
  // return the data via a reference value.
  //
  FieldClass* Find_Field(const char* id);
  bool Get_Field(const char* id, char& data);
  bool Get_Field(const char* id, unsigned char& data);
  bool Get_Field(const char* id, short& data);
  bool Get_Field(const char* id, unsigned short& data);
  bool Get_Field(const char* id, long& data);
  bool Get_Field(const char* id, unsigned long& data);
  bool Get_Field(const char* id, char* data, std::size_t data_size);
  bool Get_Field(const char* id, void* data, int& length);

  char* Create_Comms_Packet(int& size);

 private:
  uint16_t Size;
  int16_t ID;
  FieldClass* Head;
};

#endif  // CNC_RED_ALERT_TECH_PACKET_H_
