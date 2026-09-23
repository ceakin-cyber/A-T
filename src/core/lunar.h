#pragma once

#include <optional>

namespace core {

// The Moon's phase at a given moment: how far through the ~29.5-day synodic cycle (new moon to
// new moon) it is, and how much of its disk, as seen from Earth, is lit.
struct LunarPhase {
    double ageDays = 0.0;            // days since the most recent new moon, in [0, SynodicMonthDays())
    double illuminatedFraction = 0.0; // 0 at new moon, 1 at full moon
};

// The length of the mean synodic month (new moon to new moon), in days. Derived from the same
// mean elongation rate LunarPhaseAt uses (445267.1114034 degrees per Julian century, in
// LunarPhaseAt's D term), so the two stay consistent with each other, rather than hardcoding the
// commonly quoted ~29.53059 days as a separate, potentially-drifting constant.
double SynodicMonthDays();

// The Moon's phase at Julian date `julianDate` (see core/time.h). Computed from the Moon's and
// Sun's mean orbital elements -- the mean elongation of the Moon from the Sun, the Sun's mean
// anomaly, and the Moon's mean anomaly -- plus the Moon's largest periodic perturbation terms for
// the illuminated fraction (Meeus, Astronomical Algorithms, chapters 47-48: the same source this
// project already draws its solar and sidereal time formulas from), not from an external service
// or ephemeris file. This is the low-precision form: no correction terms are applied to the age,
// only to the illuminated fraction, so ageDays can be off by up to about half a day and
// illuminatedFraction by roughly a percent (verified against the independent `pyephem` library
// across several reference dates; see tests/lunar_test.cpp). UT1/TT are not distinguished here,
// far smaller than this calculation's own error.
LunarPhase LunarPhaseAt(double julianDate);

// The next new moon (illuminatedFraction at its lowest) or full moon (at its highest) at or
// after `fromJulianDate`. Found by stepping forward through time in `stepDays` increments,
// watching for illuminatedFraction to stop moving toward the target extreme, then narrowing that
// bracket to about a minute by golden-section search -- the same forward-stepping-then-refine
// approach core::FindPasses/FindMaxElevation (core/passes.h) use for satellite passes, applied
// here to the illuminated-fraction curve instead of elevation. Because the search works against
// illuminatedFraction's own periodic correction terms (not the uncorrected mean elongation
// LunarPhaseAt's own ageDays is based on), the result is markedly more accurate than deriving a
// next-event time from ageDays alone: within about 20 minutes of the independent `pyephem`
// library across several cases, versus ageDays's own roughly half-day error (see
// tests/lunar_test.cpp).
//
// stepDays only needs to be much finer than half a synodic month (~14.8 days) to not miss the
// target, which the default comfortably is. Returns nullopt only if nothing is found within
// maxDays, which should never happen in practice: consecutive new moons (or consecutive full
// moons) are at most about 29.6 days apart, well under the default.
std::optional<double> NextNewMoon(double fromJulianDate, double stepDays = 1.0,
                                  double maxDays = 40.0);
std::optional<double> NextFullMoon(double fromJulianDate, double stepDays = 1.0,
                                   double maxDays = 40.0);

// The traditional name for the Moon's phase at a given age since new moon (see
// LunarPhase::ageDays), one of the 8 standard divisions of the cycle: "NEW MOON", "WAXING
// CRESCENT", "FIRST QUARTER", "WAXING GIBBOUS", "FULL MOON", "WANING GIBBOUS", "LAST QUARTER", or
// "WANING CRESCENT". Each of the four named moments (new, first quarter, full, last quarter)
// owns the eighth of the cycle centered on it; the four waxing/waning names fill the eighths in
// between. Based on age, not illuminatedFraction, because illuminatedFraction alone cannot tell
// a waxing phase from its waning mirror (both halves of the cycle pass through the same
// illuminated fractions).
const char* MoonPhaseName(double ageDays);

} // namespace core
