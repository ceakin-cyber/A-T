#include "core/time.h"

#include <chrono>
#include <gtest/gtest.h>
#include <numbers>

namespace {

constexpr double kTwoPi = 2.0 * std::numbers::pi;

// Converts hours, minutes and seconds of sidereal time to radians.
double SiderealToRadians(int hours, int minutes, double seconds) {
    return (hours * 3600.0 + minutes * 60.0 + seconds) * kTwoPi / 86400.0;
}

// Reference values below come from Meeus, "Astronomical Algorithms", chapters 7 and 12.

TEST(JulianDate, J2000IsNoonOnJanuaryFirst2000) {
    EXPECT_DOUBLE_EQ(core::JulianDate({2000, 1, 1, 12, 0, 0.0}), 2451545.0);
}

TEST(JulianDate, MidnightIsHalfADayBeforeNoon) {
    EXPECT_DOUBLE_EQ(core::JulianDate({2000, 1, 1}), 2451544.5);
    EXPECT_DOUBLE_EQ(core::JulianDate({1999, 1, 1}), 2451179.5);
    EXPECT_DOUBLE_EQ(core::JulianDate({1987, 4, 10}), 2446895.5);
}

TEST(JulianDate, SputnikLaunch) {
    // 1957 October 4.81 UT: 0.81 of a day is 19:26:24.
    EXPECT_NEAR(core::JulianDate({1957, 10, 4, 19, 26, 24.0}), 2436116.31, 1e-6);
}

TEST(JulianDate, HandlesTimeOfDay) {
    EXPECT_NEAR(core::JulianDate({1987, 4, 10, 19, 21, 0.0}), 2446896.30625, 1e-9);
    EXPECT_NEAR(core::JulianDate({2000, 1, 1, 0, 0, 43200.0}), 2451545.0, 1e-9);
}

TEST(JulianDate, GregorianFormulaAppliesToOlderDatesToo) {
    EXPECT_DOUBLE_EQ(core::JulianDate({1600, 1, 1}), 2305447.5);
}

TEST(JulianDate, LeapDayRules) {
    // 2000 is a leap year (divisible by 400), 1900 is not (divisible by 100 only).
    EXPECT_DOUBLE_EQ(core::JulianDate({2000, 3, 1}) - core::JulianDate({2000, 2, 29}), 1.0);
    EXPECT_DOUBLE_EQ(core::JulianDate({1900, 3, 1}) - core::JulianDate({1900, 2, 28}), 1.0);
    EXPECT_DOUBLE_EQ(core::JulianDate({2004, 3, 1}) - core::JulianDate({2004, 2, 28}), 2.0);
}

TEST(JulianDate, JanuaryAndFebruaryUseThePreviousYearInTheFormula) {
    EXPECT_DOUBLE_EQ(core::JulianDate({2000, 2, 1}) - core::JulianDate({2000, 1, 1}), 31.0);
    EXPECT_DOUBLE_EQ(core::JulianDate({2000, 3, 1}) - core::JulianDate({2000, 2, 1}), 29.0);
}

TEST(JulianDateFromEpoch, DayOneIsMidnightOnJanuaryFirst) {
    EXPECT_DOUBLE_EQ(core::JulianDateFromEpoch(2000, 1.0), 2451544.5);
    EXPECT_DOUBLE_EQ(core::JulianDateFromEpoch(2000, 1.5), 2451545.0);
}

TEST(JulianDateFromEpoch, MatchesTheCalendarDateOfATleEpoch) {
    // Day 264 of leap year 2008 is September 20.
    EXPECT_NEAR(core::JulianDateFromEpoch(2008, 264.51782528),
                core::JulianDate({2008, 9, 20}) + 0.51782528, 1e-9);
}

TEST(JulianDateFromEpoch, YearRolloverIsOneDayApartInALeapYear) {
    // 2000 has 366 days, so day 366.0 is December 31 and the next year starts one day later.
    EXPECT_DOUBLE_EQ(core::JulianDateFromEpoch(2001, 1.0) - core::JulianDateFromEpoch(2000, 366.0),
                     1.0);
}

TEST(JulianDateFromTimePoint, UnixEpoch) {
    const std::chrono::system_clock::time_point epoch{};
    EXPECT_DOUBLE_EQ(core::JulianDateFromTimePoint(epoch), 2440587.5);
}

TEST(JulianDateFromTimePoint, J2000) {
    // 2000-01-01T12:00:00Z is 946728000 seconds after the Unix epoch.
    const auto j2000 = std::chrono::system_clock::time_point{std::chrono::seconds(946728000)};
    EXPECT_DOUBLE_EQ(core::JulianDateFromTimePoint(j2000), 2451545.0);
}

TEST(GreenwichMeanSiderealTime, AtJ2000) {
    // The constant term of the polynomial: 18h 41m 50.54841s.
    EXPECT_NEAR(core::GreenwichMeanSiderealTime(2451545.0), SiderealToRadians(18, 41, 50.54841),
                1e-9);
}

TEST(GreenwichMeanSiderealTime, MeeusExample12a) {
    // 1987 April 10, 0h UT: 13h 10m 46.3668s.
    const double jd = core::JulianDate({1987, 4, 10});
    EXPECT_NEAR(core::GreenwichMeanSiderealTime(jd), SiderealToRadians(13, 10, 46.3668), 1e-7);
}

TEST(GreenwichMeanSiderealTime, MeeusExample12b) {
    // 1987 April 10, 19h 21m 00s UT: 8h 34m 57.0896s.
    const double jd = core::JulianDate({1987, 4, 10, 19, 21, 0.0});
    EXPECT_NEAR(core::GreenwichMeanSiderealTime(jd), SiderealToRadians(8, 34, 57.0896), 1e-7);
}

TEST(GreenwichMeanSiderealTime, AdvancesAboutOneExtraDegreePerSolarDay) {
    // A sidereal day is ~3m56s shorter than a solar day, so after 24 h the angle has moved on
    // by 360 degrees plus about 0.9856 degrees.
    const double jd = 2458000.5;
    double delta = core::GreenwichMeanSiderealTime(jd + 1.0) - core::GreenwichMeanSiderealTime(jd);
    if (delta < 0.0) {
        delta += kTwoPi;
    }
    EXPECT_NEAR(delta * 180.0 / std::numbers::pi, 0.98565, 1e-3);
}

TEST(GreenwichMeanSiderealTime, AlwaysWithinZeroToTwoPi) {
    for (double jd = 2415020.5; jd < 2488070.5; jd += 137.31) {
        const double gmst = core::GreenwichMeanSiderealTime(jd);
        EXPECT_GE(gmst, 0.0) << "jd=" << jd;
        EXPECT_LT(gmst, kTwoPi) << "jd=" << jd;
    }
}

} // namespace
