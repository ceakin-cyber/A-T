#include "app/event_log.h"

namespace app {

void EventLog::Add(net::Clock::time_point time, std::string message) {
    entries_.push_back({time, std::move(message)});
    while (entries_.size() > capacity_) {
        entries_.pop_front();
    }
}

bool EventLog::LogVisibilityChange(net::Clock::time_point now, bool isAboveHorizon) {
    const bool firstCall = !lastAboveHorizon_.has_value();
    const bool changed = !firstCall && *lastAboveHorizon_ != isAboveHorizon;
    lastAboveHorizon_ = isAboveHorizon;

    if (!changed) {
        return false;
    }
    Add(now, isAboveHorizon ? "SIGNAL ACQUIRED" : "SIGNAL LOST");
    return true;
}

} // namespace app
