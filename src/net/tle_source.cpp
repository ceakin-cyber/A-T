#include "net/tle_source.h"

#include "net/tle_cache.h"
#include "net/tle_fetch.h"

#include <chrono>
#include <iostream>

namespace net {

std::optional<std::string> LoadTle(int noradId) {
    const std::filesystem::path cacheDir = DefaultCacheDir();
    const Clock::time_point now = Clock::now();

    if (const auto cached = ReadTleCache(cacheDir, noradId);
        cached && IsFresh(cached->fetchedAt, now)) {
        const auto minutes =
            std::chrono::duration_cast<std::chrono::minutes>(now - cached->fetchedAt);
        std::cout << "TLE " << noradId << ": using cache (" << minutes.count() << " min old)\n";
        return cached->text;
    }

    const std::optional<std::string> fetched = FetchTle(noradId);
    if (!fetched) {
        return std::nullopt;
    }
    std::cout << "TLE " << noradId << ": fetched from Celestrak\n";
    if (!WriteTleCache(cacheDir, noradId, *fetched, now)) {
        std::cerr << "TLE cache write failed in " << cacheDir << '\n';
    }
    return fetched;
}

} // namespace net
