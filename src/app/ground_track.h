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

// One sample of a ground track, with what an observer on the ground would make of it.
struct GroundTrackPoint {
    core::Geodetic geodetic;   // the point directly beneath the satellite
    double julianDate = 0.0;   // when it is there
    bool aboveHorizon = false; // whether the satellite is above the observer's horizon then
    bool sunlit = false;       // whether the satellite is in sunlight then (see core::IsSunlit)
};

// The satellite's ground track from `now - behind` to `now + ahead`, sampled every `step`, in
// time order, each point marked with whether the satellite is above `observer`'s horizon and
// whether it is sunlit at that moment. The defaults cover the last half orbit and the next two
// or so, for a satellite in low Earth orbit like the ISS (about 93 minutes an orbit), so the
// passes coming up over the observer show on the map before they happen. A point where the
// propagator fails is skipped, as in ComputeGroundTrack.
std::vector<GroundTrackPoint>
ComputeObservedGroundTrack(const core::Sgp4Model& model, const core::Geodetic& observer,
                           net::Clock::time_point now,
                           std::chrono::minutes behind = std::chrono::minutes(45),
                           std::chrono::minutes ahead = std::chrono::minutes(190),
                           std::chrono::seconds step = std::chrono::seconds(30));

// True if a line from a point at lon1Deg to one at lon2Deg (both in [-180, 180]) should not be
// drawn directly, because the ground track actually crosses the +-180 degree meridian between
// them rather than the near side of the globe.
bool CrossesAntimeridian(double lon1Deg, double lon2Deg);

} // namespace app
