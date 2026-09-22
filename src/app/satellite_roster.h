#pragma once

#include "app/config.h"
#include "app/event_log.h"
#include "app/pass_planner.h"
#include "app/satellite_position.h"
#include "app/tracked_satellite.h"
#include "core/geodetic.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace app {

// One entry of the watchlist, together with everything it takes to track it. `satellite` and
// `planner` are unset if it failed to load (no network and no cache); `eventLog` still exists in
// that case, just staying empty.
struct WatchedSatellite {
    WatchEntry entry;
    std::optional<TrackedSatellite> satellite;
    std::optional<PassPlanner> planner;
    EventLog eventLog;
    std::optional<SatellitePosition> position;
    std::optional<core::Pass> nextPass;
};

using SatelliteLoader = std::function<std::optional<TrackedSatellite>(int noradId)>;

// Recomputes `watched`'s position and next pass for `now`, and logs a rise or set if the
// satellite's visibility has changed since the last call. Does nothing if it never loaded.
void UpdateWatchedSatellite(WatchedSatellite& watched, net::Clock::time_point now);

// Loads every entry of a watchlist and keeps them ready to track, with one of them selected.
class SatelliteRoster {
  public:
    // `loader` defaults to the real network/cache path (LoadSatellite); tests substitute a fake
    // one so loading does not need the network.
    SatelliteRoster(const std::vector<WatchEntry>& watchlist, const core::Geodetic& observer,
                    SatelliteLoader loader = LoadSatellite);

    // Advances every loaded satellite (not only the selected one), so each keeps a live position,
    // next pass and event log even while it is not on screen.
    void Update(net::Clock::time_point now);

    std::size_t Size() const { return satellites_.size(); }
    const WatchedSatellite& At(std::size_t index) const { return satellites_.at(index); }

    int SelectedIndex() const { return selectedIndex_; }
    // Clamped to a valid index; does nothing if the roster is empty.
    void SetSelectedIndex(int index);
    const WatchedSatellite& Selected() const {
        return satellites_.at(static_cast<std::size_t>(selectedIndex_));
    }

  private:
    std::vector<WatchedSatellite> satellites_;
    int selectedIndex_ = 0;
};

} // namespace app
