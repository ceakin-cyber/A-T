#pragma once

#include "net/tle_cache.h"

#include <filesystem>
#include <optional>

namespace net {

// How long a cached Kp reading is reused before it is fetched again. NOAA's own planetary Kp
// index only updates every 3 hours (each reading covers a 3-hour window; see
// net/kp_index_fetch.h), so refreshing more often than that can never see new data. An hour
// stays comfortably under that, so a new reading is picked up reasonably soon after it is
// published, without polling every few minutes for data that only changes a few times a day.
constexpr std::chrono::hours kKpMaxAge{1};

struct CachedKp {
    double kp = 0.0;
    Clock::time_point fetchedAt;
};

// Reads the cached Kp reading from `dir` (see net::DefaultCacheDir()). Returns nullopt if the
// file is missing or its contents are unreadable.
std::optional<CachedKp> ReadKpCache(const std::filesystem::path& dir);

// Writes the Kp reading and its fetch time to the cache, creating the directory if needed.
// Returns false if the file could not be written.
bool WriteKpCache(const std::filesystem::path& dir, double kp, Clock::time_point fetchedAt);

} // namespace net
