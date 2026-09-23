#include "core/lunar.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace core {

namespace {

constexpr double kDegToRad = std::numbers::pi / 180.0;

// Mean elongation of the Moon from the Sun's own linear term, in degrees per Julian century
// (Meeus 47.2): how fast the Moon pulls away from the Sun in ecliptic longitude, on average.
// 360 degrees of that is exactly one synodic month.
constexpr double kMeanElongationDegPerCentury = 445267.1114034;

// Wraps degrees into [0, 360).
double NormalizeDegrees(double degrees) {
    double wrapped = std::fmod(degrees, 360.0);
    if (wrapped < 0.0) {
        wrapped += 360.0;
    }
    return wrapped;
}

} // namespace

double SynodicMonthDays() {
    return 360.0 * 36525.0 / kMeanElongationDegPerCentury;
}

LunarPhase LunarPhaseAt(double julianDate) {
    const double t = (julianDate - 2451545.0) / 36525.0; // Julian centuries since J2000.0
    const double t2 = t * t;
    const double t3 = t2 * t;
    const double t4 = t3 * t;

    // Mean elongation of the Moon from the Sun, the Sun's mean anomaly, and the Moon's mean
    // anomaly, all in degrees (Meeus 47.2).
    const double meanElongationDeg = 297.8501921 + 445267.1114034 * t - 0.0018819 * t2 +
                                     t3 / 545868.0 - t4 / 113065000.0;
    const double sunMeanAnomalyDeg =
        357.5291092 + 35999.0502909 * t - 0.0001536 * t2 + t3 / 24490000.0;
    const double moonMeanAnomalyDeg = 134.9633964 + 477198.8675055 * t + 0.0087414 * t2 +
                                      t3 / 69699.0 - t4 / 14712000.0;

    const double meanElongation = NormalizeDegrees(meanElongationDeg);
    const double d = meanElongation * kDegToRad;
    const double m = NormalizeDegrees(sunMeanAnomalyDeg) * kDegToRad;
    const double mp = NormalizeDegrees(moonMeanAnomalyDeg) * kDegToRad;

    // Phase angle (Sun-Moon-Earth), low precision, with its largest periodic correction terms
    // (Meeus 48.4): 180 degrees (unlit) at new moon, 0 (fully lit) at full moon.
    const double phaseAngleDeg = 180.0 - meanElongation - 6.289 * std::sin(mp) +
                                 2.100 * std::sin(m) - 1.274 * std::sin(2.0 * d - mp) -
                                 0.658 * std::sin(2.0 * d) - 0.214 * std::sin(2.0 * mp) -
                                 0.110 * std::sin(d);

    LunarPhase phase;
    phase.illuminatedFraction = (1.0 + std::cos(phaseAngleDeg * kDegToRad)) / 2.0;
    phase.ageDays = meanElongation / 360.0 * SynodicMonthDays();
    return phase;
}

namespace {

constexpr double kExtremeToleranceDays = 60.0 / 86400.0; // about a minute

// Golden-section search for where illuminatedFraction reaches its maximum (findMaximum) or
// minimum (!findMaximum) in [a, b], to about a minute. Assumes a single such extremum in the
// interval -- the same assumption core::FindMaxElevation makes about a single elevation peak in
// a pass, which NextExtreme's caller is responsible for bracketing correctly before calling this.
double RefineExtreme(double a, double b, bool findMaximum) {
    constexpr double kInverseGoldenRatio = 0.6180339887498949;
    const auto value = [findMaximum](double jd) {
        const double illuminated = LunarPhaseAt(jd).illuminatedFraction;
        return findMaximum ? illuminated : -illuminated;
    };

    double c = b - kInverseGoldenRatio * (b - a);
    double d = a + kInverseGoldenRatio * (b - a);
    double fc = value(c);
    double fd = value(d);
    while (b - a > kExtremeToleranceDays) {
        if (fc > fd) {
            b = d;
            d = c;
            fd = fc;
            c = b - kInverseGoldenRatio * (b - a);
            fc = value(c);
        } else {
            a = c;
            c = d;
            fc = fd;
            d = a + kInverseGoldenRatio * (b - a);
            fd = value(d);
        }
    }
    return 0.5 * (a + b);
}

// Steps forward from `fromJulianDate`, watching illuminatedFraction for the first step where it
// stops moving toward the target extreme (rising, for a maximum; falling, for a minimum), then
// narrows the two-step bracket around that reversal with RefineExtreme. See NextNewMoon and
// NextFullMoon's shared doc comment in core/lunar.h for the reasoning behind this approach.
std::optional<double> NextExtreme(double fromJulianDate, bool findMaximum, double stepDays,
                                  double maxDays) {
    struct Sample {
        double jd;
        double illuminatedFraction;
    };
    const auto sample = [](double jd) { return Sample{jd, LunarPhaseAt(jd).illuminatedFraction}; };

    Sample current = sample(fromJulianDate);
    std::optional<Sample> previous;
    std::optional<bool> wasApproaching;

    double elapsed = 0.0;
    while (elapsed < maxDays) {
        const double step = std::min(stepDays, maxDays - elapsed);
        const Sample next = sample(current.jd + step);
        const bool isApproaching = findMaximum
                                       ? next.illuminatedFraction > current.illuminatedFraction
                                       : next.illuminatedFraction < current.illuminatedFraction;

        if (wasApproaching.has_value() && *wasApproaching && !isApproaching) {
            const double left = previous ? previous->jd : fromJulianDate;
            return RefineExtreme(left, next.jd, findMaximum);
        }

        wasApproaching = isApproaching;
        previous = current;
        current = next;
        elapsed += step;
    }
    return std::nullopt;
}

} // namespace

std::optional<double> NextNewMoon(double fromJulianDate, double stepDays, double maxDays) {
    return NextExtreme(fromJulianDate, /*findMaximum=*/false, stepDays, maxDays);
}

std::optional<double> NextFullMoon(double fromJulianDate, double stepDays, double maxDays) {
    return NextExtreme(fromJulianDate, /*findMaximum=*/true, stepDays, maxDays);
}

} // namespace core
