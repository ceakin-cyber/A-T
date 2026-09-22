#include "app/ground_track.h"

#include "core/frames.h"
#include "core/time.h"

#include <cmath>

namespace app {

std::vector<core::Geodetic> ComputeGroundTrack(const core::Sgp4Model& model,
                                               net::Clock::time_point now,
                                               std::chrono::minutes span,
                                               std::chrono::minutes step) {
    std::vector<core::Geodetic> track;
    if (step.count() <= 0) {
        return track;
    }

    const double nowJd = core::JulianDateFromTimePoint(now);
    for (double offset = -span.count(); offset <= span.count(); offset += step.count()) {
        const double jd = nowJd + offset / 1440.0;
        const auto state = core::Propagate(model, (jd - model.epochJd) * 1440.0);
        if (!state) {
            continue;
        }
        track.push_back(core::EcefToGeodetic(core::TemeToEcef(state->position, jd)));
    }
    return track;
}

bool CrossesAntimeridian(double lon1Deg, double lon2Deg) {
    return std::fabs(lon1Deg - lon2Deg) > 180.0;
}

} // namespace app
