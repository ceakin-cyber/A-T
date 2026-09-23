#include "app/transmission_events.h"

namespace app {

bool PassChanged(const std::optional<core::Pass>& previous,
                 const std::optional<core::Pass>& current) {
    if (previous.has_value() != current.has_value()) {
        return true;
    }
    if (!previous.has_value()) {
        return false; // both nullopt: nothing to report
    }
    return previous->riseJd != current->riseJd;
}

bool IsIdle(std::optional<net::Clock::time_point> lastEventTime, net::Clock::time_point now) {
    if (!lastEventTime.has_value()) {
        return true;
    }
    return now - *lastEventTime >= kIdleThreshold;
}

} // namespace app
