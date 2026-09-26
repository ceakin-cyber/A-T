#pragma once

#include <string>

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

} // namespace app
