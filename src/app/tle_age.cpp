#include "app/tle_age.h"

#include "core/time.h"

#include <algorithm>
#include <cstdint>

namespace app {

namespace {

constexpr double kSecondsPerDay = 86400.0;

} // namespace

Seconds TleAge(const TrackedSatellite& satellite, net::Clock::time_point now) {
    const double days = core::JulianDateFromTimePoint(now) - satellite.model.epochJd;
    return Seconds(days * kSecondsPerDay);
}

std::string FormatAge(Seconds age) {
    const std::int64_t totalMinutes = static_cast<std::int64_t>(std::max(0.0, age.count()) / 60.0);
    const std::int64_t days = totalMinutes / 1440;
    const std::int64_t hours = (totalMinutes % 1440) / 60;
    const std::int64_t minutes = totalMinutes % 60;

    if (days > 0) {
        return std::to_string(days) + "D " + std::to_string(hours) + "H";
    }
    if (hours > 0) {
        return std::to_string(hours) + "H " + std::to_string(minutes) + "M";
    }
    return std::to_string(minutes) + "M";
}

TleFreshness ClassifyAge(Seconds age) {
    if (age.count() <= 3.0 * kSecondsPerDay) {
        return TleFreshness::Fresh;
    }
    if (age.count() <= 7.0 * kSecondsPerDay) {
        return TleFreshness::Aging;
    }
    return TleFreshness::Stale;
}

} // namespace app
