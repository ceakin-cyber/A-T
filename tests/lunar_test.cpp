#include "core/lunar.h"

#include <gtest/gtest.h>

namespace {

TEST(SynodicMonthDays, MatchesTheCommonlyQuotedMeanValue) {
    // ~29.53059 days is the standard quoted mean synodic month.
    EXPECT_NEAR(core::SynodicMonthDays(), 29.530589, 1e-5);
}

// Reference illuminated fraction and age-since-new-moon values below are from the independent
// `pyephem` library (itself built on more complete lunar/solar theory than this project's own
// low-precision formula), computed once offline for each Julian date. Tolerances are set from
// the actual discrepancy observed against that reference across these cases (illuminated
// fraction within about 0.003, age within about 0.55 days), the same way this project verifies
// SGP4 against python-sgp4 (see README's Verification section).
struct ReferenceCase {
    const char* label;
    double julianDate;
    double illuminatedFraction;
    double ageDays;
};

constexpr ReferenceCase kReferenceCases[] = {
    {"2000-01-01", 2451544.50000, 0.27161, 24.06136},
    {"near a new moon, 2000-01-06", 2451550.25972, 0.00020, 0.00024},
    {"near a full moon, 2000-01-21", 2451564.69514, 0.99999, 14.43566},
    {"2026-09-22 (this project's present)", 2461305.50000, 0.78307, 10.85631},
    {"2026-01-01", 2461041.50000, 0.91400, 11.92828},
    {"1987-04-10 (Meeus's own worked-example date elsewhere in this suite)", 2446895.50000,
     0.82603, 11.46838},
    {"near a new moon, 2020-06-21", 2459022.00000, 0.00055, 0.22123},
    {"near a first quarter, 2026-09-18", 2461302.36368, 0.50121, 7.71999},
    {"near a last quarter, 2026-09-04", 2461287.82719, 0.50133, 22.59340},
};

TEST(LunarPhaseAt, MatchesTheIndependentPyephemReference) {
    for (const ReferenceCase& c : kReferenceCases) {
        const core::LunarPhase phase = core::LunarPhaseAt(c.julianDate);
        EXPECT_NEAR(phase.illuminatedFraction, c.illuminatedFraction, 0.01) << c.label;
        EXPECT_NEAR(phase.ageDays, c.ageDays, 0.6) << c.label;
    }
}

TEST(LunarPhaseAt, IlluminatedFractionStaysInZeroToOne) {
    // Sweep a bit over four synodic months so every phase is exercised more than once.
    for (double jd = 2451545.0; jd < 2451545.0 + 4.0 * core::SynodicMonthDays(); jd += 0.5) {
        const core::LunarPhase phase = core::LunarPhaseAt(jd);
        EXPECT_GE(phase.illuminatedFraction, 0.0) << jd;
        EXPECT_LE(phase.illuminatedFraction, 1.0) << jd;
    }
}

TEST(LunarPhaseAt, AgeDaysStaysInZeroToTheSynodicMonth) {
    for (double jd = 2451545.0; jd < 2451545.0 + 4.0 * core::SynodicMonthDays(); jd += 0.5) {
        const core::LunarPhase phase = core::LunarPhaseAt(jd);
        EXPECT_GE(phase.ageDays, 0.0) << jd;
        EXPECT_LT(phase.ageDays, core::SynodicMonthDays()) << jd;
    }
}

TEST(LunarPhaseAt, AgeDaysAdvancesByAboutOneDayPerDay) {
    // Away from the wraparound at a new moon, age should track real elapsed time almost exactly
    // (the Moon's own mean motion is what defines it), not jump or run backward.
    const double jd = 2451550.5; // a few days after the 2000-01-06 new moon reference above
    const double age1 = core::LunarPhaseAt(jd).ageDays;
    const double age2 = core::LunarPhaseAt(jd + 1.0).ageDays;
    EXPECT_NEAR(age2 - age1, 1.0, 0.05);
}

TEST(LunarPhaseAt, NewMoonAndFullMoonAreOppositeInIlluminatedFraction) {
    // The mean elongation reaches 180 degrees (full moon) almost exactly half a synodic month
    // after it reaches 0 (new moon).
    const double newMoonJd = 2451550.25972; // near-exact new moon reference above
    const double fullMoonJd = newMoonJd + core::SynodicMonthDays() / 2.0;
    EXPECT_NEAR(core::LunarPhaseAt(newMoonJd).illuminatedFraction, 0.0, 0.01);
    EXPECT_NEAR(core::LunarPhaseAt(fullMoonJd).illuminatedFraction, 1.0, 0.01);
}

} // namespace
