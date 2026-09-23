#pragma once

#include "net/tle_source.h"

#include <chrono>
#include <string>

namespace app {

// Text for the tracker panel. Angles come in as radians and go out in degrees, with the
// hemisphere spelled as a letter, for example "40.8729 N" or "38.1415 W".
std::string FormatLatitude(double radians);
std::string FormatLongitude(double radians);

// An elevation in radians as degrees, for example "33.2 DEG".
std::string FormatElevation(double radians);

// "420.16 KM" and "7.66 KM/S".
std::string FormatAltitudeKm(double kilometers);
std::string FormatSpeedKmPerSec(double kilometersPerSecond);

// A UTC time as "09-21 14:32:10" (month-day hour:minute:second).
std::string FormatUtcTime(std::chrono::system_clock::time_point time);

// Time remaining or a pass length: "45S", "12M 05S", "1H 23M" or "1D 3H". Negative shows as "0S".
std::string FormatCountdown(std::chrono::duration<double> duration);

// The header bar's MODE text for where the current TLE came from: "LIVE" for a fresh network
// fetch, "CACHED" for a fresh cache hit, or "LOW-VISIBILITY" once the app is running on a stale
// cache after a failed fetch.
std::string FormatNodeMode(net::TleSource source);

// An illuminated fraction (0 to 1, see core::LunarPhase) as a whole-number percentage, for
// example "78%".
std::string FormatIlluminatedFraction(double fraction);

} // namespace app
