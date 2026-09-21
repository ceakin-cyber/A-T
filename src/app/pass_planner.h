#pragma once

#include "app/tracked_satellite.h"
#include "core/geodetic.h"
#include "core/passes.h"
#include "core/sgp4.h"

#include <optional>
#include <vector>

namespace app {

// Keeps the upcoming passes of one satellite over one observer, and answers "what is the next
// pass" cheaply on every frame. The expensive scan runs only when there is nothing valid to
// answer from: at the first call, when the clock moves outside the scanned window, or when the
// window is nearly used up.
class PassPlanner {
  public:
    PassPlanner(const core::Sgp4Model& model, const core::Geodetic& observer);

    // The pass in progress at `now`, or otherwise the next one to rise. Returns nullopt if there
    // is none in the next two days, or if the propagator fails.
    std::optional<core::Pass> Next(net::Clock::time_point now);

  private:
    void Scan(double fromJd);

    core::Sgp4Model model_;
    core::Geodetic observer_;

    bool scanned_ = false;
    double validFromJd_ = 0.0; // earliest time this scan can answer for
    double windowEndJd_ = 0.0;
    std::vector<core::Pass> passes_;
};

} // namespace app
