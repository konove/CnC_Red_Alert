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

/* $Header:   F:\projects\c&c\vcs\code\nullconn.cpv   1.10   16 Oct 1995
 * 16:51:36   JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : NULLCONN.CPP                             *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : April 5, 1995 *
 *                                                                         *
 *                  Last Update : April 20, 1995   [DRD]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions: * NullModemConnClass::NullModemConnClass -- class constructor *
 *   NullModemConnClass::~NullModemConnClass -- class destructor           *
 *   NullModemConnClass::Init -- hardware-dependent initialization
 ** NullModemConnClass::Send -- hardware-dependent packet sending
 ** NullModemConnClass::Compute_CRC -- computes CRC for given buffer *
 *   NullModemConnClass::Packet_Overhead_Size -- number of extra bytes     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "td/nullconn.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "absl/log/check.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "port/aligned_buffer.h"
#include "port/unaligned.h"
#include "sdllib/wincomm.h"
#include "td/connect.h"
#include "td/defines.h"
#include "td/externs.h"
#include "td/noseqcon.h"
#include "td/tcpip.h"

/***************************************************************************
 * NullModemConnClass::NullModemConnClass -- class constructor             *
 *                                                                         *
 * INPUT:                                                                  *
 *		numsend			desired # send queue entries
 ** numreceive		desired # send receive entries
 ** maxlen			max length of application's packets
 ** magicnum			application-defined magic # for the packets
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
NullModemConnClass::NullModemConnClass(int numsend, int numreceive, int maxlen,
                                       uint16_t magicnum)
    : NonSequencedConnClass(
          numsend, numreceive, maxlen, magicnum,
          60,  // Retry Delta Time
          -1,  // Max Retries (-1 means ignore this timeout parameter)
          1200),
      SendBuf(base::ToSize(Actual_Max_Packet()))  // Timeout: 20 seconds
{
  /*------------------------------------------------------------------------
  Pre-set the port value to NULL, so Send won't send until we've been Init'd
  ------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
  Allocate the Send Buffer; the parent constructor has set MaxPacketLen,
  so we can use it in our computation.
  ------------------------------------------------------------------------*/
  //	SendBuf = new char [MaxPacketLen + sizeof(int) * 3];
  // new char[] provides alignment for the packet headers stored at its base.

} /* end of NullModemConnClass */

/***************************************************************************
 * NullModemConnClass::~NullModemConnClass -- class destructor             *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
NullModemConnClass::~NullModemConnClass() =
    default; /* end of ~NullModemConnClass */

/***************************************************************************
 * NullModemConnClass::Init -- hardware-dependent initialization
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		port		GreenLeaf port handle
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
void NullModemConnClass::Init(HANDLE port_handle) {
  NonSequencedConnClass::Init();
  PortHandle = port_handle;

} /* end of Init */

/***************************************************************************
 * NullModemConnClass::Send -- hardware-dependent packet sending
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		port		GreenLeaf port handle
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int NullModemConnClass::Send(std::span<const std::byte> buf, int buflen) {
  if (buflen < 0 || base::ToSize(buflen) > buf.size() ||
      base::ToSize(buflen) + sizeof(SerialHeaderType) + sizeof(int) + 1 >
          SendBuf.size()) {
    return 0;
  }
  // int status;

  /*------------------------------------------------------------------------
  Error if we haven't been properly initialized
  ------------------------------------------------------------------------*/
  if (PortHandle == nullptr) {
    return 0;
  }

  /*------------------------------------------------------------------------
  Package the data into the Send Buffer
  ------------------------------------------------------------------------*/
  auto* header = port::AlignedObject<SerialHeaderType>(SendBuf.data());
  header->MagicNumber = PACKET_SERIAL_START;
  header->Length = static_cast<uint16_t>(buflen);
  header->MagicNumber2 = PACKET_SERIAL_VERIFY;

  int sendlen = static_cast<int>(sizeof(SerialHeaderType));
  base::CopyBytes(
      std::as_writable_bytes(std::span(SendBuf)).subspan(base::ToSize(sendlen)),
      buf, base::ToSize(buflen));
  sendlen += buflen;
  port::WriteUnaligned(
      std::as_writable_bytes(std::span(SendBuf)).subspan(base::ToSize(sendlen)),
      Compute_CRC(buf, buflen));
  sendlen += static_cast<int>(sizeof(int));

  SendBuf.at(base::ToSize(sendlen)) = '\r';
  sendlen += 1;

  /*------------------------------------------------------------------------
  Send the data
  ------------------------------------------------------------------------*/
  // status =
#ifdef FORCE_WINSOCK
  if (Winsock.Get_Connected() || GameToPlay == GAME_INTERNET) {
    Winsock.Write(std::as_bytes(std::span(SendBuf)), sendlen);
  } else {
    SerialPort->Write_To_Serial_Port(SendBuf.data(), sendlen);
  }
#else
  SerialPort->Write_To_Serial_Port(SendBuf.data(), sendlen);
#endif  // WINSOCK

  // if ( status == ASSUCCESS ) {
  return 1;
  //} else {
  // Smart_Printf( "Write Buffer status %d, Port->status %d, sendlen %d \n",
  // status, Port->status, sendlen );
  //	return(false);
  //}
}

/***************************************************************************
 * NullModemConnClass::Compute_CRC -- computes CRC for given buffer
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf		buffer to compute CRC for
 ** buflen	length of buffer in bytes
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int NullModemConnClass::Compute_CRC(std::span<const std::byte> buf,
                                    int buflen) {
  CHECK_GE(buflen, 0);
  CHECK_LE(base::ToSize(buflen), buf.size());
  unsigned int hibit = 0;

  unsigned int sum = 0;
  for (int i = 0; i < buflen; i++) {
    if (sum & 0x80000000) {  // check hi bit to rotate into low bit
      hibit = 1;
    } else {
      hibit = 0;
    }

    sum <<= 1;
    sum +=
        hibit + std::to_integer<unsigned char>(base::At(buf, base::ToSize(i)));
  }

  return static_cast<int>(sum);
}

/***************************************************************************
 * NullModemConnClass::Packet_Overhead_Size -- number of extra bytes       *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		number of bytes used for communications only.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   04/20/1995 DRD : Created.                                             *
 *=========================================================================*/
int NullModemConnClass::Packet_Overhead_Size() {
  //
  // short for Null Modem Magic Number
  // short for Null Modem length of packet
  // int for Null Modem CRC check
  // CommHeaderType for Queued packets
  //

  return PACKET_SERIAL_OVERHEAD_SIZE + sizeof(CommHeaderType);

} /* end of Packet_Overhead_Size */
