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
#ifndef CNC_RED_ALERT_ENGINE_NET_PACKET_H_
#define CNC_RED_ALERT_ENGINE_NET_PACKET_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "engine/net/field.h"

class PacketClass {
 public:
  explicit PacketClass(int16_t id = 0) : Size(0), ID(id) {}
  explicit PacketClass(std::span<const std::byte> curbuf);
  ~PacketClass() = default;

  PacketClass(const PacketClass&) = delete;
  PacketClass& operator=(const PacketClass&) = delete;
  PacketClass(PacketClass&&) = delete;
  PacketClass& operator=(PacketClass&&) = delete;

  // Adds a field to the packet. A later field comes first in the packet and
  // hides an earlier one of the same name from Find_Field.
  void Add_Field(FieldClass field);

  //
  // These convenience functions construct the field from its name and value.
  //
  void Add_Field(const char* field, char data) {
    Add_Field(FieldClass(field, data));
  }
  void Add_Field(const char* field, unsigned char data) {
    Add_Field(FieldClass(field, data));
  }
  void Add_Field(const char* field, int16_t data) {
    Add_Field(FieldClass(field, data));
  }
  void Add_Field(const char* field, uint16_t data) {
    Add_Field(FieldClass(field, data));
  }
  void Add_Field(const char* field, int32_t data) {
    Add_Field(FieldClass(field, data));
  }
  void Add_Field(const char* field, uint32_t data) {
    Add_Field(FieldClass(field, data));
  }
  void Add_Field(const char* field, const char* data) {
    Add_Field(FieldClass(field, data));
  }
  void Add_Field(const char* field, std::span<const std::byte> data) {
    Add_Field(FieldClass(field, data));
  }

  //
  // These functions search for a field of a given name in the list and
  // return the data via a reference value.
  //
  FieldClass* Find_Field(const char* id);
  bool Get_Field(const char* id, char& data);
  bool Get_Field(const char* id, unsigned char& data);
  bool Get_Field(const char* id, int16_t& data);
  bool Get_Field(const char* id, uint16_t& data);
  bool Get_Field(const char* id, int32_t& data);
  bool Get_Field(const char* id, uint32_t& data);
  bool Get_Field(const char* id, std::span<char> data);
  bool Get_Field(const char* id, std::span<std::byte> data, int& length);

  // Serializes the packet for the wire: the 4-byte header (size, ID), then
  // each field's header and data, padded to a multiple of 4 bytes.
  [[nodiscard]] std::vector<char> Create_Comms_Packet();

 private:
  uint16_t Size;
  int16_t ID;
  std::vector<FieldClass> Fields;  // In the order added; sent last first.
};

#endif  // CNC_RED_ALERT_ENGINE_NET_PACKET_H_
