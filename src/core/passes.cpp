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

// Fills in the peak of a pass whose rise and set are known.
void AddPeak(const Sgp4Model& model, const Geodetic& observer, Pass& pass) {
    if (const auto peak = FindMaxElevation(model, observer, pass.riseJd, pass.setJd)) {
        pass.maxElevationJd = peak->julianDate;
        pass.maxElevation = peak->elevation;
    }
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

std::optional<ElevationPeak> FindMaxElevation(const Sgp4Model& model, const Geodetic& observer,
                                              double startJd, double endJd) {
    if (endJd < startJd) {
        return std::nullopt;
    }

    // Golden-section search: shrink the interval by a constant factor each step, keeping the
    // side with the higher elevation.
    constexpr double kInverseGoldenRatio = 0.6180339887498949;
    double a = startJd;
    double b = endJd;
    double c = b - kInverseGoldenRatio * (b - a);
    double d = a + kInverseGoldenRatio * (b - a);
    std::optional<double> fc = ElevationAt(model, observer, c);
    std::optional<double> fd = ElevationAt(model, observer, d);
    if (!fc || !fd) {
        return std::nullopt;
    }
    while (b - a > kCrossingToleranceDays) {
        if (*fc > *fd) {
            b = d;
            d = c;
            fd = fc;
            c = b - kInverseGoldenRatio * (b - a);
            fc = ElevationAt(model, observer, c);
            if (!fc) {
                return std::nullopt;
            }
        } else {
            a = c;
            c = d;
            fc = fd;
            d = a + kInverseGoldenRatio * (b - a);
            fd = ElevationAt(model, observer, d);
            if (!fd) {
                return std::nullopt;
            }
        }
    }

    // The peak can sit right on an edge of the interval (a pass cut off by the window), where the
    // search only gets within its tolerance. Compare with both edges so that case is exact.
    ElevationPeak peak;
    peak.julianDate = 0.5 * (a + b);
    const std::optional<double> middle = ElevationAt(model, observer, peak.julianDate);
    const std::optional<double> atStart = ElevationAt(model, observer, startJd);
    const std::optional<double> atEnd = ElevationAt(model, observer, endJd);
    if (!middle || !atStart || !atEnd) {
        return std::nullopt;
    }
    peak.elevation = *middle;
    if (*atStart > peak.elevation) {
        peak = {startJd, *atStart};
    }
    if (*atEnd > peak.elevation) {
        peak = {endJd, *atEnd};
    }
    return peak;
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
            AddPeak(model, observer, *current);
            passes.push_back(*current);
            current.reset();
        }

        previous = elevation;
        time = next;
    }

    if (current) {
        current->setJd = time;
        current->setsAfterWindow = true;
        AddPeak(model, observer, *current);
        passes.push_back(*current);
    }
    return passes;
}

} // namespace core
