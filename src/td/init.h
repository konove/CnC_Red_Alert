#ifndef CNC_RED_ALERT_TD_INIT_H_
#define CNC_RED_ALERT_TD_INIT_H_

#include <optional>
#include <span>
#include <string_view>

#include "td/ipxaddr.h"
#include "td/startup_options.h"

void Uninit_Game();
void Load_Title_Page(bool visible = false);
void Anim_Init();
bool Init_Game();
bool Select_Game(bool fade = false);
// Returns what the command-line switches ask for, leaving the game's state
// alone; main() applies the options. `arguments` are the command-line
// arguments without the program name. Returns nothing when the game should
// not start: the usage text was asked for or a switch is invalid.
std::optional<StartupOptions> Parse_Command_Line(
    std::span<const std::string_view> arguments);
void Parse_INI_File();
int Version_Number();
void Save_Recording_Values();
void Load_Recording_Values();

// Returns the network across an IPX bridge that `address` names, the
// "-DESTNET" switch's value or the config file's DestNet option: up to ten
// dot-separated hex bytes, the first four the network number and the rest the
// node. Only the network is used; the returned address has the broadcast
// node, so packets reach every machine across the bridge. Returns nothing for
// a malformed address, or one shorter than four bytes.
std::optional<IPXAddressClass> ParseDestNet(std::string_view address);

#endif  // CNC_RED_ALERT_TD_INIT_H_
