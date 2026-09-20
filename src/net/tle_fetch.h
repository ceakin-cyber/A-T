#pragma once

#include <optional>
#include <string>

namespace net {

// Fetches the current TLE for a NORAD catalog number from Celestrak.
// Returns the raw three-line text (name, line 1, line 2), or nullopt on any failure.
// The reason for a failure is written to stderr.
std::optional<std::string> FetchTle(int noradId);

} // namespace net
