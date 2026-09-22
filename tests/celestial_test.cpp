#include "core/celestial.h"

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>
#include <vector>

namespace {

constexpr double kPi = std::numbers::pi;

double Radians(double degrees) {
    return degrees * kPi / 180.0;
}

double Degrees(double radians) {
    return radians * 180.0 / kPi;
}

double Norm(const core::Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

void ExpectVec(const core::Vec3& actual, const core::Vec3& expected, double tolerance) {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
    EXPECT_NEAR(actual.z, expected.z, tolerance);
}

TEST(EquatorialToUnitVector, VernalEquinoxIsPlusX) {
    ExpectVec(core::EquatorialToUnitVector(0.0, 0.0), {1.0, 0.0, 0.0}, 1e-15);
}

TEST(EquatorialToUnitVector, NinetyDegreesRaOnTheEquatorIsPlusY) {
    ExpectVec(core::EquatorialToUnitVector(kPi / 2.0, 0.0), {0.0, 1.0, 0.0}, 1e-15);
}

TEST(EquatorialToUnitVector, OneEightyDegreesRaOnTheEquatorIsMinusX) {
    ExpectVec(core::EquatorialToUnitVector(kPi, 0.0), {-1.0, 0.0, 0.0}, 1e-15);
}

TEST(EquatorialToUnitVector, TwoSeventyDegreesRaOnTheEquatorIsMinusY) {
    ExpectVec(core::EquatorialToUnitVector(3.0 * kPi / 2.0, 0.0), {0.0, -1.0, 0.0}, 1e-15);
}

TEST(EquatorialToUnitVector, NorthCelestialPoleIsPlusZRegardlessOfRa) {
    for (const double raDeg : {0.0, 45.0, 123.0, 359.0}) {
        ExpectVec(core::EquatorialToUnitVector(Radians(raDeg), kPi / 2.0), {0.0, 0.0, 1.0}, 1e-12);
    }
}

TEST(EquatorialToUnitVector, SouthCelestialPoleIsMinusZRegardlessOfRa) {
    for (const double raDeg : {0.0, 90.0, 200.0}) {
        ExpectVec(core::EquatorialToUnitVector(Radians(raDeg), -kPi / 2.0), {0.0, 0.0, -1.0},
                  1e-12);
    }
}

TEST(EquatorialToUnitVector, IsAlwaysUnitLength) {
    for (double raDeg = 0.0; raDeg < 360.0; raDeg += 37.0) {
        for (double decDeg = -90.0; decDeg <= 90.0; decDeg += 23.0) {
            const core::Vec3 v = core::EquatorialToUnitVector(Radians(raDeg), Radians(decDeg));
            EXPECT_NEAR(Norm(v), 1.0, 1e-12) << "ra=" << raDeg << " dec=" << decDeg;
        }
    }
}

TEST(EquatorialToUnitVector, ACoordinateOnTheEquatorHasZeroZ) {
    for (const double raDeg : {10.0, 130.0, 250.0}) {
        EXPECT_NEAR(core::EquatorialToUnitVector(Radians(raDeg), 0.0).z, 0.0, 1e-15);
    }
}

// A few well-known bright stars, J2000 coordinates from published catalog values. Expected unit
// vectors were computed independently in Python from those coordinates, not from this code's
// own output.
struct KnownStar {
    const char* name;
    double raDeg;
    double decDeg;
    core::Vec3 expected;
};

const std::vector<KnownStar>& KnownStars() {
    static const std::vector<KnownStar> stars = {
        // Polaris: RA 2h31m49.09s, Dec +89 15'50.8" -- near the north celestial pole.
        {"Polaris",
         (2.0 + 31.0 / 60.0 + 49.09 / 3600.0) * 15.0,
         89.0 + 15.0 / 60.0 + 50.8 / 3600.0,
         {0.010126953205375844, 0.007899111853183503, 0.9999175210239628}},
        // Sirius, the brightest star in the sky: RA 6h45m08.9s, Dec -16 42'58".
        {"Sirius",
         (6.0 + 45.0 / 60.0 + 8.9 / 3600.0) * 15.0,
         -(16.0 + 42.0 / 60.0 + 58.0 / 3600.0),
         {-0.18745405323332234, 0.9392177877421448, -0.2876298404462758}},
        // Vega: RA 18h36m56.3s, Dec +38 47'01".
        {"Vega",
         (18.0 + 36.0 / 60.0 + 56.3 / 3600.0) * 15.0,
         38.0 + 47.0 / 60.0 + 1.0 / 3600.0,
         {0.12509456633092567, -0.7694142985947372, 0.626380863845988}},
    };
    return stars;
}

TEST(EquatorialToUnitVector, MatchesIndependentlyComputedRealStars) {
    for (const KnownStar& star : KnownStars()) {
        SCOPED_TRACE(star.name);
        const core::Vec3 v =
            core::EquatorialToUnitVector(Radians(star.raDeg), Radians(star.decDeg));
        ExpectVec(v, star.expected, 1e-12);
        EXPECT_NEAR(Norm(v), 1.0, 1e-12);
    }
}

// The reference values below were derived and checked independently in Python before this was
// implemented in C++, using closed-form geometric facts about meridian transits and the zenith,
// not by trusting this code's own output.

TEST(EquatorialToHorizontal, MeridianTransitNorthOfZenithFacesDueNorth) {
    // dec > lat, hour angle 0 (RA == LST): the star transits north of the zenith, due north, at
    // altitude 90 - (dec - lat).
    const auto h = core::EquatorialToHorizontal(0.0, Radians(60.0), Radians(40.0), 0.0);
    EXPECT_NEAR(h.altitudeRad, Radians(70.0), 1e-9);
    EXPECT_NEAR(h.azimuthRad, 0.0, 1e-9);
}

TEST(EquatorialToHorizontal, MeridianTransitSouthOfZenithFacesDueSouth) {
    // dec < lat: the star transits south of the zenith, due south, at altitude 90 - (lat - dec).
    const auto h = core::EquatorialToHorizontal(0.0, Radians(10.0), Radians(40.0), 0.0);
    EXPECT_NEAR(h.altitudeRad, Radians(60.0), 1e-9);
    EXPECT_NEAR(h.azimuthRad, Radians(180.0), 1e-9);
}

TEST(EquatorialToHorizontal, AnObjectAtTheObserversLatitudeAndZeroHourAngleIsAtTheZenith) {
    // dec == lat, hour angle 0: directly overhead, azimuth undefined but altitude must be 90.
    const auto h = core::EquatorialToHorizontal(0.0, Radians(40.0), Radians(40.0), 0.0);
    EXPECT_NEAR(h.altitudeRad, Radians(90.0), 1e-6);
}

// Matches the check your own plan calls out: Polaris sits roughly at the observer's latitude,
// regardless of local sidereal time, since it is close to the north celestial pole.
TEST(EquatorialToHorizontal, PolarisAltitudeIsRoughlyTheObserversLatitude) {
    const double polarisRa = Radians((2.0 + 31.0 / 60.0 + 49.09 / 3600.0) * 15.0);
    const double polarisDec = Radians(89.0 + 15.0 / 60.0 + 50.8 / 3600.0);
    for (const double latDeg : {0.0, 30.0, 51.4779, 60.0}) {
        for (const double lstDeg : {0.0, 90.0, 180.0, 270.0}) {
            SCOPED_TRACE(testing::Message() << "lat=" << latDeg << " lst=" << lstDeg);
            const auto h = core::EquatorialToHorizontal(polarisRa, polarisDec, Radians(latDeg),
                                                        Radians(lstDeg));
            EXPECT_NEAR(Degrees(h.altitudeRad), latDeg, 1.0);
        }
    }
}

TEST(EquatorialToHorizontal, MatchesIndependentlyComputedValuesAwayFromTheMeridian) {
    // A star east of the meridian (RA=50 deg, dec=20 deg, observer lat=45 deg, hour angle -30
    // deg) and the same star west of the meridian (hour angle +30 deg): mirror images across the
    // meridian, so they share the same altitude and their azimuths sum to 360 degrees. Computed
    // independently in Python, not from this code's own output.
    const auto east =
        core::EquatorialToHorizontal(Radians(50.0), Radians(20.0), Radians(45.0), Radians(20.0));
    const auto west =
        core::EquatorialToHorizontal(Radians(50.0), Radians(20.0), Radians(45.0), Radians(80.0));
    EXPECT_NEAR(Degrees(east.altitudeRad), 54.814089, 1e-5);
    EXPECT_NEAR(Degrees(east.azimuthRad), 125.375256, 1e-4);
    EXPECT_NEAR(Degrees(west.altitudeRad), 54.814089, 1e-5);
    EXPECT_NEAR(Degrees(west.azimuthRad), 234.624744, 1e-4);
    EXPECT_LT(east.azimuthRad, kPi) << "east of the meridian should be in the eastern half of sky";
    EXPECT_GT(west.azimuthRad, kPi) << "west of the meridian should be in the western half of sky";
}

TEST(EquatorialToHorizontal, AltitudeAndAzimuthStayInRange) {
    for (double raDeg = 0.0; raDeg < 360.0; raDeg += 47.0) {
        for (double decDeg = -80.0; decDeg <= 80.0; decDeg += 31.0) {
            for (double lstDeg = 0.0; lstDeg < 360.0; lstDeg += 67.0) {
                const auto h = core::EquatorialToHorizontal(Radians(raDeg), Radians(decDeg),
                                                            Radians(51.4779), Radians(lstDeg));
                EXPECT_GE(h.altitudeRad, -kPi / 2.0 - 1e-9);
                EXPECT_LE(h.altitudeRad, kPi / 2.0 + 1e-9);
                EXPECT_GE(h.azimuthRad, 0.0);
                EXPECT_LT(h.azimuthRad, 2.0 * kPi);
            }
        }
    }
}

} // namespace
