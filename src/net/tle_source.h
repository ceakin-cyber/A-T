#pragma once

#include <optional>
#include <string>

namespace net {

// Returns the TLE for a catalog number: from the disk cache if it is fresh, otherwise fetched
// from Celestrak and cached. Returns nullopt if there is no fresh cache and the fetch fails.
std::optional<std::string> LoadTle(int noradId);

} // namespace net
