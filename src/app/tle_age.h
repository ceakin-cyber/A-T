#pragma once

#include "app/tracked_satellite.h"

#include <chrono>
#include <string>

namespace app {

using Seconds = std::chrono::duration<double>;

enum class TleFreshness {
    Fresh, // up to 3 days old: accurate to a few km
    Aging, // 3 to 7 days: errors of several km to tens of km are possible
    Stale, // more than 7 days: positions can no longer be trusted
};

// Time since the TLE's epoch, meaning when its elements were measured. This is not the time
// since the file was downloaded: a TLE fetched just now can already be a day old. It is negative
// if `now` is before the epoch.
Seconds TleAge(const TrackedSatellite& satellite, net::Clock::time_point now);

// Compact text with at most two units, for example "12M", "5H 12M" or "3D 4H". A negative age is
// shown as zero.
std::string FormatAge(Seconds age);

TleFreshness ClassifyAge(Seconds age);

} // namespace app
