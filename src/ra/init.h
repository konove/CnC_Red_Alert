#ifndef CNC_RED_ALERT_RA_INIT_H_
#define CNC_RED_ALERT_RA_INIT_H_

#include <optional>
#include <span>
#include <string_view>

#include "ra/ipxaddr.h"
#include "ra/startup_options.h"

void Load_Title_Page(bool visible = false);
bool Init_Game();
bool Select_Game(bool fade = false);
// Returns what the command-line switches ask for, leaving the game's state
// alone; main() applies the options. `arguments` are the command-line
// arguments without the program name. Prints the usage text and returns
// nothing on "-?" or an invalid "-X" option.
std::optional<StartupOptions> Parse_Command_Line(
    std::span<const std::string_view> arguments);
void Parse_INI_File();

// Returns the network across an IPX bridge that `address` names, the
// "-DESTNET" switch's value or the config file's DestNet option: up to ten
// dot-separated hex bytes, the first four the network number and the rest the
// node. Only the network is used; the returned address has the broadcast
// node, so packets reach every machine across the bridge. Returns nothing for
// a malformed address, or one shorter than four bytes.
std::optional<IPXAddressClass> ParseDestNet(std::string_view address);

#endif  // CNC_RED_ALERT_RA_INIT_H_
