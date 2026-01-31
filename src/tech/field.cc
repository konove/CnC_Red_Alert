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
 *                    File Name : FIELD.CPP                                *
 *                                                                         *
 *                   Programmer : Philip W. Gorrow                         *
 *                                                                         *
 *                   Start Date : 04/22/96                                 *
 *                                                                         *
 *                  Last Update : April 22, 1996 [PWG]                     *
 *                                                                         *
 *  Actual member function for the field class.                            *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "tech/field.h"

#include <cstring>

// htons/htonl
#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

FieldClass::FieldClass(const char* id, char data) {
  strncpy(ID, id, sizeof(ID));
  DataType = TYPE_CHAR;
  Size = sizeof(data);
  Data = new char[Size];
  memcpy(Data, &data, Size);
  Next = nullptr;
}

FieldClass::FieldClass(const char* id, unsigned char data) {
  strncpy(ID, id, sizeof(ID));
  DataType = TYPE_UNSIGNED_CHAR;
  Size = sizeof(data);
  Data = new char[Size];
  memcpy(Data, &data, Size);
  Next = nullptr;
}

FieldClass::FieldClass(const char* id, short data) {
  strncpy(ID, id, sizeof(ID));
  DataType = TYPE_SHORT;
  Size = sizeof(data);
  Data = new char[Size];
  memcpy(Data, &data, Size);
  Next = nullptr;
}

FieldClass::FieldClass(const char* id, unsigned short data) {
  strncpy(ID, id, sizeof(ID));
  DataType = TYPE_UNSIGNED_SHORT;
  Size = sizeof(data);
  Data = new char[Size];
  memcpy(Data, &data, Size);
  Next = nullptr;
}

FieldClass::FieldClass(const char* id, long data) {
  strncpy(ID, id, sizeof(ID));
  DataType = TYPE_LONG;
  Size = sizeof(data);
  Data = new char[Size];
  memcpy(Data, &data, Size);
  Next = nullptr;
}

FieldClass::FieldClass(const char* id, unsigned long data) {
  strncpy(ID, id, sizeof(ID));
  DataType = TYPE_UNSIGNED_LONG;
  Size = sizeof(data);
  Data = new char[Size];
  memcpy(Data, &data, Size);
  Next = nullptr;
}

FieldClass::FieldClass(const char* id, const char* data) {
  strncpy(ID, id, sizeof(ID));
  DataType = TYPE_STRING;
  Size = static_cast<unsigned short>(strlen(data) + 1);
  Data = new char[Size];
  memcpy(Data, data, Size);
  Next = nullptr;
}

FieldClass::FieldClass(const char* id, void* data, int length) {
  strncpy(ID, id, sizeof(ID));
  DataType = TYPE_CHUNK;
  Size = static_cast<unsigned short>(length);
  Data = new char[Size];
  memcpy(Data, data, Size);
  Next = nullptr;
}

/**************************************************************************
 * PACKETCLASS::HOST_TO_NET_FIELD -- Converts host field to net format    *
 *                                                                        *
 * INPUT:		FIELD 	* to the data field we need to convert
 **
 *                                                                        *
 * OUTPUT:     none
 **
 *                                                                        *
 * HISTORY:                                                               *
 *   04/22/1996 PWG : Created.                                            *
 *========================================================================*/
void FieldClass::Host_To_Net() {
  //
  // Before we convert the data type, we should convert the actual data
  //  sent.
  //
  switch (DataType) {
    case TYPE_CHAR:
    case TYPE_UNSIGNED_CHAR:
    case TYPE_STRING:
    case TYPE_CHUNK:
      break;

    case TYPE_SHORT:
    case TYPE_UNSIGNED_SHORT:
      *static_cast<unsigned short*>(Data) =
          htons(*static_cast<unsigned short*>(Data));
      break;

    case TYPE_LONG:
    case TYPE_UNSIGNED_LONG:
      *static_cast<unsigned long*>(Data) =
          htonl(*static_cast<unsigned long*>(Data));
      break;

    //
    // Might be good to insert some type of error message here for unknown
    //   datatypes -- but will leave that for later.
    //
    default:
      break;
  }
  //
  // Finally convert over the data type and the size of the packet.
  //
  DataType = htons(DataType);
  Size = htons(Size);
}
/**************************************************************************
 * PACKETCLASS::NET_TO_HOST_FIELD -- Converts net field to host format    *
 *                                                                        *
 * INPUT:		FIELD 	* to the data field we need to convert
 **
 *                                                                        *
 * OUTPUT:     none
 **
 *                                                                        *
 * HISTORY:                                                               *
 *   04/22/1996 PWG : Created.                                            *
 *========================================================================*/
void FieldClass::Net_To_Host() {
  //
  // Convert the variables to host order.  This needs to be converted so
  // the switch statement does compares on the data that follows.
  //
  Size = ntohs(Size);

  DataType = ntohs(DataType);

  //
  // Before we convert the data type, we should convert the actual data
  //  sent.
  //
  switch (DataType) {
    case TYPE_CHAR:
    case TYPE_UNSIGNED_CHAR:
    case TYPE_STRING:
    case TYPE_CHUNK:
      break;

    case TYPE_SHORT:
    case TYPE_UNSIGNED_SHORT:
      *static_cast<unsigned short*>(Data) =
          ntohs(*static_cast<unsigned short*>(Data));
      break;

    case TYPE_LONG:
    case TYPE_UNSIGNED_LONG:
      *static_cast<unsigned long*>(Data) =
          ntohl(*static_cast<unsigned long*>(Data));
      break;

    //
    // Might be good to insert some type of error message here for unknown
    //   datatypes -- but will leave that for later.
    //
    default:
      break;
  }
}
