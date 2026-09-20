#include "net/tle_source.h"

#include "net/tle_fetch.h"

#include <chrono>
#include <iostream>

namespace net {

const char* ToString(TleSource source) {
    switch (source) {
    case TleSource::Network:
        return "network";
    case TleSource::FreshCache:
        return "cache";
    case TleSource::StaleCache:
        return "stale cache";
    }
    return "unknown";
}

std::optional<LoadedTle> LoadTle(const std::filesystem::path& cacheDir, int noradId,
                                 Clock::time_point now, const TleFetcher& fetch) {
    const std::optional<CachedTle> cached = ReadTleCache(cacheDir, noradId);
    if (cached && IsFresh(cached->fetchedAt, now)) {
        return LoadedTle{cached->text, cached->fetchedAt, TleSource::FreshCache};
    }

    if (const std::optional<std::string> fetched = fetch(noradId)) {
        if (!WriteTleCache(cacheDir, noradId, *fetched, now)) {
            std::cerr << "TLE cache write failed in " << cacheDir << '\n';
        }
        return LoadedTle{*fetched, now, TleSource::Network};
    }

    if (cached) {
        return LoadedTle{cached->text, cached->fetchedAt, TleSource::StaleCache};
    }
    return std::nullopt;
}

namespace {

std::string AgeText(Clock::time_point fetchedAt, Clock::time_point now) {
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(now - fetchedAt).count();
    if (minutes < 120) {
        return std::to_string(minutes) + " min";
    }
    return std::to_string(minutes / 60) + " h";
}

} // namespace

std::optional<LoadedTle> LoadTle(int noradId) {
    const Clock::time_point now = Clock::now();
    const std::optional<LoadedTle> tle = LoadTle(DefaultCacheDir(), noradId, now, FetchTle);

    if (!tle) {
        std::cerr << "TLE " << noradId << ": fetch failed and no cached copy exists\n";
        return tle;
    }
    switch (tle->source) {
    case TleSource::Network:
        std::cout << "TLE " << noradId << ": fetched from Celestrak\n";
        break;
    case TleSource::FreshCache:
        std::cout << "TLE " << noradId << ": using cache (" << AgeText(tle->fetchedAt, now)
                  << " old)\n";
        break;
    case TleSource::StaleCache:
        std::cerr << "TLE " << noradId << ": fetch failed, using stale cache ("
                  << AgeText(tle->fetchedAt, now) << " old)\n";
        break;
    }
    return tle;
}

} // namespace net
