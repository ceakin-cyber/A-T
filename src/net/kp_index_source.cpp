#include "net/kp_index_source.h"

#include "net/kp_index_fetch.h"

#include <iostream>

namespace net {

const char* ToString(KpSource source) {
    switch (source) {
    case KpSource::Network:
        return "network";
    case KpSource::FreshCache:
        return "cache";
    case KpSource::StaleCache:
        return "stale cache";
    }
    return "unknown";
}

std::optional<LoadedKp> LoadKpIndex(const std::filesystem::path& cacheDir, Clock::time_point now,
                                    const KpFetcher& fetch) {
    const std::optional<CachedKp> cached = ReadKpCache(cacheDir);
    if (cached && IsFresh(cached->fetchedAt, now, kKpMaxAge)) {
        return LoadedKp{cached->kp, cached->fetchedAt, KpSource::FreshCache};
    }

    if (const std::optional<double> fetched = fetch()) {
        if (!WriteKpCache(cacheDir, *fetched, now)) {
            std::cerr << "Kp index cache write failed in " << cacheDir << '\n';
        }
        return LoadedKp{*fetched, now, KpSource::Network};
    }

    if (cached) {
        return LoadedKp{cached->kp, cached->fetchedAt, KpSource::StaleCache};
    }
    return std::nullopt;
}

std::optional<LoadedKp> LoadKpIndex() {
    const Clock::time_point now = Clock::now();
    const KpFetcher fetch = [] {
        const std::optional<std::string> raw = FetchKpIndex();
        return raw ? ParseMostRecentKp(*raw) : std::nullopt;
    };
    const std::optional<LoadedKp> kp = LoadKpIndex(DefaultCacheDir(), now, fetch);

    if (!kp) {
        std::cerr << "Kp index: fetch failed and no cached copy exists\n";
        return kp;
    }
    switch (kp->source) {
    case KpSource::Network:
        std::cout << "Kp index: fetched from NOAA (" << kp->kp << ")\n";
        break;
    case KpSource::FreshCache:
        std::cout << "Kp index: using cache (" << kp->kp << ")\n";
        break;
    case KpSource::StaleCache:
        std::cerr << "Kp index: fetch failed, using stale cache (" << kp->kp << ")\n";
        break;
    }
    return kp;
}

} // namespace net
