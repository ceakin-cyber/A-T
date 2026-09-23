#include "app/system_status.h"

#include <algorithm>

namespace app {

std::string SystemState(bool anySatelliteLoaded) {
    return anySatelliteLoaded ? "ONLINE" : "OFFLINE";
}

std::optional<net::Clock::time_point> LastSync(
    const std::vector<net::Clock::time_point>& fetchTimes) {
    if (fetchTimes.empty()) {
        return std::nullopt;
    }
    return *std::max_element(fetchTimes.begin(), fetchTimes.end());
}

} // namespace app
