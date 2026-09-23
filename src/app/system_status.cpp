#include "app/system_status.h"

#include "app/format.h"

#include <algorithm>

namespace app {

std::string SystemState(bool anySatelliteLoaded) {
    return anySatelliteLoaded ? "ONLINE" : "OFFLINE";
}

std::string SystemMode(const std::vector<net::TleSource>& sources) {
    if (sources.empty()) {
        return "OFFLINE";
    }
    net::TleSource worst = net::TleSource::Network;
    for (const net::TleSource source : sources) {
        if (source == net::TleSource::StaleCache) {
            worst = net::TleSource::StaleCache;
            break; // nothing worse than this; no need to keep looking
        }
        if (source == net::TleSource::FreshCache) {
            worst = net::TleSource::FreshCache;
        }
    }
    return FormatNodeMode(worst);
}

std::optional<net::Clock::time_point> LastSync(
    const std::vector<net::Clock::time_point>& fetchTimes) {
    if (fetchTimes.empty()) {
        return std::nullopt;
    }
    return *std::max_element(fetchTimes.begin(), fetchTimes.end());
}

} // namespace app
