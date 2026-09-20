#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>

namespace net {

using Clock = std::chrono::system_clock;

// How long a cached TLE is reused before it is fetched again.
constexpr std::chrono::hours kTleMaxAge{2};

struct CachedTle {
    std::string text; // raw TLE text, as returned by Celestrak
    Clock::time_point fetchedAt;
};

// Per-user cache directory: $XDG_CACHE_HOME/a-t, else ~/.cache/a-t, else a temp directory.
std::filesystem::path DefaultCacheDir();

// Reads the cached TLE for a catalog number. Returns nullopt if the file is missing or its
// timestamp header is unreadable.
std::optional<CachedTle> ReadTleCache(const std::filesystem::path& dir, int noradId);

// Writes the TLE and its fetch time to the cache, creating the directory if needed.
// Returns false if the file could not be written.
bool WriteTleCache(const std::filesystem::path& dir, int noradId, const std::string& text,
                   Clock::time_point fetchedAt);

// True if fetchedAt is no older than maxAge. A fetch time in the future (clock skew) is not fresh.
bool IsFresh(Clock::time_point fetchedAt, Clock::time_point now,
             Clock::duration maxAge = kTleMaxAge);

} // namespace net
