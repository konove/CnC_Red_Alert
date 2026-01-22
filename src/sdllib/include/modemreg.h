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
#ifndef CNC_RED_ALERT_SDLLIB_MODEMREG_H_
#define CNC_RED_ALERT_SDLLIB_MODEMREG_H_

#include <string>

class ModemRegistryEntryClass {
 public:
  explicit ModemRegistryEntryClass(int modem_number);

  const char* Get_Modem_Name() { return ModemName_.c_str(); }

  const char* Get_Modem_Device_Name() { return ModemDeviceName_.c_str(); }

  const char* Get_Modem_Error_Correction_Enable() {
    return ErrorCorrectionEnable_.c_str();
  }

  const char* Get_Modem_Error_Correction_Disable() {
    return ErrorCorrectionDisable_.c_str();
  }

  const char* Get_Modem_Compression_Enable() {
    return CompressionEnable_.c_str();
  }

  const char* Get_Modem_Compression_Disable() {
    return CompressionDisable_.c_str();
  }

  const char* Get_Modem_Hardware_Flow_Control() {
    return HardwareFlowControl_.c_str();
  }

  const char* Get_Modem_No_Flow_Control() { return NoFlowControl_.c_str(); }

 private:
  std::string ModemName_;
  std::string ModemDeviceName_;
  std::string ErrorCorrectionEnable_;
  std::string ErrorCorrectionDisable_;
  std::string CompressionEnable_;
  std::string CompressionDisable_;
  std::string HardwareFlowControl_;
  std::string NoFlowControl_;
};

#endif  // CNC_RED_ALERT_SDLLIB_MODEMREG_H_
