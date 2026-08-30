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
 *                    File Name : FIELD.H                                  *
 *                                                                         *
 *                   Programmer : Philip W. Gorrow                         *
 *                                                                         *
 *                   Start Date : 04/22/96                                 *
 *                                                                         *
 *                  Last Update : April 22, 1996 [PWG]                     *
 *                                                                         *
 * This module takes care of maintaining the field list used to process    *
 * packets.
 **
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#ifndef CNC_RED_ALERT_TECH_FIELD_H_
#define CNC_RED_ALERT_TECH_FIELD_H_

#define FIELD_HEADER_SIZE (sizeof(FieldClass) - (sizeof(void*) * 2))

#define TYPE_CHAR 1
#define TYPE_UNSIGNED_CHAR 2
#define TYPE_SHORT 3
#define TYPE_UNSIGNED_SHORT 4
#define TYPE_LONG 5
#define TYPE_UNSIGNED_LONG 6
#define TYPE_STRING 7
#define TYPE_CHUNK 20

class FieldClass {
 public:
  friend class PacketClass;
  //
  // Define constructors to be able to create all the different kinds
  // of fields.
  //
  // Members are value-initialized rather than left indeterminate: the
  // packet reader default-constructs a field and then memcpy's only
  // FIELD_HEADER_SIZE bytes into it, which covers ID, DataType and Size but
  // not Data or Next.
  FieldClass() : ID{}, DataType(0), Size(0), Data(nullptr), Next(nullptr) {}
  FieldClass(const char* id, char data);
  FieldClass(const char* id, unsigned char data);
  FieldClass(const char* id, short data);
  FieldClass(const char* id, unsigned short data);
  FieldClass(const char* id, long data);
  FieldClass(const char* id, unsigned long data);
  FieldClass(const char* id, const char* data);
  FieldClass(const char* id, void* data, int length);

  void Host_To_Net();
  void Net_To_Host();

 private:
  char ID[4];               // id value of this field
  unsigned short DataType;  // id of the data type we are using
  unsigned short Size;      // size of the data portion of this field
  void* Data;               // pointer to the data portion of this field
  FieldClass* Next;         // pointer to the next field in the field list
};

#endif  // CNC_RED_ALERT_TECH_FIELD_H_
