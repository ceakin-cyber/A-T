#pragma once

#include "core/sgp4.h"
#include "core/tle.h"
#include "net/tle_cache.h"
#include "net/tle_source.h"

#include <optional>

namespace app {

// A satellite that is ready to propagate, together with where its elements came from.
struct TrackedSatellite {
    core::Tle tle;
    core::Sgp4Model model;
    net::Clock::time_point fetchedAt; // when the TLE was downloaded
    net::TleSource source;
};

// Parses a loaded TLE and initializes SGP4 from it. Returns nullopt, after printing the reason
// to stderr, if the text is not a valid TLE or describes an orbit the propagator does not
// support (deep space).
std::optional<TrackedSatellite> MakeSatellite(const net::LoadedTle& loaded);

// Loads the TLE for a NORAD catalog number (from the cache or Celestrak) and prepares it for
// propagation. Returns nullopt if there is no usable TLE.
std::optional<TrackedSatellite> LoadSatellite(int noradId);

} // namespace app
