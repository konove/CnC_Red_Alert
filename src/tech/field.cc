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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#include "base/buffer.h"
#include "port/unaligned.h"

// htons/htonl
#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

FieldClass::FieldClass(const char* id, char data)
    : DataType(TYPE_CHAR), Size(sizeof(data)), Data(Size), Next(nullptr) {
  std::ranges::copy(std::string_view(id).substr(0, sizeof(ID)), std::begin(ID));

  base::CopyBytes(Data, base::ObjectBytes(data), Size);
}

FieldClass::FieldClass(const char* id, unsigned char data)
    : DataType(TYPE_UNSIGNED_CHAR),
      Size(sizeof(data)),
      Data(Size),
      Next(nullptr) {
  std::ranges::copy(std::string_view(id).substr(0, sizeof(ID)), std::begin(ID));

  base::CopyBytes(Data, base::ObjectBytes(data), Size);
}

FieldClass::FieldClass(const char* id, int16_t data)
    : DataType(TYPE_SHORT), Size(sizeof(data)), Data(Size), Next(nullptr) {
  std::ranges::copy(std::string_view(id).substr(0, sizeof(ID)), std::begin(ID));

  base::CopyBytes(Data, base::ObjectBytes(data), Size);
}

FieldClass::FieldClass(const char* id, uint16_t data)
    : DataType(TYPE_UNSIGNED_SHORT),
      Size(sizeof(data)),
      Data(Size),
      Next(nullptr) {
  std::ranges::copy(std::string_view(id).substr(0, sizeof(ID)), std::begin(ID));

  base::CopyBytes(Data, base::ObjectBytes(data), Size);
}

FieldClass::FieldClass(const char* id, int32_t data)
    : DataType(TYPE_LONG), Size(sizeof(data)), Data(Size), Next(nullptr) {
  std::ranges::copy(std::string_view(id).substr(0, sizeof(ID)), std::begin(ID));

  base::CopyBytes(Data, base::ObjectBytes(data), Size);
}

FieldClass::FieldClass(const char* id, uint32_t data)
    : DataType(TYPE_UNSIGNED_LONG),
      Size(sizeof(data)),
      Data(Size),
      Next(nullptr) {
  std::ranges::copy(std::string_view(id).substr(0, sizeof(ID)), std::begin(ID));

  base::CopyBytes(Data, base::ObjectBytes(data), Size);
}

FieldClass::FieldClass(const char* id, const char* data)
    : DataType(TYPE_STRING),
      Size(static_cast<uint16_t>(std::string_view(data).size() + 1)),
      Data(Size),
      Next(nullptr) {
  std::ranges::copy(std::string_view(id).substr(0, sizeof(ID)), std::begin(ID));

  base::CopyBytes(Data, std::as_bytes(std::span(std::string_view(data))),
                  Size - 1);
}

FieldClass::FieldClass(const char* id, std::span<const std::byte> data)
    : DataType(TYPE_CHUNK),
      Size(static_cast<uint16_t>(data.size())),
      Data(Size),
      Next(nullptr) {
  std::ranges::copy(std::string_view(id).substr(0, sizeof(ID)), std::begin(ID));

  base::CopyBytes(Data, data, Size);
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
      port::WriteUnaligned(Data, htons(port::ReadUnaligned<uint16_t>(Data)));
      break;

    case TYPE_LONG:
    case TYPE_UNSIGNED_LONG:
      port::WriteUnaligned(Data, htonl(port::ReadUnaligned<uint32_t>(Data)));
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
      port::WriteUnaligned(Data, ntohs(port::ReadUnaligned<uint16_t>(Data)));
      break;

    case TYPE_LONG:
    case TYPE_UNSIGNED_LONG:
      port::WriteUnaligned(Data, ntohl(port::ReadUnaligned<uint32_t>(Data)));
      break;

    //
    // Might be good to insert some type of error message here for unknown
    //   datatypes -- but will leave that for later.
    //
    default:
      break;
  }
}
