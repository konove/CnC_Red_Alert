// File: Scenario file transfers between the players of a multiplayer game.

#ifndef CNC_RED_ALERT_RA_SENDFILE_H_
#define CNC_RED_ALERT_RA_SENDFILE_H_

#include <cstddef>
#include <span>

// Receives a scenario file the host is sending, writing it under
// 'file_name'. Returns whether the whole file arrived with the right CRC.
bool Receive_Remote_File(char* file_name, unsigned int file_length,
                         unsigned int crc, int gametype);

// Sends a scenario file to every other player. Returns whether they all
// acknowledged it.
bool Send_Remote_File(const char* file_name, int gametype);

// Asks the host for the scenario it is about to play and writes the name it
// was stored under into 'return_name'. Returns whether the transfer
// succeeded.
bool Get_Scenario_File_From_Host(std::span<char> return_name, size_t dest_size,
                                 int gametype);

#endif  // CNC_RED_ALERT_RA_SENDFILE_H_
