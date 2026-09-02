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

// The Planet Westwood connection settings and the game-result bookkeeping the
// Internet game path shares. Everything that talked to Westwood Chat -- the
// launch packet, the registry lookup, the spawn and the waiting dialog -- used
// to live here; Westwood Online replaced all of it.

#include "ra/internet.h"

char PlanetWestwoodHandle[] = {"Handle"};      // Planet WW user name
char PlanetWestwoodPassword[] = {"Password"};  // Planet WW password
char PlanetWestwoodIPAddress[IP_ADDRESS_MAX] = {
    "206.154.108.87"};                 // IP of server or other player
long PlanetWestwoodPortNumber = 1234;  // Port number to send to
bool PlanetWestwoodIsHost =
    false;  // Flag true if player has control of game options
unsigned long PlanetWestwoodGameID;     // Game ID
unsigned long PlanetWestwoodStartTime;  // Time that game was started
bool GameStatisticsPacketSent;  // Flag that game stats have been sent
bool ConnectionLost;  // Flag that the connection to the other player was lost
int ShowCommand;
