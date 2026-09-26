#include "app/ground_track.h"

#include "core/frames.h"
#include "core/sun.h"
#include "core/time.h"
#include "core/topocentric.h"

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

std::vector<GroundTrackPoint>
ComputeObservedGroundTrack(const core::Sgp4Model& model, const core::Geodetic& observer,
                           net::Clock::time_point now, std::chrono::minutes behind,
                           std::chrono::minutes ahead, std::chrono::seconds step) {
    std::vector<GroundTrackPoint> track;
    if (step.count() <= 0) {
        return track;
    }

    const double nowJd = core::JulianDateFromTimePoint(now);
    const double stepDays = step.count() / 86400.0;
    const double startJd = nowJd - behind.count() / 1440.0;
    // Counted in whole steps, rather than by adding the step to a Julian date over and over,
    // which would let rounding error build up in the times.
    const auto steps = (std::chrono::duration_cast<std::chrono::seconds>(behind + ahead)) / step;
    for (long long i = 0; i <= steps; ++i) {
        const double jd = startJd + static_cast<double>(i) * stepDays;
        const auto state = core::Propagate(model, (jd - model.epochJd) * 1440.0);
        if (!state) {
            continue;
        }
        const core::Vec3 ecef = core::TemeToEcef(state->position, jd);
        track.push_back({core::EcefToGeodetic(ecef), jd,
                         core::EcefToLookAngles(observer, ecef).elevation > 0.0,
                         core::IsSunlit(ecef, core::SunDirectionEcef(jd))});
    }
    return track;
}

bool CrossesAntimeridian(double lon1Deg, double lon2Deg) {
    return std::fabs(lon1Deg - lon2Deg) > 180.0;
}

} // namespace app
