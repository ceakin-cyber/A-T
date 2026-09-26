#include "app/relay_queue.h"

namespace app {

const char* ToString(RelayState state) {
    switch (state) {
    case RelayState::Sent:
        return "SENT";
    case RelayState::Hold:
        return "HOLD";
    case RelayState::Armed:
        return "ARMED";
    case RelayState::None:
        return "NONE";
    }
    return "UNKNOWN";
}

RelayItem CarrierPing(const std::vector<std::optional<net::TleSource>>& sources) {
    RelayItem item{"CARRIER PING", RelayState::None};
    if (sources.empty()) {
        return item;
    }
    item.state = RelayState::Sent;
    for (const std::optional<net::TleSource>& source : sources) {
        if (!source || *source == net::TleSource::StaleCache) {
            item.state = RelayState::Hold;
            break; // nothing worse than a failed fetch; no need to keep looking
        }
    }
    return item;
}

} // namespace app
