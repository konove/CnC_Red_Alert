// File: Opening an installed game's data for the command line tools: the
// search path, the MIX archives and the key their indexes are encrypted with.

#ifndef CNC_RED_ALERT_TOOLS_GAME_DATA_H_
#define CNC_RED_ALERT_TOOLS_GAME_DATA_H_

#include <string_view>

#include "tech/pk.h"

// The public key the shipped archives' indexes are encrypted with. The first
// call builds it; it lives as long as the program, which the archives
// registered with it require.
const PKey& MixKey();

// Makes game_dir, an installation directory of either game, the place game
// files are looked up: adds it to the search path and registers every archive
// either game ships that is present there, outermost first, so the archives
// nested in the MAIN*.MIX of today's Red Alert release open too. After this,
// OpenGameFile() and MixArchive find a file whether it is loose or packed.
void OpenGameData(std::string_view game_dir);

#endif  // CNC_RED_ALERT_TOOLS_GAME_DATA_H_
