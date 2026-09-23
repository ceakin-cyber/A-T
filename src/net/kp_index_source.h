#pragma once

#include "net/kp_index_cache.h"

#include <functional>
#include <optional>

namespace net {

enum class KpSource {
    Network,    // fetched from NOAA and parsed just now
    FreshCache, // cache was recent enough that no fetch was needed
    StaleCache, // fetch (or parse) failed, so an older cached copy was used
};

// A short name for logging and display, such as "network" or "stale cache".
const char* ToString(KpSource source);

struct LoadedKp {
    double kp = 0.0;
    Clock::time_point fetchedAt; // when this reading was fetched, not when it was loaded
    KpSource source;
};

// Fetches and parses the most recent Kp value in one step (see FetchKpIndex and
// ParseMostRecentKp in net/kp_index_fetch.h), returning nullopt if either step fails. Tests
// substitute a fake one so loading does not need the network.
using KpFetcher = std::function<std::optional<double>()>;

// Loads the most recent Kp value: a fresh cache entry if there is one (see kKpMaxAge), otherwise
// a new fetch (which is cached), otherwise a stale cache entry if the fetch failed. Returns
// nullopt only if there is no usable cache and the fetch failed.
std::optional<LoadedKp> LoadKpIndex(const std::filesystem::path& cacheDir, Clock::time_point now,
                                    const KpFetcher& fetch);

// Same, using the default cache directory (net::DefaultCacheDir()), the current time, and a
// fetcher built from FetchKpIndex + ParseMostRecentKp. Logs which source was used.
std::optional<LoadedKp> LoadKpIndex();

} // namespace net
