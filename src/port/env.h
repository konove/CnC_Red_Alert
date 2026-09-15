// File: Reading environment variables by value.
//
// std::getenv returns a pointer into the process environment, which any later
// putenv or setenv may move. GetEnv copies the value out at once, so callers
// hold a string of their own and the one getenv call is documented here.
//
// Example:
//   if (const auto path = port::GetEnv("RA_SAVE_DUMP")) {
//     dump_file.Open(path->c_str(), FileAccess::kWrite);
//   }

#ifndef CNC_RED_ALERT_PORT_ENV_H_
#define CNC_RED_ALERT_PORT_ENV_H_

#include <optional>
#include <string>

namespace port {

// Returns the value of the environment variable `name`, or nullopt when it is
// not set. A variable set to the empty string yields an empty string.
[[nodiscard]] std::optional<std::string> GetEnv(const char* name);

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_ENV_H_
