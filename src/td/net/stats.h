// File: The game statistics Tiberian Dawn reports to the Westwood server.

#ifndef CNC_RED_ALERT_TD_NET_STATS_H_
#define CNC_RED_ALERT_TD_NET_STATS_H_

namespace td {

// Notes when the game started and when it ended. The statistics packet
// carries the difference as the game's length.
void Register_Game_Start_Time();
void Register_Game_End_Time();

// Sends the finished game's statistics, once. Does nothing if they have
// gone already or if this was not an internet game.
void Send_Statistics_Packet();

}  // namespace td

#endif  // CNC_RED_ALERT_TD_NET_STATS_H_
