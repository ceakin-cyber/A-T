#pragma once

#include "net/tle_cache.h"

#include <string>

namespace app {

// The station's own identity and health, for the header's system status readout: who is running
// it, which node it is, what mode it is operating in, its overall lifecycle state, and when its
// data was last synchronized. Every field here is a plain placeholder for now -- this only
// defines the shape; callsign and nodeId are given real (if hardcoded) values next, and mode,
// state and lastSync are wired to reflect the app's actual behavior in the issues after that.
struct SystemStatus {
    std::string callsign; // operator identity, e.g. an amateur radio callsign
    std::string nodeId;   // this station's own identifier
    std::string mode;     // operating mode, e.g. "LIVE", "CACHED", "LOW-VISIBILITY"
    std::string state;    // lifecycle state, e.g. "STARTING", "RUNNING"

    // When this station's data (its tracked satellites' TLEs, at least) was last synchronized.
    net::Clock::time_point lastSync{};
};

// A placeholder callsign and node id, until a config file (like app::kObserver before it) makes
// them real; mode, state and lastSync are still wired up in later issues. "N0CALL" is the ham
// radio convention for "no callsign set" -- not a real signal, and a clear placeholder to anyone
// who recognizes it.
inline constexpr const char* kCallsign = "N0CALL";
inline constexpr const char* kNodeId = "NODE-01";

} // namespace app
