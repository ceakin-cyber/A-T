#pragma once

#include "core/passes.h"
#include "net/tle_cache.h"

#include <chrono>
#include <optional>

namespace app {

// Whether `current` is a meaningfully different pass than `previous`: true if exactly one of the
// two is present, or if both are present but their rise times differ. Used to notice when
// PassPlanner::Next (via SatelliteRoster) has produced a genuinely new answer -- worth a "PASS
// COMPUTED" incoming-transmission event -- as opposed to a frame that just reused the same pass
// as before.
bool PassChanged(const std::optional<core::Pass>& previous, const std::optional<core::Pass>& current);

// How long the incoming-transmission feed can go without a new event before IsIdle reports it as
// idle.
inline constexpr std::chrono::minutes kIdleThreshold{5};

// Whether the incoming-transmission feed is idle right now, worth logging its own "PASSIVE
// MONITORING ENGAGED" fallback message: true if it has never logged anything at all
// (`lastEventTime` is nullopt), or if `now` is at least kIdleThreshold past its most recent
// entry. The caller (see main.cpp) logs that message as a normal entry when this is true, which
// naturally stops IsIdle from firing again until either real activity resumes or another
// kIdleThreshold passes -- so a station that stays quiet gets one fallback message, then another
// every kIdleThreshold after that, like a heartbeat, rather than one every single frame.
bool IsIdle(std::optional<net::Clock::time_point> lastEventTime, net::Clock::time_point now);

} // namespace app
