#ifndef CNC_RED_ALERT_TECH_GAME_FILE_H_
#define CNC_RED_ALERT_TECH_GAME_FILE_H_

// File: opening the game's data as a ByteStream. A name is looked up as a
// loose file through SearchPaths first and then inside the registered
// mixfile archives, whether those are cached in memory or still on disk, so
// callers see an ordinary stream either way. game_file.cc also holds the
// name-to-stream lookup that the archives themselves are opened with, which is
// what lets a mixfile packed inside another mixfile work.
//
// Originally CCFILE.H (class CCFileClass) by Joe L. Bostic, October 17, 1994.

#include <memory>
#include <string_view>

#include "base/types.h"
#include "tech/byte_stream.h"
#include "tech/file_access.h"

// Returns a stream over the bytes of name resolved the way the game looks up
// data: writes never search the search paths or the archives, going straight
// to a loose file next to the executable (though OpenDiskFile still prefers
// an existing lowercase twin of that name over creating a new upper-case
// one); a read first checks the search paths and then the registered
// mixfile archives (a loose file wins over a packed copy, so patches work).
// Returns nullptr if name is empty, found nowhere, or cannot be opened. This
// is how the mixfile archives open their own files, including one packed
// inside another.
std::unique_ptr<ByteStream> OpenGameFile(std::string_view name,
                                         FileAccess access = FileAccess::kRead);

// Returns true if name is packed in a registered archive or found as a loose
// file.
bool GameFileExists(std::string_view name);

// Returns the size of name in bytes, for a packed file the size of the
// embedded file, without opening it. Returns 0 for a name found nowhere.
base::ssize GameFileSize(std::string_view name);

// Deletes the loose file by this name. Returns false, deleting nothing, if
// there is none; a file packed in an archive cannot be deleted.
bool DeleteGameFile(std::string_view name);

#endif  // CNC_RED_ALERT_TECH_GAME_FILE_H_
