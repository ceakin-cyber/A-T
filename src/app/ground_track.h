#pragma once

#include "core/geodetic.h"
#include "core/sgp4.h"
#include "net/tle_cache.h"

#include <chrono>
#include <vector>

namespace app {

// The satellite's ground track from `now - span` to `now + span`, sampled every `step`, in time
// order. A point where the propagator fails is skipped, so the result can be shorter than the
// full span implies, but every point in it is valid.
std::vector<core::Geodetic> ComputeGroundTrack(const core::Sgp4Model& model,
                                               net::Clock::time_point now,
                                               std::chrono::minutes span = std::chrono::minutes(50),
                                               std::chrono::minutes step = std::chrono::minutes(2));

// True if a line from a point at lon1Deg to one at lon2Deg (both in [-180, 180]) should not be
// drawn directly, because the ground track actually crosses the +-180 degree meridian between
// them rather than the near side of the globe.
bool CrossesAntimeridian(double lon1Deg, double lon2Deg);

} // namespace app
