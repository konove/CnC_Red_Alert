#ifndef CNC_RED_ALERT_RA_WOL_MAIN_H_
#define CNC_RED_ALERT_RA_WOL_MAIN_H_

// File: Entry point of the Westwood Online client.

// Runs the Westwood Online session from login through the chat lobby to game
// setup, creating pWolapi the first time through and reusing it after a game.
// Returns 1 when a game has been set up and the caller should start it, 0 when
// the player backed out to the main menu, and -1 when a patch was downloaded
// and the application must shut down for it to install.
int WOL_Main();

#endif  // CNC_RED_ALERT_RA_WOL_MAIN_H_
