#pragma once

#include "net/tle_cache.h"

#include <optional>
#include <string>
#include <vector>

namespace app {

// The station's own identity and health, for the header's system status readout: who is running
// it, which node it is, what mode it is operating in, its overall lifecycle state, and when its
// data was last synchronized. callsign and nodeId have real (if hardcoded) values, and state and
// lastSync have real starting values (see SystemState and LastSync below); mode is still a plain
// placeholder, wired to reflect the app's actual behavior in the issue after this one.
struct SystemStatus {
    std::string callsign;              // operator identity, e.g. an amateur radio callsign
    std::string nodeId;                // this station's own identifier
    std::string mode;                  // operating mode, e.g. "LIVE", "CACHED", "LOW-VISIBILITY"
    std::string state = "INITIALIZING"; // lifecycle state; see SystemState

    // When this station's data (its tracked satellites' TLEs, at least) was last synchronized.
    // The default, epoch, means "never" -- the same as an empty LastSync() result; see below.
    net::Clock::time_point lastSync{};
};

// A placeholder callsign and node id, until a config file (like app::kObserver before it) makes
// them real; mode and lastSync are still wired up in later issues. "N0CALL" is the ham radio
// convention for "no callsign set" -- not a real signal, and a clear placeholder to anyone who
// recognizes it.
inline constexpr const char* kCallsign = "N0CALL";
inline constexpr const char* kNodeId = "NODE-01";

// The system's lifecycle state once TLE loading has been attempted for every watched satellite:
// "ONLINE" if at least one of them came back with real data (whether from the network or a
// cache), "OFFLINE" if every one of them failed (no network and no cache for any of them). This
// mirrors the header bar's own existing NODE: ONLINE/OFFLINE check (see ui/header.cpp), just
// generalized from one selected satellite to the whole roster. Before this has been called at
// all, SystemStatus::state defaults to "INITIALIZING" -- there is no observable gap between that
// and the first call in this app's current, synchronous startup, but the field still models the
// state honestly for whenever loading becomes asynchronous.
std::string SystemState(bool anySatelliteLoaded);

// The most recent successful data fetch, for SystemStatus::lastSync: the latest of the given
// fetch times (each a loaded satellite's own TrackedSatellite::fetchedAt), or nullopt if
// `fetchTimes` is empty -- the same case SystemState reports as "OFFLINE". The caller decides how
// to fold that into SystemStatus::lastSync (its own default, epoch, is a reasonable "never").
std::optional<net::Clock::time_point> LastSync(
    const std::vector<net::Clock::time_point>& fetchTimes);

} // namespace app
