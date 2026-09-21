#include "app/satellite_position.h"

#include "core/frames.h"
#include "core/sgp4.h"
#include "core/time.h"

#include <cmath>

namespace app {

std::optional<SatellitePosition> ComputePosition(const TrackedSatellite& satellite,
                                                 net::Clock::time_point now) {
    constexpr double kMinutesPerDay = 1440.0;

    const double julianDate = core::JulianDateFromTimePoint(now);
    const double minutesSinceEpoch = (julianDate - satellite.model.epochJd) * kMinutesPerDay;

    const std::optional<core::StateVector> state =
        core::Propagate(satellite.model, minutesSinceEpoch);
    if (!state) {
        return std::nullopt;
    }

    SatellitePosition position;
    position.ecef = core::TemeToEcef(state->position, julianDate);
    position.geodetic = core::EcefToGeodetic(position.ecef);
    position.speedKmPerSec =
        std::sqrt(state->velocity.x * state->velocity.x + state->velocity.y * state->velocity.y +
                  state->velocity.z * state->velocity.z);
    return position;
}

} // namespace app
