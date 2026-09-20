#pragma once

#include "net/tle_cache.h"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace net {

enum class TleSource {
    Network,    // fetched from Celestrak just now
    FreshCache, // cache was recent enough that no fetch was needed
    StaleCache, // fetch failed, so an older cached copy was used
};

struct LoadedTle {
    std::string text;
    Clock::time_point fetchedAt; // when this TLE was downloaded, not when it was loaded
    TleSource source;
};

using TleFetcher = std::function<std::optional<std::string>(int noradId)>;

// Loads a TLE: a fresh cache entry if there is one, otherwise a new fetch (which is cached),
// otherwise a stale cache entry if the fetch failed. Returns nullopt only if there is no usable
// cache and the fetch failed.
std::optional<LoadedTle> LoadTle(const std::filesystem::path& cacheDir, int noradId,
                                 Clock::time_point now, const TleFetcher& fetch);

// Same, using the default cache directory, the current time and Celestrak. Logs which source
// was used.
std::optional<LoadedTle> LoadTle(int noradId);

} // namespace net
