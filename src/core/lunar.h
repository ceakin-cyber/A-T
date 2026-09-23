#pragma once

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

} // namespace core
