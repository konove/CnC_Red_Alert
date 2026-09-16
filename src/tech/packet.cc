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
 *                    File Name : PACKET.CPP                               *
 *                                                                         *
 *                   Programmer : Philip W. Gorrow                         *
 *                                                                         *
 *                   Start Date : 04/22/96                                 *
 *                                                                         *
 *                  Last Update : April 24, 1996 [PWG]                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   *PacketClass::Find_Field -- Finds a field if it exists in the packets *
 *   Get_Field -- Find specified name and returns data                     *
 *   PacketClass::~PacketClass -- destroys a packet class be freeing list  *
 *   PacketClass::Add_Field -- Adds a FieldClass entry to head of packet li*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "tech/packet.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#include "base/buffer.h"
#include "base/numeric.h"
#include "port/unaligned.h"
#include "tech/field.h"

// htons/ntohs
#ifdef _WIN32
#define NOMINMAX
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

/**************************************************************************
 * PACKETCLASS::~PACKETCLASS -- destroys a packet class be freeing list   *
 *                                                                        *
 * INPUT:		none *
 *                                                                        *
 * OUTPUT:     none
 **
 *                                                                        *
 * HISTORY:                                                               *
 *   04/24/1996 PWG : Created.                                            *
 *========================================================================*/
PacketClass::~PacketClass() {
  FieldClass* next = nullptr;
  //
  // Loop through the entire field list and delete each entry.
  //
  for (FieldClass* current = Head; current; current = next) {
    next = current->Next;
    delete current;
  }
}

/**************************************************************************
 * PACKETCLASS::ADD_FIELD -- Adds a FieldClass entry to head of packet li *
 *                                                                        *
 * INPUT:		FieldClass * - a properly constructed field class entry.
 **
 *                                                                        *
 * OUTPUT:     none                                                       *
 *                                                                        *
 * HISTORY:                                                               *
 *   04/24/1996 PWG : Created.                                            *
 *========================================================================*/
void PacketClass::Add_Field(FieldClass* field) {
  field->Next = Head;
  Head = field;
}

/**************************************************************************
 * PACKETCLASS::PACKETCLASS -- Creates a Packet object from a COMMS packe *
 *                                                                        *
 * INPUT:                                                                 *
 *                                                                        *
 * OUTPUT:                                                                *
 *                                                                        *
 * WARNINGS:                                                              *
 *                                                                        *
 * HISTORY:                                                               *
 *   04/22/1996 PWG : Created.                                            *
 *========================================================================*/
PacketClass::PacketClass(std::span<const std::byte> curbuf)
    : Size(0), ID(0), Head(nullptr) {
  if (curbuf.size() < 4) {
    return;
  }
  Size = ntohs(port::ReadUnaligned<uint16_t>(curbuf));
  ID = static_cast<int16_t>(
      ntohs(port::ReadUnaligned<uint16_t>(curbuf.subspan(2))));
  if (Size < 4 || Size > curbuf.size()) {
    return;
  }
  curbuf = curbuf.first(Size).subspan(4);
  while (curbuf.size() >= FIELD_HEADER_SIZE) {
    auto* field = new FieldClass;
    base::CopyBytes(base::ObjectBytes(field->ID), curbuf, 4);
    field->DataType = port::ReadUnaligned<uint16_t>(curbuf.subspan(4));
    field->Size = port::ReadUnaligned<uint16_t>(curbuf.subspan(6));
    curbuf = curbuf.subspan(FIELD_HEADER_SIZE);
    const std::size_t size = ntohs(field->Size);
    const std::size_t pad = (4 - (size % 4)) % 4;
    if (size + pad > curbuf.size()) {
      delete field;
      return;
    }
    const int type = ntohs(field->DataType);
    if (((type == TYPE_CHAR || type == TYPE_UNSIGNED_CHAR) && size != 1) ||
        ((type == TYPE_SHORT || type == TYPE_UNSIGNED_SHORT) && size != 2) ||
        ((type == TYPE_LONG || type == TYPE_UNSIGNED_LONG) && size != 4)) {
      delete field;
      return;
    }
    field->Data.assign(curbuf.begin(),
                       curbuf.begin() + static_cast<std::ptrdiff_t>(size));
    curbuf = curbuf.subspan(size + pad);
    field->Net_To_Host();
    Add_Field(field);
  }
}

/**************************************************************************
 * CREATE_COMMS_PACKET -- Walks field list creating a packet              *
 *                                                                        *
 * INPUT:		short - the id of the packet so the server can identify
 *it * unsigned short & - the size of the packet returned here    *
 *                                                                        *
 * OUTPUT:     void * pointer to the linear packet data                   *
 *                                                                        *
 * WARNINGS: 	This routine allocates memory that the user is responsible *
 *  				for freeing.
 **
 *                                                                        *
 * HISTORY:                                                               *
 *   04/22/1996 PWG : Created.                                            *
 *========================================================================*/
char* PacketClass::Create_Comms_Packet(int& size) {

  //
  // Size starts at four because that is the size of the packet header.
  //
  size = 4;

  //
  // Take a quick spin through and calculate the size of the packet we
  //   are building.
  //
  for (FieldClass* current = Head; current; current = current->Next) {
    size +=
        static_cast<uint16_t>(FIELD_HEADER_SIZE);  // add in packet header size
    size += current->Size;       // add in data size
    size +=
        (4 - (size % 4)) % 4;  // add in pad value to dword align next packet
  }

  //
  // Now that we know the size allocate a buffer big enough to hold the
  // packet.
  //
  char* retval = new char[base::ToSize(size)];
  // retval owns exactly size bytes allocated immediately above.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  auto curbuf = std::as_writable_bytes(std::span(retval, base::ToSize(size)));

  //
  // write the size into the packet header
  //
  port::WriteUnaligned(curbuf, htons(static_cast<uint16_t>(size)));
  curbuf = curbuf.subspan(sizeof(uint16_t));
  port::WriteUnaligned(curbuf, htons(static_cast<uint16_t>(ID)));
  curbuf = curbuf.subspan(sizeof(int16_t));

  //
  // Ok now that the actual header information has been written we need to write
  // out field information.
  //
  for (FieldClass* current = Head; current; current = current->Next) {
    //
    // Temporarily convert the packet to net format (this saves alot of
    //   effort, and seems safe...)
    //
    current->Host_To_Net();

    //
    // Copy the adjusted header into the buffer and then advance the buffer
    //
    base::CopyBytes(curbuf, base::ObjectBytes(current->ID), 4);
    port::WriteUnaligned(curbuf.subspan(4), current->DataType);
    port::WriteUnaligned(curbuf.subspan(6), current->Size);
    curbuf = curbuf.subspan(FIELD_HEADER_SIZE);

    //
    // Copy the data into the buffer and then advance the buffer
    //
    base::CopyBytes(curbuf, current->Data, ntohs(current->Size));
    curbuf = curbuf.subspan(ntohs(current->Size));

    //
    // Finally take care of any pad bytes by setting them to 0
    //
    const int pad = (4 - (ntohs(current->Size) % 4)) % 4;

    //
    //	If there is any pad left over, make sure you memset it
    // to zeros, so it looks like a pad.
    //
    if (pad) {
      std::ranges::fill(curbuf.first(base::ToSize(pad)), std::byte{0});
      curbuf = curbuf.subspan(base::ToSize(pad));
    }

    current->Net_To_Host();
  }
  return retval;
}

/**************************************************************************
 * PACKETCLASS::FIND_FIELD -- Finds a field if it exists in the packets   *
 *                                                                        *
 * INPUT:		char *  - the id of the field we are looking for.
 **
 *                                                                        *
 * OUTPUT:     FieldClass * pointer to the field class                    *
 *                                                                        *
 * HISTORY:                                                               *
 *   04/23/1996 PWG : Created.                                            *
 *========================================================================*/
FieldClass* PacketClass::Find_Field(const char* id) {
  for (FieldClass* current = Head; current; current = current->Next) {
    if (std::string_view(id).substr(0, 4) ==
        std::string_view(current->ID, static_cast<std::size_t>(
                                          std::ranges::find(current->ID, '\0') -
                                          std::begin(current->ID)))) {
      return current;
    }
  }
  return nullptr;
}

/**************************************************************************
 * GET_FIELD -- Find specified name and returns data                      *
 *                                                                        *
 * INPUT:		char *   - the id of the field that holds the data.
 ** char &   - the reference to store the data into *
 *                                                                        *
 * OUTPUT:		true if the field was found, false if it was not.
 **
 *                                                                        *
 * WARNINGS:	The data reference is not changed if the field is not * found.
 **
 *                                                                        *
 * HISTORY:                                                               *
 *   04/23/1996 PWG : Created.                                            *
 *========================================================================*/
bool PacketClass::Get_Field(const char* id, char& data) {
  const FieldClass* field = Find_Field(id);
  if (field) {
    data = port::ReadUnaligned<char>(field->Data);
  }
  return field != nullptr;
}

/**************************************************************************
 * GET_FIELD -- Find specified name and returns data                      *
 *                                                                        *
 * INPUT:		char *   - the id of the field that holds the data.
 ** unsigned char &   - the reference to store the data into	  *
 *                                                                        *
 * OUTPUT:		true if the field was found, false if it was not.
 **
 *                                                                        *
 * WARNINGS:	The data reference is not changed if the field is not * found.
 **
 *                                                                        *
 * HISTORY:                                                               *
 *   04/23/1996 PWG : Created.                                            *
 *========================================================================*/
bool PacketClass::Get_Field(const char* id, unsigned char& data) {
  const FieldClass* field = Find_Field(id);
  if (field) {
    data = port::ReadUnaligned<unsigned char>(field->Data);
  }
  return field != nullptr;
}

/**************************************************************************
 * GET_FIELD -- Find specified name and returns data                      *
 *                                                                        *
 * INPUT:		char *   - the id of the field that holds the data.
 ** short &   - the reference to store the data into	        *
 *                                                                        *
 * OUTPUT:		true if the field was found, false if it was not.
 **
 *                                                                        *
 * WARNINGS:	The data reference is not changed if the field is not * found.
 **
 *                                                                        *
 * HISTORY:                                                               *
 *   04/23/1996 PWG : Created.                                            *
 *========================================================================*/
bool PacketClass::Get_Field(const char* id, int16_t& data) {
  const FieldClass* field = Find_Field(id);
  if (field) {
    data = port::ReadUnaligned<int16_t>(field->Data);
  }
  return field != nullptr;
}

/**************************************************************************
 * GET_FIELD -- Find specified name and returns data                      *
 *                                                                        *
 * INPUT:		char *   - the id of the field that holds the data.
 ** unsigned short &   - the reference to store the data into  *
 *                                                                        *
 * OUTPUT:		true if the field was found, false if it was not.
 **
 *                                                                        *
 * WARNINGS:	The data reference is not changed if the field is not * found.
 **
 *                                                                        *
 * HISTORY:                                                               *
 *   04/23/1996 PWG : Created.                                            *
 *========================================================================*/
bool PacketClass::Get_Field(const char* id, uint16_t& data) {
  const FieldClass* field = Find_Field(id);
  if (field) {
    data = port::ReadUnaligned<uint16_t>(field->Data);
  }
  return field != nullptr;
}

/**************************************************************************
 * GET_FIELD -- Find specified name and returns data                      *
 *                                                                        *
 * INPUT:		char *   - the id of the field that holds the data.
 ** long &   - the reference to store the data into  			  *
 *                                                                        *
 * OUTPUT:		true if the field was found, false if it was not.
 **
 *                                                                        *
 * WARNINGS:	The data reference is not changed if the field is not * found.
 **
 *                                                                        *
 * HISTORY:                                                               *
 *   04/23/1996 PWG : Created.                                            *
 *========================================================================*/
bool PacketClass::Get_Field(const char* id, int32_t& data) {
  const FieldClass* field = Find_Field(id);
  if (field) {
    data = port::ReadUnaligned<int32_t>(field->Data);
  }
  return field != nullptr;
}

/**************************************************************************
 * GET_FIELD -- Find specified name and returns data as a string          *
 *                                                                        *
 * INPUT:		char *   - the id of the field that holds the data.
 ** char *   - the string to store the data into
 **
 *                                                                        *
 * OUTPUT:		true if the field was found, false if it was not.
 **
 *                                                                        *
 * WARNINGS:	The string is not changed if the field is not found.  It   *
 *					is assumed that the string variabled
 *specified by the      * pointer is large enough to hold the data.
 **
 *                                                                        *
 * HISTORY:                                                               *
 *   04/23/1996 PWG : Created.                                            *
 *========================================================================*/
bool PacketClass::Get_Field(const char* id, std::span<char> data) {
  const FieldClass* field = Find_Field(id);
  if (field) {
    const auto terminator = std::ranges::find(field->Data, std::byte{0});
    const auto count =
        static_cast<std::size_t>(terminator - field->Data.begin());
    if (!data.empty()) {
      const auto copied = std::min(count, data.size() - 1);
      for (std::size_t i = 0; i < copied; ++i) {
        data[i] = static_cast<char>(field->Data[i]);
      }
      data[copied] = '\0';
    }
  }
  return field != nullptr;
}

/**************************************************************************
 * GET_FIELD -- Find specified name and returns data                      *
 *                                                                        *
 * INPUT:		char *   - the id of the field that holds the data.
 ** unsigned long &   - the reference to store the data into   *
 *                                                                        *
 * OUTPUT:		true if the field was found, false if it was not.
 **
 *                                                                        *
 * WARNINGS:	The data reference is not changed if the field is not * found.
 **
 *                                                                        *
 * HISTORY:                                                               *
 *   04/23/1996 PWG : Created.                                            *
 *========================================================================*/
bool PacketClass::Get_Field(const char* id, uint32_t& data) {
  const FieldClass* field = Find_Field(id);
  if (field) {
    data = port::ReadUnaligned<uint32_t>(field->Data);
  }
  return field != nullptr;
}

/**************************************************************************
 * GET_FIELD -- Find specified name and returns data                      *
 *                                                                        *
 * INPUT:		char *   - the id of the field that holds the data.
 ** void * - the reference to store the data into                * int    - the
 *length of the buffer passed in                  *
 *                                                                        *
 * OUTPUT:		true if the field was found, false if it was not.
 **
 *                                                                        *
 * WARNINGS:	The data reference is not changed if the field is not * found.
 **
 *                                                                        *
 * HISTORY:                                                               *
 *   6/4/96 4:46PM ST : Created                                           *
 *========================================================================*/
bool PacketClass::Get_Field(const char* id, std::span<std::byte> data,
                            int& length) {
  const FieldClass* field = Find_Field(id);
  if (field) {
    base::CopyBytes(
        data, field->Data,
        std::min({data.size(), field->Data.size(), base::ToSize(length)}));
    length = static_cast<int>(field->Size);
  }
  return field != nullptr;
}
