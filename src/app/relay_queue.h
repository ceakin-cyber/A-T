#pragma once

#include "net/tle_source.h"

#include <optional>
#include <string>
#include <vector>

namespace app {

// Where a relay queue item stands, shown beside its label in the RELAY QUEUE panel.
enum class RelayState {
    Sent,  // already relayed
    Hold,  // queued but held back
    Armed, // ready to relay
    None,  // no state to show
};

// One line of the RELAY QUEUE panel: a label and its state.
struct RelayItem {
    std::string label;
    RelayState state = RelayState::None;
};

// The state's display name for the RELAY QUEUE panel: "SENT", "HOLD", "ARMED" or "NONE".
const char* ToString(RelayState state);

// The CARRIER PING relay item, its state taken from the last real TLE fetch for each watched
// satellite: one entry per satellite, holding the source its TLE loaded from, or nullopt if it
// failed to load (fetch failed with no cache to fall back on). Like SystemMode (see
// app/system_status.h), the worst case across the roster wins:
//   - HOLD if any satellite's fetch failed (a stale cache, or nothing at all);
//   - SENT if every one succeeded -- fetched just now, or a fresh cache left by a recent
//     successful fetch;
//   - NONE if there are no watched satellites, so nothing was ever fetched.
// ARMED is not produced yet: nothing currently queues a ping to go out later.
RelayItem CarrierPing(const std::vector<std::optional<net::TleSource>>& sources);

} // namespace app
