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

/* $Header: /counterstrike/NULLMGR.CPP 2     3/07/97 6:40p Steve_tall $ */
/***************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : NULLMGR.CPP                              *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : April 5, 1995
 **
 *                                                                         *
 *                  Last Update : May 1, 1995 [BRR]                        *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   NullModemClass::NullModemClass -- class constructor                   *
 *   NullModemClass::~NullModemClass -- class destructor                   *
 *   NullModemClass::Port_Device -- names the device for a port            *
 *   NullModemClass::Open -- opens the port & creates the connection       *
 ** NullModemClass::Send_Message -- sends a message
 ** NullModemClass::Get_Message -- polls the Queue for a message
 ** NullModemClass::Service -- main polling routine
 ** NullModemClass::Num_Send -- Returns # of unACK'd send entries
 ** NullModemClass::Num_Receive -- Returns # entries in the receive queue *
 *   NullModemClass::Response_Time -- Returns Queue's avg response time    *
 *   NullModemClass::Reset_Response_Time -- Resets response time computatio*
 *   NullModemClass::Oldest_Send -- Returns ptr to oldest unACK'd send buf *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "engine/net/nullmgr.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "absl/log/log.h"
#include "base/aligned_buffer.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/unaligned.h"
#include "engine/net/combuf.h"
#include "engine/net/connect.h"
#include "engine/net/nullconn.h"
#include "engine/net/serial/modemreg.h"
#include "engine/net/serial/wincomm.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace engine::net {

/***************************************************************************
 * NullModemClass::NullModemClass -- class constructor                     *
 *                                                                         *
 * INPUT:                                                                  *
 *		numsend			# desired entries for the send queue
 ** numreceive		# desired entries for the receive queue
 ** maxlen			application's max packet length
 ** magicnum			application-specific magic # (so we don't
 ** accidentally end up talking to another one of our own	* products using
 *the same protocol)							*
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
NullModemClass::NullModemClass(int numsend, int numreceive, int maxlen,
                               uint16_t magicnum,
                               NullModemConnClass::PacketDiverter divert)
    : MaxLen(maxlen),
      NumSend(numsend),
      NumReceive(numreceive),
      MagicNum(magicnum),
      Divert(divert) {
  /*------------------------------------------------------------------------
  Init Port to NULL; we haven't opened Greenleaf yet.
  ------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
  Init timing parameters
  ------------------------------------------------------------------------*/

} /* end of NullModemClass */

/***************************************************************************
 * NullModemClass::~NullModemClass -- class destructor                     *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
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
NullModemClass::~NullModemClass() {
  Delete_Connection();

} /* end of ~NullModemClass */

/***************************************************************************
 * NullModemClass::Port_Device -- names the device for a port              *
 *                                                                         *
 * INPUT:                                                                  *
 *		port			0x3f8, 0x2f8, 0x3e8, 0x2e8 for COM1-4,
 * or 1 for a modem   * modem_name	the modem's name, when port is 1
 *                  * registry		where the modem's registry entry is left
 *                *
 *                                                                         *
 * OUTPUT:                                                                 *
 *		the device to open, or nullopt for an unknown port     *
 *                                                                         *
 *=========================================================================*/
std::optional<std::string> NullModemClass::Port_Device(
    const int port, const char* modem_name,
    std::unique_ptr<ModemRegistryEntryClass>& registry) {
  switch (port) {
    case 0x3f8:
      return "COM1";

    case 0x2f8:
      return "COM2";

    case 0x3e8:
      return "COM3";

    case 0x2e8:
      return "COM4";

    case 1: {
      std::unique_ptr<ModemRegistryEntryClass> found;
      std::string device = modem_name;
      for (int i = 0; i < 10; i++) {
        auto entry = std::make_unique<ModemRegistryEntryClass>(i);
        if (entry->Get_Modem_Name() &&
            (std::string_view(modem_name) == entry->Get_Modem_Name())) {
          device = entry->Get_Modem_Device_Name();
          found = std::move(entry);
          break;
        }
      }
      registry = std::move(found);
      return device;
    }

    default:
      return std::nullopt;
  }
}

/***************************************************************************
 * NullModemClass::Open -- opens the port & creates the connection
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		device			the device to open; see Port_Device()
 *                * baud				300, 1200, 9600, etc
 ** parity			'O' (odd), 'E' (even), 'N' (none), 'S' (space),
 ** 'M' (mark)
 ** wordlength		5, 6, 7, or 8
 ** stopbits			1 or 2
 ** flowcontrol		1 for hardware flow control
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *   10/9/1996  ST : Modified to take device name in win32                 *
 *=========================================================================*/
int NullModemClass::Open(const char* device, int baud, char parity, int wordlen,
                         int stopbits, int flowcontrol) {
#ifdef _WIN32
  // Make sure the port is closed before we start
  if (PortHandle) {
    CloseHandle(PortHandle);
    PortHandle = NULL;
  }
#endif

  if (!Connection) {
    /*------------------------------------------------------------------------
    Init our Connection
    ------------------------------------------------------------------------*/
    Connection = std::make_unique<NullModemConnClass>(NumSend, NumReceive,
                                                      MaxLen, MagicNum, Divert);

    Connection->Set_Retry_Delta(RetryDelta);
    Connection->Set_Max_Retries(MaxRetries);
    Connection->Set_TimeOut(Timeout);

    /*---------------------------------------------------------------------
    Allocate our packet parsing buffer; make it the same # of packets as the
    # of receive queue entries the application has requested.  Use the
    "Actual" maximum packet size, given from the connection; this allows for
    both headers that get added to the packet.
    ---------------------------------------------------------------------*/
    RXSize = Connection->Actual_Max_Packet() * NumReceive;
    RXBuf.resize(base::ToSize(RXSize));

    // new char[] provides alignment for the packet headers stored at its base.
    BuildBuf.resize(base::ToSize(MaxLen));

    EchoBuf.resize(base::ToSize(EchoSize));
  }

  RXCount = 0;
  EchoCount = 0;

  /*------------------------------------------------------------------------
  This call allocates all necessary queue buffers
  ------------------------------------------------------------------------*/

  /*
  ** Create a new modem class for our com port
  */
  if (!SerialPort) {
    SerialPort = std::make_unique<WinModemClass>();
  }

  /*
  ** Shift up the baud rate to sensible values
  */
  //	if (baud == 14400) baud = 19200;
  //	if (baud == 28800) baud = 38400;

  PortHandle = SerialPort->Serial_Port_Open(device, baud, parity, wordlen,
                                            stopbits, flowcontrol);
  if (PortHandle == nullptr) {
    Shutdown();
    return 0;
  }

  Connection->Init(PortHandle);

  NumConnections = 1;

  return 1;
}

/***********************************************************************************************
 * NMC::Num_Connections -- returns NumConnections member *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   NumConnections *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 11:44AM ST : Documented / Win32 support *
 *=============================================================================================*/
int NullModemClass::Num_Connections() { return NumConnections; }

/***********************************************************************************************
 * NMC::Delete_Connection -- removes the connection *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   true *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 11:44AM ST : Documented / Win32 support *
 *=============================================================================================*/
bool NullModemClass::Delete_Connection() {
  Connection.reset();

  if (!RXBuf.empty()) {
    RXBuf.clear();
  }

  if (!BuildBuf.empty()) {
    BuildBuf.clear();
  }

  if (!EchoBuf.empty()) {
    EchoBuf.clear();
  }

  NumConnections = 0;

  return true;
} /* end of Delete_Connection */

/***********************************************************************************************
 * NMC::Init_Send_Queue -- Initialises the connections send queue *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   true *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 11:46AM ST : Documented / Win32 support *
 *=============================================================================================*/
bool NullModemClass::Init_Send_Queue() {
  /*------------------------------------------------------------------------
  Init the send queue
  ------------------------------------------------------------------------*/
  if (Connection) {
    Connection->Queue->Init_Send_Queue();
  }

  return true;
}

/***********************************************************************************************
 * NullModemClass::ShutDown -- Closes serial port and removes the connection *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 11:43AM ST : Documented / Win32 support *
 *=============================================================================================*/
void NullModemClass::Shutdown() {
  if (PortHandle && SerialPort) {
    SerialPort->Serial_Port_Close();
    SerialPort.reset();
    PortHandle = nullptr;
    Delete_Connection();
  }
} /* end of Shutdown */

/***************************************************************************
 * NullModemClass::Set_Timing -- sets timing for all connections
 **
 *                                                                         *
 * This will set the timing parameters.  This allows an application to * measure
 *the Response_Time while running, and adjust timing accordingly.	*
 *                                                                         *
 * INPUT:                                                                  *
 *		retrydelta	value to set for retry delta
 ** maxretries	value to set for max # retries
 ** timeout		value to set for connection timeout
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
 *   08/07/1995 DRD : Created.                                             *
 *=========================================================================*/
void NullModemClass::Set_Timing(int32_t retrydelta, int32_t maxretries,
                                int32_t timeout) {
  RetryDelta = retrydelta;
  MaxRetries = maxretries;
  Timeout = timeout;

  Connection->Set_Retry_Delta(RetryDelta);
  Connection->Set_Max_Retries(MaxRetries);
  Connection->Set_TimeOut(Timeout);

} /* end of Set_Timing */

/***************************************************************************
 * NullModemClass::Send_Message -- sends a message
 **
 *                                                                         *
 * For clarity's sake, here's what happens to the buffer passed in:
 **
 * - It gets passed to the Connection's Send_Packet() routine
 **
 * - The CommHeaderType header gets tacked onto it
 **
 * - The resulting buffer gets added to the Connection's Send Queue
 **
 * - When Service() determines that it needs to send the data, it
 ** copies the entire packet (CommHeaderType and all) into its local * SendBuf,
 *adds the packet start ID, length, and CRC, then sends it out.*
 *                                                                         *
 * The ack_req argument will almost always be '1' (the default).  The only
 ** reason to use 0 is if you don't know whether the other system is
 ** ready or not, so you have to periodically send out a query packet, * and
 *wait for a response.  (Using the connection's built-in retry * system would
 *just blast out useless data if the other system isn't		* even there.)
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer to send
 ** buflen		length of buffer
 ** ack_req		1 = ACK is required; 0 = not
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK; 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int NullModemClass::Send_Message(std::span<const std::byte> buf, int buflen,
                                 int ack_req) {
  if (NumConnections == 0) {
    return 0;
  }

  const int rc = Connection->Send_Packet(buf, buflen, ack_req);
  if (!rc) {
    DLOG(WARNING) << "Serial send queue overflow; packet dropped";
  }

  return rc;

} /* end of Send_Message */

/***************************************************************************
 * NullModemClass::Get_Message -- polls the Queue for a message
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer to store message in
 ** buflen		ptr filled in with length of message
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = message was received; 0 = wasn't
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int NullModemClass::Get_Message(std::span<std::byte> buf, int* buflen) {
  if (NumConnections == 0) {
    return 0;
  }
  return Connection->Get_Packet(buf, buflen);
}

/***************************************************************************
 * NullModemClass::Service -- main polling routine
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = connection has gone bad
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *   8/2/96     ST : Win32 support                                         *
 *=========================================================================*/
int NullModemClass::Service() {
  int i = 0;                  // loop counter
  SerialHeaderType header{};  // decoded packet start, length
  SerialCRCType crc{};        // decoded packet CRC
  const char moredata = 0;

  if (NumConnections == 0) {
    return 0;
  }

  RXCount += WinModemClass::Read_From_Serial_Port(
      std::span(RXBuf).subspan(base::ToSize(RXCount)).data(), RXSize - RXCount);

  // minimum packet size

  if (RXCount < static_cast<int>(PACKET_SERIAL_OVERHEAD_SIZE) + 1) {
    return Connection->Service();
  }

  /*------------------------------------------------------------------------
  Now scan the buffer for the start of a packet.
  ------------------------------------------------------------------------*/
  int pos = -1;  // current position in RXBuf
  for (i = 0; i <= RXCount - static_cast<int>(sizeof(int16_t)); i++) {
    if (base::ReadUnaligned<uint16_t>(std::as_writable_bytes(std::span(RXBuf))
                                          .subspan(base::ToSize(i))) ==
        PACKET_SERIAL_START) {
      pos = i;
      break;
    }
  }

  /*------------------------------------------------------------------------
  No start code was found; throw away all bytes except the last few, and
  return.
  ------------------------------------------------------------------------*/
  if (pos == -1) {
    // Smart_Printf( "No magic number found \n" );
    /*.....................................................................
    move the remaining, un-checked bytes to the start of the buffer
    .....................................................................*/
    base::MoveBytes(
        std::as_writable_bytes(std::span(RXBuf)),
        std::as_writable_bytes(std::span(RXBuf)).subspan(base::ToSize(i)),
        sizeof(int16_t) - 1);
    RXCount = sizeof(int16_t) - 1;
    return Connection->Service();
  }

  /*------------------------------------------------------------------------
  Check to see if there are enough bytes for the header to be decoded
  ------------------------------------------------------------------------*/
  if (std::cmp_less(RXCount - pos, sizeof(SerialHeaderType))) {
    base::MoveBytes(
        std::as_writable_bytes(std::span(RXBuf)),
        std::as_writable_bytes(std::span(RXBuf)).subspan(base::ToSize(pos)),
        base::ToSize(RXCount - pos));
    RXCount -= pos;
    return Connection->Service();
  }

  /*------------------------------------------------------------------------
  A start code was found; check the packet's length & CRC
  ------------------------------------------------------------------------*/
  header = base::ReadUnaligned<SerialHeaderType>(
      std::as_writable_bytes(std::span(RXBuf)).subspan(base::ToSize(pos)));

  /*------------------------------------------------------------------------
  If we lost a byte in the length, we may end up waiting a very long time
  for the buffer to get to the right length; check the verify value to
  make sure this didn't happen.
  ------------------------------------------------------------------------*/
  if (header.MagicNumber2 != PACKET_SERIAL_VERIFY) {
    // Smart_Printf( "Verify failed\n");
    //		Hex_Dump_Data(
    //(std::as_writable_bytes(std::span(RXBuf)).subspan(base::ToSize(pos))),
    // PACKET_SERIAL_OVERHEAD_SIZE );

    pos += sizeof(int16_t);  // throw away the bogus start code
    base::MoveBytes(
        std::as_writable_bytes(std::span(RXBuf)),
        std::as_writable_bytes(std::span(RXBuf)).subspan(base::ToSize(pos)),
        base::ToSize(RXCount - pos));
    RXCount -= pos;
    return Connection->Service();
  }

  const uint16_t length = header.Length;

  /*------------------------------------------------------------------------
  Special case: if the length comes out too long for us to process:
  - Assume the packet is bad
  - Throw away the bogus packet-start code
  - Return;  we'll search for another packet-start code next time.
  ------------------------------------------------------------------------*/
  if (std::cmp_greater(length, MaxLen)) {
    // Smart_Printf( "length too lonnng %d, max %d \n", length, MaxLen );

    pos += sizeof(int16_t);  // throw away the bogus start code
    base::MoveBytes(
        std::as_writable_bytes(std::span(RXBuf)),
        std::as_writable_bytes(std::span(RXBuf)).subspan(base::ToSize(pos)),
        base::ToSize(RXCount - pos));
    RXCount -= pos;
    return Connection->Service();
  }

  /*------------------------------------------------------------------------
  If the entire packet isn't stored in our buffer, copy the remaining bytes
  to the front of the buffer & return.
  ------------------------------------------------------------------------*/
  if (pos + length + static_cast<int>(PACKET_SERIAL_OVERHEAD_SIZE) > RXCount) {
    if (moredata) {
      // Smart_Printf( "waiting for more data %d, pos = %d \n", ((length +
      // PACKET_SERIAL_OVERHEAD_SIZE) - (RXCount - pos)), pos );
    }

    if (pos) {
      base::MoveBytes(
          std::as_writable_bytes(std::span(RXBuf)),
          std::as_writable_bytes(std::span(RXBuf)).subspan(base::ToSize(pos)),
          base::ToSize(RXCount - pos));
      RXCount -= pos;
    }
    return Connection->Service();
  }

  /*------------------------------------------------------------------------
  Now grab the CRC value in the packet, & compare it to the CRC value
  computed from the actual data.  If they don't match, throw away the bogus
  start-code, move the rest to the front of the buffer, & return.
  We'll continue parsing this data when we're called next time.
  ------------------------------------------------------------------------*/
  crc = base::ReadUnaligned<SerialCRCType>(
      std::as_writable_bytes(std::span(RXBuf))
          .subspan(base::ToSize(pos) + sizeof(SerialHeaderType) + length));
  if (NullModemConnClass::Compute_CRC(
          std::as_writable_bytes(std::span(RXBuf))
              .subspan(base::ToSize(pos) + sizeof(SerialHeaderType)),
          length) != crc.SerialCRC) {
    DLOG(WARNING) << "Serial packet failed its CRC check; packet dropped";

    // Smart_Printf( "CRC check failed for packet of length %d \n", length );

    //		if (length < 100) {
    //			Hex_Dump_Data(
    //(std::as_writable_bytes(std::span(RXBuf)).subspan(base::ToSize(pos))),
    //(PACKET_SERIAL_OVERHEAD_SIZE + length) );
    //		}

    pos += sizeof(int16_t);  // throw away the bogus start code
    base::MoveBytes(
        std::as_writable_bytes(std::span(RXBuf)),
        std::as_writable_bytes(std::span(RXBuf)).subspan(base::ToSize(pos)),
        base::ToSize(RXCount - pos));
    RXCount -= pos;
    return Connection->Service();
  }

  /*------------------------------------------------------------------------
  Give the new packet to the Connection to process.
  ------------------------------------------------------------------------*/
  if (!Connection->Receive_Packet(
          std::as_writable_bytes(std::span(RXBuf))
              .subspan(base::ToSize(pos) + sizeof(SerialHeaderType)),
          length)) {
    DLOG(WARNING) << "Serial receive queue overflow; packet lost";
  }

  /*------------------------------------------------------------------------
  Move all data past this packet to the front of the buffer.
  ------------------------------------------------------------------------*/
  pos += static_cast<int>(PACKET_SERIAL_OVERHEAD_SIZE) + length;
  base::MoveBytes(
      std::as_writable_bytes(std::span(RXBuf)),
      std::as_writable_bytes(std::span(RXBuf)).subspan(base::ToSize(pos)),
      base::ToSize(RXCount - pos));
  RXCount -= pos;

  /*------------------------------------------------------------------------
  Now, service the connection's Queue's; this will handle ACK & Retries.
  ------------------------------------------------------------------------*/
  return Connection->Service();

} /* end of Service */

/***************************************************************************
 * NullModemClass::Num_Send -- Returns # of unACK'd send entries
 **
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/01/1995 BRR : Created.                                             *
 *=========================================================================*/
int NullModemClass::Num_Send() {
  if (Connection) {
    return Connection->Queue->Num_Send();
  }
  return 0;

} /* end of Num_Send */

/***************************************************************************
 * NullModemClass::Num_Receive -- Returns # entries in the receive queue   *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/01/1995 BRR : Created.                                             *
 *=========================================================================*/
int NullModemClass::Num_Receive() {
  if (Connection) {
    return Connection->Queue->Num_Receive();
  }
  return 0;

} /* end of Num_Receive */

/***************************************************************************
 * NullModemClass::Response_Time -- Returns Queue's avg response time      *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/01/1995 BRR : Created.                                             *
 *=========================================================================*/
int32_t NullModemClass::Response_Time() {
  if (Connection) {
    return Connection->Queue->Avg_Response_Time();
  }
  return 0;

} /* end of Response_Time */

/***************************************************************************
 * NullModemClass::Reset_Response_Time -- Resets response time computation *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/01/1995 BRR : Created.                                             *
 *=========================================================================*/
void NullModemClass::Reset_Response_Time() {
  if (Connection) {
    Connection->Queue->Reset_Response_Time();
  }

} /* end of Reset_Response_Time */

/***************************************************************************
 * Oldest_Send -- Returns ptr to oldest unACK'd send buffer                *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/01/1995 BRR : Created.                                             *
 *=========================================================================*/
std::span<const std::byte> NullModemClass::Oldest_Send() {
  std::span<const std::byte> buf;

  for (int i = 0; i < Connection->Queue->Num_Send(); i++) {
    engine::net::SendQueueType* send_entry =
        Connection->Queue->Get_Send(i);  // ptr to send entry header
    if (send_entry) {
      auto* packet = base::AlignedObject<engine::net::CommHeaderType>(
          send_entry->Buffer.data());
      if (packet->Code == static_cast<unsigned char>(
                              engine::net::ConnectionClass::PACKET_DATA_ACK) &&
          send_entry->IsACK == 0) {
        buf = std::span(send_entry->Buffer)
                  .first(base::ToSize(send_entry->BufLen));
        break;
      }
    }
  }

  return buf;

} /* end of Oldest_Send */

}  // namespace engine::net
