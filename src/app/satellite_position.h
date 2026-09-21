#pragma once

#include "app/tracked_satellite.h"
#include "core/geodetic.h"
#include "core/vec3.h"

#include <optional>

namespace app {

// Where a satellite is at one instant, in Earth-fixed terms.
struct SatellitePosition {
    core::Geodetic geodetic;    // latitude and longitude in radians, altitude in km
    core::Vec3 ecef;            // km
    double speedKmPerSec = 0.0; // speed relative to inertial space
};

// Propagates the satellite to the given time and converts the result to latitude, longitude and
// altitude. The clock time is treated as UT1. Returns nullopt if the propagator fails, which
// happens when the TLE is too far from the requested time for the elements to hold.
std::optional<SatellitePosition> ComputePosition(const TrackedSatellite& satellite,
                                                 net::Clock::time_point now);

} // namespace app
