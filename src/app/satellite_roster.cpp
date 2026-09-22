#include "app/satellite_roster.h"

#include "core/time.h"

#include <algorithm>

namespace app {

void UpdateWatchedSatellite(WatchedSatellite& watched, net::Clock::time_point now) {
    if (!watched.satellite || !watched.planner) {
        return;
    }
    watched.position = ComputePosition(*watched.satellite, now);
    watched.nextPass = watched.planner->Next(now);

    // The planner always returns a pass whose set time is still ahead; it is the pass in
    // progress once "now" has reached its rise time, and the upcoming one otherwise.
    const bool aboveHorizon = watched.nextPass.has_value() &&
                              now >= core::TimePointFromJulianDate(watched.nextPass->riseJd);
    watched.eventLog.LogVisibilityChange(now, aboveHorizon);
}

SatelliteRoster::SatelliteRoster(const std::vector<WatchEntry>& watchlist,
                                 const core::Geodetic& observer, SatelliteLoader loader) {
    for (const WatchEntry& entry : watchlist) {
        WatchedSatellite watched;
        watched.entry = entry;
        watched.satellite = loader(entry.noradId);
        if (watched.satellite) {
            watched.planner.emplace(watched.satellite->model, observer);
        }
        satellites_.push_back(std::move(watched));
    }
}

void SatelliteRoster::Update(net::Clock::time_point now) {
    for (WatchedSatellite& watched : satellites_) {
        UpdateWatchedSatellite(watched, now);
    }
}

void SatelliteRoster::SetSelectedIndex(int index) {
    if (satellites_.empty()) {
        return;
    }
    selectedIndex_ = std::clamp(index, 0, static_cast<int>(satellites_.size()) - 1);
}

} // namespace app
