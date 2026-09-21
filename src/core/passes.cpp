#include "core/passes.h"

#include "core/frames.h"
#include "core/topocentric.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {

constexpr double kSecondsPerDay = 86400.0;
constexpr double kMinutesPerDay = 1440.0;
constexpr double kCrossingToleranceDays = 0.05 / kSecondsPerDay;

// Finds the time between `below` and `above` at which the elevation crosses zero. `below` is a
// time at which the elevation is at or below zero and `above` one at which it is above zero,
// in either order.
double RefineCrossing(const Sgp4Model& model, const Geodetic& observer, double below,
                      double above) {
    while (std::abs(above - below) > kCrossingToleranceDays) {
        const double middle = 0.5 * (below + above);
        const std::optional<double> elevation = ElevationAt(model, observer, middle);
        if (!elevation) {
            break;
        }
        if (*elevation > 0.0) {
            above = middle;
        } else {
            below = middle;
        }
    }
    return 0.5 * (below + above);
}

} // namespace

std::optional<double> ElevationAt(const Sgp4Model& model, const Geodetic& observer,
                                  double julianDate) {
    const std::optional<StateVector> state =
        Propagate(model, (julianDate - model.epochJd) * kMinutesPerDay);
    if (!state) {
        return std::nullopt;
    }
    const Vec3 ecef = TemeToEcef(state->position, julianDate);
    return EcefToLookAngles(observer, ecef).elevation;
}

std::vector<Pass> FindPasses(const Sgp4Model& model, const Geodetic& observer, double startJd,
                             double endJd, double stepSeconds) {
    std::vector<Pass> passes;
    if (!(endJd > startJd) || !(stepSeconds > 0.0)) {
        return passes;
    }
    const double step = stepSeconds / kSecondsPerDay;

    std::optional<double> previous = ElevationAt(model, observer, startJd);
    if (!previous) {
        return passes;
    }

    double time = startJd;
    std::optional<Pass> current;
    if (*previous > 0.0) {
        current = Pass{startJd, startJd, true, false};
    }

    while (time < endJd) {
        const double next = std::min(time + step, endJd);
        const std::optional<double> elevation = ElevationAt(model, observer, next);
        if (!elevation) {
            break;
        }

        if (*previous <= 0.0 && *elevation > 0.0) {
            current = Pass{RefineCrossing(model, observer, time, next), 0.0, false, false};
        } else if (*previous > 0.0 && *elevation <= 0.0 && current) {
            current->setJd = RefineCrossing(model, observer, next, time);
            passes.push_back(*current);
            current.reset();
        }

        previous = elevation;
        time = next;
    }

    if (current) {
        current->setJd = time;
        current->setsAfterWindow = true;
        passes.push_back(*current);
    }
    return passes;
}

} // namespace core
