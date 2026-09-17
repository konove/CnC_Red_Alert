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
 * Functions: * NullModemClass::NullModemClass -- class constructor *
 *   NullModemClass::~NullModemClass -- class destructor                   *
 *   NullModemClass::Init -- initialization
 ** NullModemClass::Send_Message -- sends a message
 ** NullModemClass::Get_Message -- polls the Queue for a message
 ** NullModemClass::Service -- main polling routine
 ** NullModemClass::Num_Send -- Returns # of unACK'd send entries
 ** NullModemClass::Num_Receive -- Returns # entries in the receive queue *
 *   NullModemClass::Response_Time -- Returns Queue's avg response time    *
 *   NullModemClass::Reset_Response_Time -- Resets response time computatio*
 *   NullModemClass::Oldest_Send -- Returns ptr to oldest unACK'd send buf *
 *   NullModemClass::Detect_Modem -- Detects and initializes the modem     *
 *   NullModemClass::Dial_Modem -- dials a number passed                   *
 *   NullModemClass::Answer_Modem -- waits for call and answers            *
 *   NullModemClass::Hangup_Modem -- hangs up the modem                    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "ra/nullmgr.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "port/aligned_buffer.h"
#include "port/safe_string.h"
#include "port/unaligned.h"
#include "ra/combuf.h"
#include "ra/connect.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/init.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/msgbox.h"
#include "ra/nullconn.h"
#include "ra/nulldlg.h"
#include "ra/session.h"
#include "ra/text_ids.h"
#include "ra/textbtn.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/modemreg.h"
#include "sdllib/wincomm.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/number_parse.h"

#ifdef _WIN32
#include <windows.h>

#endif


// the following line was taken from Greenleaf's <ibmkeys.h> <asciidef.h>
// because of other define conflicts

// #define ESC 27
// #define NOKEY 0xffff
#define INIT_COMMAND_RETRIES 2

// this time is in milliseconds

#define DEFAULT_TIMEOUT 1500

//
// the following is for a fix around a greenleaf bug
// where they do not check for the value of abortkey
// to determine whether or not they call the abort modem function.
//
extern "C" {
extern void (*AbortModemFunctionPtr)(int);
}

void (*NullModemClass::OrigAbortModemFunc)(int);

KeyNumType NullModemClass::Input;
GadgetClass* NullModemClass::Commands;  // button list

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
                               uint16_t magicnum)
    : MaxLen(maxlen),
      NumSend(numsend),
      NumReceive(numreceive),
      MagicNum(magicnum) {
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
 * NullModemClass::Init -- initialization
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		port				address
 ** irq				2-15
 ** dev_name			name of communications device (win32 only) *
 *		baud				300, 1200, 9600, etc
 ** parity			'O' (odd), 'E' (even), 'N' (none), 'S' (space),
 ** 'M' (mark)
 ** wordlength		5, 6, 7, or 8
 ** stopbits			1 or 2
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
int NullModemClass::Init(int port, int /*irq*/, char* dev_name, int baud,
                         char parity, int wordlen, int stopbits,
                         int flowcontrol) {
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
    Connection = new NullModemConnClass(NumSend, NumReceive, MaxLen, MagicNum);

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
    SerialPort = new WinModemClass;
  }

  /*
  ** Shift up the baud rate to sensible values
  */
  //	if (baud == 14400) baud = 19200;
  //	if (baud == 28800) baud = 38400;

  static char com_ids[9][5] = {"COM1", "COM2", "COM3", "COM4", "COM5",
                               "COM6", "COM7", "COM8", "COM9"};

  const char* device = nullptr;

  switch (port) {
    case 0x3f8:
      device = com_ids[0];
      break;

    case 0x2f8:
      device = com_ids[1];
      break;

    case 0x3e8:
      device = com_ids[2];
      break;

    case 0x2e8:
      device = com_ids[3];
      break;

    case 1:
      /*
      ** 1 is a special value. It means use the device name not the port
      *address.
      */
      device = dev_name;

      /*
      ** If we can match a registry entry with the device name then use that,
      *otherwise use
      ** the device name directly to open the port with.
      */
      if (ModemRegistry) {
        delete ModemRegistry;
        ModemRegistry = nullptr;
      }
      for (int i = 0; i < 10; i++) {
        ModemRegistry = new ModemRegistryEntryClass(i);
        if (ModemRegistry->Get_Modem_Name() &&
            (std::string_view(dev_name) == ModemRegistry->Get_Modem_Name())) {
          device = ModemRegistry->Get_Modem_Device_Name();
          break;
        }

        delete ModemRegistry;
        ModemRegistry = nullptr;
      }
      break;

    default:
      device = nullptr;
  }

  /*
  ** Open the com port
  */
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
  if (Connection) {
    delete Connection;
    Connection = nullptr;
  }

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
 * NMC::Detect_Port -- Checks that the specified com port exists *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to SerialSettingsType *
 *                                                                                             *
 * OUTPUT:   true if port is valid *
 *                                                                                             *
 * WARNINGS: Win32 version always returns true as win95 shouldnt allow us to
 *open the          * port if it doesnt exist or is in use by the mouse *
 *                                                                                             *
 * HISTORY: * 8/2/96 11:47AM ST : Documented / Win32 support *
 *=============================================================================================*/
DetectPortType NullModemClass::Detect_Port(SerialSettingsType* settings) {
  static char com_ids[9][5] = {"COM1", "COM2", "COM3", "COM4", "COM5",
                               "COM6", "COM7", "COM8", "COM9"};


  /*
  ** Create a new modem class for our com port
  */
  if (!SerialPort) {
    SerialPort = new WinModemClass;
  } else {
    SerialPort->Serial_Port_Close();
  }

  /*
  ** Shift up the baud rate to sensible values
  */
  const int baud = settings->Baud;
  //	if (baud == 14400) baud = 19200;
  //	if (baud == 28800) baud = 38400;

  /*
  ** Translate the port address into a usable device name
  */
  const char* device = nullptr;

  switch (settings->Port) {
    case 0x3f8:
      device = com_ids[0];
      break;

    case 0x2f8:
      device = com_ids[1];
      break;

    case 0x3e8:
      device = com_ids[2];
      break;

    case 0x2e8:
      device = com_ids[3];
      break;

    case 1:
      /*
      ** 1 is a special value. It means use the device name not the port
      *address.
      */
      device = settings->ModemName;

      /*
      ** If we can match a registry entry with the device name then use that,
      *otherwise use
      ** the device name directly to open the port with.
      */
      if (ModemRegistry) {
        delete ModemRegistry;
        ModemRegistry = nullptr;
      }
      for (int i = 0; i < 10; i++) {
        ModemRegistry = new ModemRegistryEntryClass(i);
        if (ModemRegistry->Get_Modem_Name() &&
            (std::string_view(device) == ModemRegistry->Get_Modem_Name())) {
          /*
          ** Got a match. Break out leaving the registry info intact.
          */
          device = ModemRegistry->Get_Modem_Device_Name();
          break;
        }

        delete ModemRegistry;
        ModemRegistry = nullptr;
      }
      break;

    default:
      return PORT_INVALID;
  }

  /*
  ** Open the com port
  */
  HANDLE porthandle = SerialPort->Serial_Port_Open(
      device, baud, 0, 8, 1, settings->HardwareFlowControl ? 1 : 0);

  if (porthandle == nullptr) {
    return PORT_INVALID;
  }

  SerialPort->Serial_Port_Close();
  return PORT_VALID;
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
    delete SerialPort;
    SerialPort = nullptr;
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
  int i = 0;                // loop counter
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
    if (port::ReadUnaligned<uint16_t>(std::as_writable_bytes(std::span(RXBuf))
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
  header = port::ReadUnaligned<SerialHeaderType>(
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
    //PACKET_SERIAL_OVERHEAD_SIZE );

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
#if (CONN_DEBUG)
    printf("length too lonnng\n");
#endif
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
  if (pos + length + static_cast<int>(PACKET_SERIAL_OVERHEAD_SIZE) >
        RXCount) {
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
  crc = port::ReadUnaligned<SerialCRCType>(
      std::as_writable_bytes(std::span(RXBuf))
          .subspan(base::ToSize(pos) + sizeof(SerialHeaderType) + length));
  if (NullModemConnClass::Compute_CRC(
          std::as_writable_bytes(std::span(RXBuf))
              .subspan(base::ToSize(pos) + sizeof(SerialHeaderType)),
          length) != crc.SerialCRC) {
    DLOG(WARNING) << "Serial packet failed its CRC check; packet dropped";

#if (CONN_DEBUG)
    printf("CRC check failed\n");
#endif
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
    SendQueueType* send_entry =
        Connection->Queue->Get_Send(i);  // ptr to send entry header
    if (send_entry) {
      auto* packet =
          port::AlignedObject<CommHeaderType>(send_entry->Buffer.data());
      if (packet->Code ==
              static_cast<unsigned char>(ConnectionClass::PACKET_DATA_ACK) &&
          send_entry->IsACK == 0) {
        buf = std::span(send_entry->Buffer)
                  .first(base::ToSize(send_entry->BufLen));
        break;
      }
    }
  }

  return buf;

} /* end of Oldest_Send */

/***************************************************************************
 * NullModemClass::Detect_Modem -- Detects and initializes the modem       *
 *                                                                         *
 * INPUT:                                                                  *
 *		settings		ptr to SerialSettings structure
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		-1				init string invalid
 ** 0				no modem found
 ** 1				modem found
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   06/02/1995 DRD : Created.                                             *
 *   8/2/96      ST : Added Win32 support                                  *
 *=========================================================================*/
int NullModemClass::Detect_Modem(SerialSettingsType* settings, bool reconnect) {
  /*------------------------------------------------------------------------
  Button Enumerations
  ------------------------------------------------------------------------*/
  int error_count = 0;

  int width = 0;
  int height = 0;  // dialog dimensions
  char buffer[80 * 3];

  /*
  ** Get resolution factor
  */
  //	int	factor = SeenBuff.Get_Width()/320;

  /*------------------------------------------------------------------------
  Determine the dimensions of the text to be used for the dialog box.
  These dimensions will control how the dialog box looks.
  ------------------------------------------------------------------------*/
  port::SafeCopy(buffer, Text_String(TXT_INITIALIZING_MODEM));

  Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack, kTpfText);
  const int lines =
      Format_Window_String(buffer, SeenBuff.Get_Height(), width, height);

  width = std::max(width, 180);
  width += 80;
  height += 80;

  const int x = (SeenBuff.Get_Width() - width) / 2;
  const int y = (SeenBuff.Get_Height() - height) / 2;

  /*------------------------------------------------------------------------
  Initialize
  ------------------------------------------------------------------------*/
  Set_Logic_Page(SeenBuff);

  /*------------------------------------------------------------------------
  Draw the dialog
  ------------------------------------------------------------------------*/
  Hide_Mouse();
  if (!reconnect) {
    Load_Title_Page(true);
  }

  Dialog_Box(x, y, width, height);
  Draw_Caption(TXT_NONE, x, y, width);

  if (lines == 1) {
    Fancy_Text_Print(buffer, x + (width / 2), y + 25,
                     GadgetClass::Get_Color_Scheme(), kTBlack,
                     kTpfText | TPF_CENTER);
  } else {
    Fancy_Text_Print(buffer, x + 40, y + 25, GadgetClass::Get_Color_Scheme(),
                     kTBlack, kTpfText);
  }

  Show_Mouse();

  /*
  ** OK, lets not mess about any more. Just turn on echo, verbose, and result
  *codes
  ** before we even begin. At least this way when we get an error later on we
  *have already
  ** removed all the steps we use to try and recover.
  ** The timeouts need to be quite small in case the modem is turned off.
  */

  /*
  ** Turn on result codes.
  */
  Send_Modem_Command("ATQ0", '\r', buffer, 81, DEFAULT_TIMEOUT / 2, 2);

  /*
  ** Make result codes verbose.
  */
  Send_Modem_Command("ATV1", '\r', buffer, 81, DEFAULT_TIMEOUT / 2, 2);

  /*
  ** Turn on echo.
  */
  Send_Modem_Command("ATE1", '\r', buffer, 81, DEFAULT_TIMEOUT / 2, 2);

  ModemVerboseOn = true;
  ModemEchoOn = true;

  /*
  ** Try sending a plain old AT command to the modem. Now that we have
  *theoretically
  ** turned on verbose result codes we should get an 'OK' back.
  **
  */
  int status = Send_Modem_Command("AT", '\r', buffer, 81, DEFAULT_TIMEOUT, 2);

  if (status < ASSUCCESS) {
    return 0;
  }

  /*
  ** Send the user supplied modem init string
  */
  if (settings->InitStringIndex != -1) {
    const std::string initStr =
        Session.InitStrings.at(settings->InitStringIndex);

    std::istringstream tokenStream(initStr);
    std::string token;

    while (std::getline(tokenStream, token, '|')) {
      // Handle consecutive delimiters
      if (token.empty()) {
        continue;
      }

      status = Send_Modem_Command(token.c_str(), '\r', buffer, 81, 3000, 1);

      if (status < ASSUCCESS) {
        if (WWMessageBox().Process(TXT_ERROR_NO_INIT, TXT_IGNORE, TXT_CANCEL)) {
          return 0;
        }
        error_count++;
        break;
      }
    }
  } else {
  }

  if (settings->Port == 1 && ModemRegistry) {
    // Helper lambda to handle the "Append AT -> Send -> Check Error" pattern.
    // Captures context to access 'buffer' and other necessary variables.
    const auto sendInitCommand = [&](const char* cmdSuffix, int errorMsgId,
                                     int timeout = DEFAULT_TIMEOUT) -> bool {
      if (!cmdSuffix) {
        return true;  // Nothing to send, proceed.
      }

      std::string fullCommand = "AT";
      fullCommand += cmdSuffix;

      const int result =
          Send_Modem_Command(fullCommand.c_str(), '\r', buffer, 81, timeout, 1);

      // Stop initialization only when the command failed and the user clicked
      // "Cancel" (Process returns true for it); success or "Ignore" continues.
      return result == kModemCmdOk || result == kModemCmd0 ||
             !WWMessageBox().Process(errorMsgId, TXT_IGNORE, TXT_CANCEL);
    };

    // 1. Flow Control
    const char* flowCmd = settings->HardwareFlowControl
                              ? ModemRegistry->Get_Modem_Hardware_Flow_Control()
                              : ModemRegistry->Get_Modem_No_Flow_Control();
    const int flowTimeout =
        settings->HardwareFlowControl ? 300 : DEFAULT_TIMEOUT;

    if (!sendInitCommand(flowCmd, TXT_NO_FLOW_CONTROL_RESPONSE, flowTimeout)) {
      return 0;
    }

    // 2. Compression
    const char* compCmd = settings->Compression
                              ? ModemRegistry->Get_Modem_Compression_Enable()
                              : ModemRegistry->Get_Modem_Compression_Disable();

    if (!sendInitCommand(compCmd, TXT_NO_COMPRESSION_RESPONSE)) {
      return 0;
    }

    // 3. Error Correction
    const char* errCmd =
        settings->ErrorCorrection
            ? ModemRegistry->Get_Modem_Error_Correction_Enable()
            : ModemRegistry->Get_Modem_Error_Correction_Disable();

    if (!sendInitCommand(errCmd, TXT_NO_ERROR_CORRECTION_RESPONSE)) {
      return 0;
    }
  }

  /*
  ** We require that auto-answer be disabled so turn it off now.
  */
  status = Send_Modem_Command("ATS0=0", '\r', buffer, 81, DEFAULT_TIMEOUT,
                              INIT_COMMAND_RETRIES);
  if (status != kModemCmdOk) {
    if (WWMessageBox().Process(TXT_ERROR_NO_DISABLE, TXT_IGNORE, TXT_CANCEL)) {
      return 0;
    }
    error_count++;
  }

  /*
  ** If we had an unreasonable number of ignored errors then return failure
  */
  if (error_count >= 3) {
    WWMessageBox().Process(TXT_ERROR_TOO_MANY, TXT_OK);
    return 0;
  }

  return 1;
}

/***************************************************************************
 * NullModemClass::Dial_Modem -- dials a number passed                     *
 *                                                                         *
 * INPUT:                                                                  *
 *		settings		ptr to SerialSettings structure
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		status		DialStatus
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   06/02/1995 DRD : Created.                                             *
 *   8/2/96      ST : Win32 support                                        *
 *=========================================================================*/
// Not const: dials through the serial port.
// NOLINTNEXTLINE(readability-make-member-function-const)
DialStatusType NullModemClass::Dial_Modem(const char* string,
                                          DialMethodType method,
                                          bool reconnect) {
  /*
  ** Get the resolution factor
  */
  //	int factor = SeenBuff.Get_Width()/320;

  /*------------------------------------------------------------------------
  Button Enumerations
  ------------------------------------------------------------------------*/
  constexpr int kButtonCancel = 100;

  /*------------------------------------------------------------------------
  Dialog variables
  ------------------------------------------------------------------------*/
  bool process = true;  // process while true
  DialStatusType dialstatus = DIAL_ERROR;

  int width = 0;
  int height = 0;  // dialog dimensions
  /*------------------------------------------------------------------------
  Determine the dimensions of the text to be used for the dialog box.
  These dimensions will control how the dialog box looks.
  ------------------------------------------------------------------------*/
  const char* buffer_const =
      Text_String(reconnect ? TXT_MODEM_CONNERR_REDIALING : TXT_DIALING);

  std::string buffer(buffer_const);

  Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack, kTpfText);
  Format_Window_String(std::span(buffer), SeenBuff.Get_Height(), width, height);

  const int text_width = width;
  width = std::max(width, 180);
  width += 80;
  height += 120;

  const int x = (SeenBuff.Get_Width() - width) / 2;
  const int y = (SeenBuff.Get_Height() - height) / 2;

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL, kTpfButton,
      x + ((width - (String_Pixel_Width(Text_String(TXT_CANCEL)) + 16)) / 2),
      y + height - (FontHeight + FontYSpacing + 4) - 20);

  /*------------------------------------------------------------------------
  Initialize
  ------------------------------------------------------------------------*/
  Set_Logic_Page(SeenBuff);

  /*------------------------------------------------------------------------
  Create the list
  ------------------------------------------------------------------------*/
  Commands = &cancelbtn;

  Commands->Flag_List_To_Redraw();

  /*------------------------------------------------------------------------
  Draw the dialog
  ------------------------------------------------------------------------*/
  Hide_Mouse();
  if (!reconnect) {
    Load_Title_Page(true);
  }

  Dialog_Box(x, y, width, height);
  Draw_Caption(TXT_NONE, x, y, width);

  Fancy_Text_Print(buffer.c_str(),
                   (SeenBuff.Get_Width() / 2) - (text_width / 2), y + 50,
                   GadgetClass::Get_Color_Scheme(), kTBlack, kTpfText);

  Commands->Draw_All();
  Show_Mouse();

  /*
  ** Start waiting for connection response
  */
  SerialPort->Set_Modem_Dial_Type(static_cast<WinCommDialMethodType>(method));
  /*
  ** Clear out any old modem results that might be hanging around
  */
  SerialPort->Get_Modem_Result(60, buffer.c_str(), 81);
  /*
  ** Dial that sucker
  */
  SerialPort->Dial_Modem(string);

  /*
  ** Sets up the ability to abort modem commands when any input is in the
  ** Keyboard buffer.  This also calls the game CallBack().
  */
  Setup_Abort_Modem();

  /*------------------------------------------------------------------------
  Main Processing Loop
  ------------------------------------------------------------------------*/
  process = true;
  int delay = ModemWaitCarrier;
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      Commands->Draw_All();
    }

    delay = SerialPort->Get_Modem_Result(delay, buffer.c_str(), 81);

    /*.....................................................................
    Process input
    .....................................................................*/
    switch (static_cast<int>(Input)) {
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        dialstatus = DIAL_CANCELED;
        process = false;
        break;

      default:
        break;
    }

    if (process) {
      if (buffer.starts_with("CON")) {
        port::SafeCopy(ModemRXString, buffer.c_str());
        dialstatus = DIAL_CONNECTED;
        process = false;
      } else if (buffer.starts_with("BUSY")) {
        dialstatus = DIAL_BUSY;
        process = false;
      } else if (buffer.starts_with("NO C")) {
        dialstatus = DIAL_NO_CARRIER;
        process = false;
      } else if (buffer.starts_with("NO D")) {
        dialstatus = DIAL_NO_DIAL_TONE;
        process = false;
      } else if (buffer.starts_with("ERRO")) {
        dialstatus = DIAL_ERROR;
        process = false;
      }
    }

    if (delay <= 0) {
      process = false;
    }
  }

  Remove_Abort_Modem();
  // cancelbtn is a local of this function; drop the pointer with it so the
  // static does not outlive the button it names.
  Commands = nullptr;

  return dialstatus;

} /* end of Dial_Modem */

/***************************************************************************
 * NullModemClass::Answer_Modem -- waits for call and answers              *
 *                                                                         *
 * INPUT:                                                                  *
 *		reconnect	whether this is to reconnect
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		status		DialStatus
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   06/02/1995 DRD : Created.                                             *
 *   8/2/96      ST : Added Win32 support                                  *
 *=========================================================================*/
// Not const: answers through the serial port.
// NOLINTNEXTLINE(readability-make-member-function-const)
DialStatusType NullModemClass::Answer_Modem(bool reconnect) {
  /*
  ** Get the resolution factor
  */
  //	int factor 		= (SeenBuff.Get_Width() == 320) ? 1 : 2;

  /*------------------------------------------------------------------------
  Button Enumerations
  ------------------------------------------------------------------------*/
  constexpr int kButtonCancel = 100;

  /*------------------------------------------------------------------------
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ------------------------------------------------------------------------*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*------------------------------------------------------------------------
  Dialog variables
  ------------------------------------------------------------------------*/
  RedrawType display = REDRAW_ALL;  // redraw level
  bool process = true;              // process while true
  DialStatusType dialstatus = DIAL_ERROR;
  bool ring = false;

  int width = 0;
  int height = 0;  // dialog dimensions
  char text_buffer[80 * 3];
  char comm_buffer[80 * 3];

  /*------------------------------------------------------------------------
  Determine the dimensions of the text to be used for the dialog box.
  These dimensions will control how the dialog box looks.
  ------------------------------------------------------------------------*/
  if (reconnect) {
    port::SafeCopy(text_buffer, Text_String(TXT_MODEM_CONNERR_WAITING));
  } else {
    port::SafeCopy(text_buffer, Text_String(TXT_WAITING_FOR_CALL));
  }

  Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack, kTpfText);
  Format_Window_String(text_buffer, SeenBuff.Get_Height(), width, height);

  int text_width = width;
  width = std::max(width, 180);
  width += 80;
  height += 120;

  int x = (SeenBuff.Get_Width() - width) / 2;
  int y = (SeenBuff.Get_Height() - height) / 2;

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL, kTpfButton,
      x + ((width - (String_Pixel_Width(Text_String(TXT_CANCEL)) + 16)) / 2),
      y + height - (FontHeight + FontYSpacing + 4) - 20);

  /*------------------------------------------------------------------------
  Initialize
  ------------------------------------------------------------------------*/
  Set_Logic_Page(SeenBuff);
  Load_Title_Page(true);

  Input = KN_NONE;

  /*------------------------------------------------------------------------
  Create the list
  ------------------------------------------------------------------------*/
  Commands = &cancelbtn;

  Commands->Flag_List_To_Redraw();

  /*
  ** Sets up the ability to abort modem commands when any input is in the
  ** Keyboard buffer.  This also calls the game CallBack() and Input().
  */
  Setup_Abort_Modem();

  /*------------------------------------------------------------------------
  Main Processing Loop
  ------------------------------------------------------------------------*/
  process = true;
  int delay = 60000;
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    /*.....................................................................
    Refresh display if needed
    .....................................................................*/
    if (display != REDRAW_NONE) {
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        /*...............................................................
        Refresh the backdrop
        ...............................................................*/
        if (!reconnect) {
          Load_Title_Page(true);
        }

        /*...............................................................
        Draw the background
        ...............................................................*/
        Dialog_Box(x, y, width, height);

        /*...............................................................
        Draw the labels
        ...............................................................*/
        Draw_Caption(TXT_NONE, x, y, width);

        Fancy_Text_Print(text_buffer,
                         (SeenBuff.Get_Width() / 2) - (text_width / 2), y + 50,
                         GadgetClass::Get_Color_Scheme(), kTBlack, kTpfText);

        Commands->Draw_All();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    delay = SerialPort->Get_Modem_Result(delay, comm_buffer, 81);

    /*.....................................................................
    Process input
    .....................................................................*/
    if (!Input) {
      Input = Commands->Input();
    }
    switch (static_cast<int>(Input)) {
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        dialstatus = DIAL_CANCELED;
        process = false;
        break;

      default:
        break;
    }

    if (process) {
      if (std::string_view(comm_buffer).starts_with("RING")) {
        port::SafeCopy(text_buffer, Text_String(TXT_ANSWERING));

        Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack, kTpfText);
        Format_Window_String(text_buffer, SeenBuff.Get_Height(), width, height);

        text_width = width;
        width = std::max(width, 180);
        width += 80;
        height += 120;

        x = (SeenBuff.Get_Width() - width) / 2;
        y = (SeenBuff.Get_Height() - height) / 2;

        static constexpr unsigned char kAnswerCommand[] = {'A', 'T', 'A', '\r'};
        SerialPort->Write_To_Serial_Port(
            kAnswerCommand, static_cast<int>(sizeof(kAnswerCommand)));

        ring = true;
        delay = ModemWaitCarrier;
        display = REDRAW_ALL;
      } else if (std::string_view(comm_buffer).starts_with("CON")) {
        base::FillBytes(base::ObjectBytes(ModemRXString), 0, 80);
        port::SafeCopy(ModemRXString, comm_buffer);
        dialstatus = DIAL_CONNECTED;
        process = false;
      } else if (std::string_view(comm_buffer).starts_with("BUSY")) {
        dialstatus = DIAL_BUSY;
        process = false;
      } else if (std::string_view(comm_buffer).starts_with("NO C")) {
        dialstatus = DIAL_NO_CARRIER;
        process = false;
      } else if (std::string_view(comm_buffer).starts_with("ERRO")) {
        dialstatus = DIAL_ERROR;
        WWMessageBox().Process(TXT_ERROR_ERROR, TXT_OK);
        process = false;
      }
    }

    if (delay <= 0) {
      if (ring) {
        if (SerialPort->Get_Modem_Status() & kCdSet) {
          absl::SNPrintF(ModemRXString, sizeof(ModemRXString), "%s",
                         "Connected");
          dialstatus = DIAL_CONNECTED;
        } else {
          dialstatus = DIAL_ERROR;
          WWMessageBox().Process(TXT_ERROR_TIMEOUT, TXT_OK);
        }
        process = false;
      } else {
        delay = 60000;
      }
    }
  }

  Remove_Abort_Modem();
  // cancelbtn is a local of this function; drop the pointer with it so the
  // static does not outlive the button it names.
  Commands = nullptr;

  return dialstatus;

} /* end of Answer_Modem */

/***************************************************************************
 * NullModemClass::Hangup_Modem -- hangs up the modem                      *
 *                                                                         *
 * INPUT:                                                                  *
 *		none
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		status		successful or not
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   06/02/1995 DRD : Created.                                             *
 *   8/2/96         : Added Win32 support                                  *
 *=========================================================================*/
// Not const: hangs up through the serial port.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool NullModemClass::Hangup_Modem() {
  char buffer[81];
  char escape[4];

  /*
  **	Turn modem servicing off in the callback routine.
  */
  Session.ModemService = false;

  int status = Send_Modem_Command("AT", '\r', buffer, 81, DEFAULT_TIMEOUT, 1);

  if (status == kModemCmdOk) {
    Session.ModemService = true;
    return true;
  }

  /*
  ** Toggle DTR low then high
  */
  SerialPort->Set_Serial_DTR(false);
  Delay(3200 / 60);
  SerialPort->Set_Serial_DTR(true);

  status = Send_Modem_Command("AT", '\r', buffer, 81, DEFAULT_TIMEOUT, 1);

  if (status == kModemCmdOk) {
    Session.ModemService = true;
    return true;
  }

  int delay = ModemGuardTime;
  while (delay > 0) {
    delay = SerialPort->Get_Modem_Result(delay, buffer, 81);
  }

  /*
  ** Send modem break commmand
  */
  escape[0] = ModemEscapeCode;
  escape[1] = ModemEscapeCode;
  escape[2] = ModemEscapeCode;
  escape[3] = 0;

  SerialPort->Write_To_Serial_Port(escape, 3);

  delay = ModemGuardTime;
  while (delay > 0) {
    delay = SerialPort->Get_Modem_Result(delay, buffer, 81);

    if (std::string_view(buffer).starts_with("OK")) {
      break;
    }
  }

  /*
  ** Send the hangup command
  */
  status = Send_Modem_Command("ATH", '\r', buffer, 81, ModemHangupDelay, 1);

  if (status == kModemCmdOk) {
  } else {
    Session.ModemService = true;
    return false;
  }

  /*
  ** Send spurious ATZ command for no apparent reason
  */
  status = Send_Modem_Command("ATZ", '\r', buffer, 81, 5000, 1);

  if (status != kModemCmdOk) {
    Session.ModemService = true;
    return false;
  }

  Session.ModemService = true;
  return true;

} /* end of Hangup_Modem */

/***********************************************************************************************
 * NMC::Setup_Modem_Echo -- Sets the echo callback function pointer *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Ptr to callback function *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 12:48PM ST : Documented and added WIn32 support *
 *=============================================================================================*/
void NullModemClass::Setup_Modem_Echo(void (*func)(char c)) {
  SerialPort->Set_Echo_Function(func);
}

/***********************************************************************************************
 * NMC::Remove_Modem_Echo -- Set the echo function callback pointer to null *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 12:50PM ST : Documented / Win32 support added *
 *=============================================================================================*/
void NullModemClass::Remove_Modem_Echo() {
  //	Smart_Printf( "Remove Echo modem code\n" );
  SerialPort->Set_Echo_Function(nullptr);
}

/***********************************************************************************************
 * NMC::Print_EchoBuf -- Print out the contents of the echo buffer *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 12:51PM ST : Documented *
 *=============================================================================================*/
void NullModemClass::Print_EchoBuf() {
  for (int i = 0;
       std::cmp_less(i, std::string_view(NullModem.EchoBuf.data()).size());
       i++) {
    if (NullModem.EchoBuf.at(base::ToSize(i)) == '\r') {
      NullModem.EchoBuf.at(base::ToSize(i)) = 1;
    } else {
      if (NullModem.EchoBuf.at(base::ToSize(i)) == '\n') {
        NullModem.EchoBuf.at(base::ToSize(i)) = 2;
      }
    }
  }
  //	Smart_Printf( "Echo buffer length %d (%s)\n", NullModem.EchoCount,
  // NullModem.EchoBuf );
}

/***********************************************************************************************
 * NMC::Reset_EchoBuf -- Empties the echo buffer *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 12:51PM ST : Documented *
 *=============================================================================================*/
void NullModemClass::Reset_EchoBuf() {
  EchoBuf.front() = 0;
  EchoCount = 0;
}

/***********************************************************************************************
 * NMC::Abort_Modem -- Checks for user input so that modem operations can be
 *aborted           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   ASUSERABORT if abort key pressed. ASSUCESS otherwise. *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 12:52PM ST : Documented *
 *=============================================================================================*/
int NullModemClass::Abort_Modem() {
  /*
  ** Button Enumerations
  */
  constexpr int kButtonCancel = 100;

  /*
  ** Invoke game callback
  */
  Call_Back();

  /*
  ** Get user input
  */
  Input = Commands->Input();

  switch (static_cast<int>(Input)) {
    case KN_ESC:
    case ButtonKey(kButtonCancel):
      return ASUSERABORT;
    default:
      break;
  }

  return ASSUCCESS;

} /* end of Abort_Modem */

/***********************************************************************************************
 * NMC::Setup_Abort_Modem -- sets the modem abort function pointer *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 2:59PM ST : Documented / Win32 support added *
 *=============================================================================================*/
void NullModemClass::Setup_Abort_Modem() {
  SerialPort->Set_Abort_Function(Abort_Modem);
}

/***********************************************************************************************
 * NMC::Remove_Abort_Modem -- Removes the modem abort function pointer *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 3:01PM ST : Documented / Win32 support added *
 *=============================================================================================*/
void NullModemClass::Remove_Abort_Modem() {
  SerialPort->Set_Abort_Function(nullptr);
}

/***********************************************************************************************
 * NMC::Change_IRQ_Priority -- Increases the priority of the serial interrupt *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Interrupt request number *
 *                                                                                             *
 * OUTPUT:   ASSUCCESS if changed *
 *                                                                                             *
 * WARNINGS: The Win32 version of this function does nothing. * Priorities are
 *controlled by windoze                                              *
 *                                                                                             *
 * HISTORY: * 8/2/96 3:03PM ST : Documented / Win32 support added *
 *=============================================================================================*/
int NullModemClass::Change_IRQ_Priority(int /*irq*/) {
  return ASSUCCESS;
} /* end of Change_IRQ_Priority */

/***********************************************************************************************
 * NMC::Get_Modem_Status -- returns status of modem control bits *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Modem status *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 3:06PM ST : Documented / Win32 support added *
 *=============================================================================================*/
uint32_t NullModemClass::Get_Modem_Status() {
  char buffer[81];

  // Modem status is a small bit mask (CTS/DSR/RI/CD).
  uint32_t modemstatus = SerialPort->Get_Modem_Status();

  const int status =
      Send_Modem_Command("AT", '\r', buffer, 81, DEFAULT_TIMEOUT, 1);

  if (status == kModemCmdOk) {
    modemstatus &= ~kCdSet;
  }

  return modemstatus;

} /* end of Get_Modem_Status */

/***********************************************************************************************
 * NMC::Send_Modem_Command -- Sends an 'AT' command to the modem and gets the
 *response         *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    command to send to modem. e.g. 'ATZ' * terminator byte for command
 *string                                                * buffer to put modem
 *response into                                                 * length of
 *above buffer                                                            *
 *           delay to wait for response * number of times to retry when modem
 *doesnt respond                                *
 *                                                                                             *
 * OUTPUT:   input delay less the time it took the modem to respond *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 3:09PM ST : Documented / Win32 support added *
 *=============================================================================================*/
int NullModemClass::Send_Modem_Command(const char* command, char terminator,
                                       char* buffer, int buflen, int delay,
                                       int retries) {
  return SerialPort->Send_Command_To_Modem(command, terminator, buffer, buflen,
                                           delay, retries);
}

/***********************************************************************************************
 * NMC::Verify_And_Convert_To_Int -- converts a text string of numbers to an int
 **
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to buffer *
 *                                                                                             *
 * OUTPUT:   value of text number in buffer *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 3:13PM ST : Documented *
 *=============================================================================================*/
int NullModemClass::Verify_And_Convert_To_Int(char* buffer) {
  int value = 0;
  const int len = static_cast<int>(std::string_view(buffer).size());

  for (int i = 0; i < len; i++) {
    if (!isdigit(static_cast<unsigned char>(
            std::string_view(buffer).at(base::ToSize(i))))) {
      value = -1;
      break;
    }
  }

  if (value == 0) {
    value = tech::ParseInteger<int>(buffer).value_or(0);
  }

  return value;

} /* end of Verify_And_Convert_To_Int */

/*************************** end of nullmgr.cpp ****************************/
